from __future__ import annotations

import time

from firmware import SignatureResult, target_matches, verify_target
from openocd import OpenOcdClient, OpenOcdError
from tests import FactoryStage


ENTRY_GATE = 0x4200690E
RUNNER_START = 0x42007060


class FirmwareMismatch(OpenOcdError):
    pass


class StageRefused(OpenOcdError):
    pass


class FactoryRunner:
    def __init__(self, client: OpenOcdClient) -> None:
        self.client = client

    def verify_firmware(self) -> list[SignatureResult]:
        results = verify_target(self.client)
        if not target_matches(results):
            details = []
            for result in results:
                if not result.matched:
                    details.append(
                        f"{result.signature.name} at 0x{result.signature.address:08X}: "
                        f"expected {result.signature.expected.hex(' ')}, "
                        f"read {result.actual.hex(' ')}"
                    )
            raise FirmwareMismatch(
                "FIRMWARE_MISMATCH: control-flow manipulation refused. "
                + " | ".join(details)
            )
        return results

    def _assert_pc(self, expected: int, label: str) -> None:
        actual = self.client.read_reg("pc")
        if actual != expected:
            raise OpenOcdError(
                f"Unexpected PC for {label}: expected 0x{expected:08X}, got 0x{actual:08X}"
            )

    def _wait_at(self, expected: int, label: str, timeout: float) -> None:
        print(f"[JTAG] waiting for {label} @ 0x{expected:08X} ...")
        try:
            self.client.wait_halt(timeout=timeout)
        except OpenOcdError as exc:
            pc_text = "unknown"
            targets_text = "unavailable"
            try:
                self.client.halt()
                pc_text = f"0x{self.client.read_reg('pc'):08X}"
                targets_text = " | ".join(self.client.command("targets").splitlines())
            except Exception:
                pass
            raise OpenOcdError(
                f"Timeout waiting for {label} @ 0x{expected:08X}; "
                f"forced-halt PC={pc_text}; targets={targets_text}"
            ) from exc
        self._assert_pc(expected, label)
        print(f"[JTAG] reached {label} @ 0x{expected:08X}")

    def prepare_factory_runner(self) -> list[SignatureResult]:
        """Reach ENTRY_GATE by normal execution before any volatile state injection."""
        print("[JTAG] clear breakpoints")
        self.client.clear_breakpoints()
        print("[JTAG] reset halt")
        self.client.reset_halt()

        # Flash-mapped IROM is deliberately not verified here: immediately after
        # reset-halt this target can expose the inaccessible 0xBAD0BAD0 sentinel.
        print(f"[JTAG] arm HW breakpoint at factory entry gate 0x{ENTRY_GATE:08X}")
        self.client.set_hw_breakpoint(ENTRY_GATE)
        print("[JTAG] resume normal boot")
        self.client.resume()
        self._wait_at(ENTRY_GATE, "factory entry gate", 5.0)

        print("[JTAG] verify firmware signatures with application flash mapping live")
        results = self.verify_firmware()
        print("[JTAG] firmware signatures VERIFIED")

        stack_ptr = self.client.read_reg("a1")
        print(f"[JTAG] inject volatile factory selector state at stack 0x{stack_ptr:08X}")
        self.client.write_reg("a10", 1)
        self.client.write_byte(stack_ptr, 0xFF)

        self.client.clear_breakpoints()
        print(f"[JTAG] arm HW breakpoint at runner boundary 0x{RUNNER_START:08X}")
        self.client.set_hw_breakpoint(RUNNER_START)
        self.client.resume()
        self._wait_at(RUNNER_START, "factory runner boundary", 5.0)
        self.client.clear_breakpoints()
        return results

    def run_stage(self, stage: FactoryStage, *, audio_observe_seconds: float = 20.0) -> str:
        if stage.mode == "refused":
            raise StageRefused(stage.note)

        self.prepare_factory_runner()

        if stage.mode == "async":
            return self._run_audio(stage, audio_observe_seconds=audio_observe_seconds)

        print(f"[JTAG] select {stage.key} stage PC=0x{stage.entry:08X}")
        self.client.write_reg("pc", stage.entry)
        self.client.set_hw_breakpoint(stage.next_pc)
        self.client.resume()
        try:
            self._wait_at(stage.next_pc, f"{stage.key} return boundary", stage.timeout)
        finally:
            try:
                self.client.clear_breakpoints()
            except Exception:
                pass
        return f"{stage.key.upper()} stage returned at 0x{stage.next_pc:08X}"

    def _run_audio(self, stage: FactoryStage, *, audio_observe_seconds: float) -> str:
        if audio_observe_seconds <= 0:
            raise ValueError("audio observation time must be positive")

        self.client.set_hw_breakpoint(stage.next_pc)
        self.client.resume()
        self._wait_at(stage.next_pc, "audio launcher return", stage.timeout)
        self.client.clear_breakpoints()

        self.client.resume()
        try:
            time.sleep(audio_observe_seconds)
        finally:
            self.client.halt()
            self.client.clear_breakpoints()

        return (
            f"AUDIO task launched; target ran for {audio_observe_seconds:.1f} s observation window. "
            "Physical playback completion remains operator-observed."
        )
