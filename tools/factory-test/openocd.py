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
    """OpenOCD client with Tcl RPC as the default machine interface.

    Tcl RPC uses explicit 0x1A request/response framing and avoids the telnet
    prompt/asynchronous-output races observed during ESP32-S3 SMP halts.
    Telnet remains available as a legacy/debug fallback.
    """

    TCL_TOKEN = b"\x1a"

    def __init__(
        self,
        host: str = "127.0.0.1",
        port: int = 6666,
        target: str = "esp32s3.cpu0",
        timeout: float = 3.0,
        transport: str = "tcl",
    ) -> None:
        if transport not in ("tcl", "telnet"):
            raise ValueError("transport must be 'tcl' or 'telnet'")
        self.host = host
        self.port = port
        self.target = target
        self.timeout = timeout
        self.transport = transport
        self.sock: socket.socket | None = None
        self._rxbuf = bytearray()

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
            if self.transport == "telnet":
                self.sock.settimeout(0.25)
                self._read_telnet_prompt(timeout=self.timeout)
            else:
                self.sock.settimeout(self.timeout)
                # A harmless framed request proves that this is the Tcl RPC port.
                self.command("version")
        except (OSError, OpenOcdError) as exc:
            self.close()
            raise OpenOcdError(
                f"Cannot connect to OpenOCD {self.transport} at "
                f"{self.host}:{self.port}: {exc}"
            ) from exc

    def close(self) -> None:
        if self.sock is not None:
            try:
                self.sock.close()
            finally:
                self.sock = None
                self._rxbuf.clear()

    def command(self, command: str, *, timeout: float | None = None) -> str:
        if self.transport == "tcl":
            return self._command_tcl(command, timeout=timeout)
        return self._command_telnet(command, timeout=timeout)

    def _command_tcl(self, command: str, *, timeout: float | None = None) -> str:
        if self.sock is None:
            raise OpenOcdError("OpenOCD socket is not connected")
        old_timeout = self.sock.gettimeout()
        self.sock.settimeout(timeout or self.timeout)
        try:
            self.sock.sendall(command.encode("utf-8") + self.TCL_TOKEN)
            data = self._read_tcl_frame()
        except OSError as exc:
            raise OpenOcdError(f"OpenOCD Tcl RPC failed for {command!r}: {exc}") from exc
        finally:
            self.sock.settimeout(old_timeout)
        return data.decode("utf-8", errors="replace").replace("\r", "").strip()

    def _read_tcl_frame(self) -> bytes:
        if self.sock is None:
            raise OpenOcdError("OpenOCD socket is not connected")
        while True:
            marker = self._rxbuf.find(self.TCL_TOKEN)
            if marker >= 0:
                frame = bytes(self._rxbuf[:marker])
                del self._rxbuf[: marker + 1]
                return frame
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout as exc:
                raise OpenOcdError("Timed out waiting for OpenOCD Tcl RPC frame") from exc
            if not chunk:
                raise OpenOcdError("OpenOCD closed the Tcl RPC connection")
            self._rxbuf.extend(chunk)

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

    @staticmethod
    def _clean_terminal_text(text: str) -> str:
        while "\x08" in text:
            text = re.sub(r"[^\n]\x08", "", text)
            text = text.replace("\x08", "")
        return text.replace("\r", "")

    def _read_telnet_prompt(self, *, timeout: float) -> str:
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
            raise OpenOcdError("Timed out waiting for OpenOCD telnet prompt")
        clean = self._strip_telnet(bytes(raw)).decode("utf-8", errors="replace")
        return self._clean_terminal_text(clean)

    def _command_telnet(self, command: str, *, timeout: float | None = None) -> str:
        if self.sock is None:
            raise OpenOcdError("OpenOCD socket is not connected")
        try:
            self.sock.sendall(command.encode("ascii") + b"\n")
        except OSError as exc:
            raise OpenOcdError(f"OpenOCD telnet send failed: {exc}") from exc
        text = self._read_telnet_prompt(timeout=timeout or self.timeout)
        if text.endswith("> "):
            text = text[:-2]
        lines = text.splitlines()
        while lines and not lines[0].strip():
            lines.pop(0)
        if lines and lines[0].strip() in (command, f"> {command}"):
            lines.pop(0)
        return "\n".join(lines).strip()

    def _on_target(self, command: str, *, timeout: float | None = None) -> str:
        # On Tcl RPC execute target selection and the operation in one framed
        # interpreter request.  This prevents an SMP target switch from landing
        # between two host commands.  Telnet keeps the legacy two-command form.
        if self.transport == "tcl":
            return self.command(f"targets {self.target}; {command}", timeout=timeout)
        self.command(f"targets {self.target}")
        return self.command(command, timeout=timeout)

    def target_status(self) -> TargetStatus:
        raw = self.command("targets")
        for line in raw.splitlines():
            if self.target in line:
                low = line.lower()
                if "halted" in low:
                    return TargetStatus("halted", raw)
                if "running" in low:
                    return TargetStatus("running", raw)
        return TargetStatus("unknown", raw)

    def wait_halt(self, *, timeout: float, poll_interval: float = 0.10) -> None:
        deadline = time.monotonic() + timeout
        last = TargetStatus("unknown", "")
        while time.monotonic() < deadline:
            last = self.target_status()
            if last.state == "halted":
                return
            time.sleep(poll_interval)
        raise OpenOcdError(
            f"Target {self.target} did not halt within {timeout:.1f} seconds; "
            f"last targets={last.raw!r}"
        )

    def reset_halt(self) -> None:
        self._on_target("reset halt", timeout=5.0)
        self.wait_halt(timeout=5.0)

    def reset_run(self) -> None:
        self._on_target("reset run", timeout=5.0)

    def halt(self) -> None:
        self._on_target("halt", timeout=3.0)
        self.wait_halt(timeout=3.0)

    def resume(self) -> None:
        self._on_target("resume", timeout=3.0)

    def clear_breakpoints(self) -> None:
        self._on_target("rbp all")

    def set_hw_breakpoint(self, address: int) -> None:
        self._on_target(f"bp 0x{address:08x} 2 hw")
        listing = self._on_target("bp")
        if f"{address:08x}" not in listing.lower():
            raise OpenOcdError(
                f"OpenOCD did not report hardware breakpoint at 0x{address:08X}; "
                f"listing={listing!r}"
            )

    def read_reg(self, name: str) -> int:
        outputs: list[str] = []
        for _ in range(3):
            out = self._on_target(f"reg {name}")
            outputs.append(out)
            values = re.findall(r"0x[0-9a-fA-F]+", out)
            if values:
                return int(values[-1], 16)
            time.sleep(0.02)
        raise OpenOcdError(
            f"Cannot parse register {name!r}; responses: "
            + " | ".join(repr(out) for out in outputs)
        )

    def write_reg(self, name: str, value: int, *, verify: bool = True) -> None:
        self._on_target(f"reg {name} 0x{value:x}")
        if verify:
            actual = self.read_reg(name)
            if actual != value:
                raise OpenOcdError(
                    f"Register {name} verify failed: wrote 0x{value:X}, read 0x{actual:X}"
                )

    def read_bytes(self, address: int, count: int) -> bytes:
        out = self._on_target(f"mdb 0x{address:08x} {count}")
        values: list[int] = []
        for line in out.splitlines():
            if ":" not in line:
                continue
            rhs = line.split(":", 1)[1]
            for token in re.findall(r"\b[0-9a-fA-F]{2}\b", rhs):
                values.append(int(token, 16))
        if len(values) < count:
            raise OpenOcdError(
                f"Memory read at 0x{address:08X}: expected {count} bytes, "
                f"got {len(values)}; response={out!r}"
            )
        return bytes(values[:count])

    def write_byte(self, address: int, value: int, *, verify: bool = True) -> None:
        if not 0 <= value <= 0xFF:
            raise ValueError("byte value must be in range 0..255")
        self._on_target(f"mwb 0x{address:08x} 0x{value:02x}")
        if verify:
            actual = self.read_bytes(address, 1)[0]
            if actual != value:
                raise OpenOcdError(
                    f"Memory verify failed at 0x{address:08X}: "
                    f"wrote 0x{value:02X}, read 0x{actual:02X}"
                )
