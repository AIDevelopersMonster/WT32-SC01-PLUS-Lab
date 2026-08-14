from __future__ import annotations

import re
import socket
import time
from dataclasses import dataclass


class OpenOcdError(RuntimeError):
    pass


@dataclass(frozen=True)
class TargetStatus:
    state: str
    raw: str


class OpenOcdClient:
    """Small synchronized client for the OpenOCD telnet command port."""

    def __init__(
        self,
        host: str = "127.0.0.1",
        port: int = 4444,
        target: str = "esp32s3.cpu0",
        timeout: float = 3.0,
    ) -> None:
        self.host = host
        self.port = port
        self.target = target
        self.timeout = timeout
        self.sock: socket.socket | None = None

    def __enter__(self) -> "OpenOcdClient":
        self.connect()
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def connect(self) -> None:
        if self.sock is not None:
            return
        try:
            self.sock = socket.create_connection((self.host, self.port), timeout=self.timeout)
            self.sock.settimeout(0.25)
            self._read_prompt(timeout=self.timeout)
            self.command(f"targets {self.target}")
        except OSError as exc:
            self.close()
            raise OpenOcdError(
                f"Cannot connect to OpenOCD telnet at {self.host}:{self.port}: {exc}"
            ) from exc

    def close(self) -> None:
        if self.sock is not None:
            try:
                self.sock.close()
            finally:
                self.sock = None

    @staticmethod
    def _strip_telnet(data: bytes) -> bytes:
        out = bytearray()
        i = 0
        n = len(data)
        while i < n:
            b = data[i]
            if b != 0xFF:
                out.append(b)
                i += 1
                continue
            if i + 1 >= n:
                break
            command = data[i + 1]
            if command == 0xFF:
                out.append(0xFF)
                i += 2
            elif command in (0xFB, 0xFC, 0xFD, 0xFE):
                i += 3 if i + 2 < n else 2
            elif command == 0xFA:
                end = data.find(b"\xff\xf0", i + 2)
                if end < 0:
                    break
                i = end + 2
            else:
                i += 2
        return bytes(out)

    def _read_prompt(self, *, timeout: float) -> str:
        if self.sock is None:
            raise OpenOcdError("OpenOCD socket is not connected")
        deadline = time.monotonic() + timeout
        raw = bytearray()
        while time.monotonic() < deadline:
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                raise OpenOcdError("OpenOCD closed the telnet connection")
            raw.extend(chunk)
            if bytes(raw).endswith(b"> ") or b"\n> " in raw[-32:]:
                break
        else:
            raise OpenOcdError("Timed out waiting for OpenOCD prompt")
        clean = self._strip_telnet(bytes(raw)).decode("utf-8", errors="replace")
        return clean.replace("\r", "")

    def command(self, command: str, *, timeout: float | None = None) -> str:
        if self.sock is None:
            raise OpenOcdError("OpenOCD socket is not connected")
        try:
            self.sock.sendall(command.encode("ascii") + b"\n")
        except OSError as exc:
            raise OpenOcdError(f"OpenOCD send failed: {exc}") from exc
        text = self._read_prompt(timeout=timeout or self.timeout)
        if text.endswith("> "):
            text = text[:-2]
        lines = text.splitlines()
        while lines and not lines[0].strip():
            lines.pop(0)
        if lines and lines[0].strip() in (command, f"> {command}"):
            lines.pop(0)
        return "\n".join(lines).strip()

    def target_status(self) -> TargetStatus:
        raw = self.command("targets")
        for line in raw.splitlines():
            if self.target in line:
                low = line.lower()
                if "halted" in low:
                    return TargetStatus("halted", raw)
                if "running" in low:
                    return TargetStatus("running", raw)
        low = raw.lower()
        if "halted" in low and "running" not in low:
            return TargetStatus("halted", raw)
        if "running" in low and "halted" not in low:
            return TargetStatus("running", raw)
        return TargetStatus("unknown", raw)

    def wait_halt(self, *, timeout: float, poll_interval: float = 0.10) -> None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            status = self.target_status()
            if status.state == "halted":
                return
            time.sleep(poll_interval)
        raise OpenOcdError(f"Target did not halt within {timeout:.1f} seconds")

    def reset_halt(self) -> None:
        self.command("reset halt", timeout=5.0)
        self.wait_halt(timeout=5.0)

    def reset_run(self) -> None:
        """Perform a normal reset and let the target execute without PC/register edits."""
        self.command("reset run", timeout=5.0)

    def halt(self) -> None:
        self.command("halt", timeout=3.0)
        self.wait_halt(timeout=3.0)

    def resume(self) -> None:
        self.command("resume", timeout=3.0)

    def clear_breakpoints(self) -> None:
        self.command("rbp all")

    def set_hw_breakpoint(self, address: int) -> None:
        self.command(f"bp 0x{address:08x} 2 hw")
        listing = self.command("bp")
        if f"{address:08x}" not in listing.lower():
            raise OpenOcdError(
                f"OpenOCD did not report hardware breakpoint at 0x{address:08X}"
            )

    def read_reg(self, name: str) -> int:
        out = self.command(f"reg {name}")
        values = re.findall(r"0x[0-9a-fA-F]+", out)
        if not values:
            raise OpenOcdError(f"Cannot parse register {name!r} from: {out!r}")
        return int(values[-1], 16)

    def write_reg(self, name: str, value: int, *, verify: bool = True) -> None:
        self.command(f"reg {name} 0x{value:x}")
        if verify:
            actual = self.read_reg(name)
            if actual != value:
                raise OpenOcdError(
                    f"Register {name} verify failed: wrote 0x{value:X}, read 0x{actual:X}"
                )

    def read_bytes(self, address: int, count: int) -> bytes:
        out = self.command(f"mdb 0x{address:08x} {count}")
        values: list[int] = []
        for line in out.splitlines():
            if ":" not in line:
                continue
            rhs = line.split(":", 1)[1]
            for token in re.findall(r"\b[0-9a-fA-F]{2}\b", rhs):
                values.append(int(token, 16))
        if len(values) < count:
            raise OpenOcdError(
                f"Memory read at 0x{address:08X}: expected {count} bytes, got {len(values)}"
            )
        return bytes(values[:count])

    def write_byte(self, address: int, value: int, *, verify: bool = True) -> None:
        if not 0 <= value <= 0xFF:
            raise ValueError("byte value must be in range 0..255")
        self.command(f"mwb 0x{address:08x} 0x{value:02x}")
        if verify:
            actual = self.read_bytes(address, 1)[0]
            if actual != value:
                raise OpenOcdError(
                    f"Memory verify failed at 0x{address:08X}: "
                    f"wrote 0x{value:02X}, read 0x{actual:02X}"
                )
