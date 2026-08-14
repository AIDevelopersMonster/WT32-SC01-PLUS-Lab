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

    def prepare_factory_runner(self) -> list[SignatureResult]:
        """Reach ENTRY_GATE by normal execution before any volatile state injection.

        The verified sequence for this target is:

            reset halt
            arm hardware breakpoint at ENTRY_GATE
            resume
            wait for ENTRY_GATE
            verify flash-mapped firmware signatures
            inject only the recovered volatile selector state

        We intentionally do not read the flash-mapped IROM while still halted in
        reset/ROM state.  On this ESP32-S3/OpenOCD setup such reads may return the
        inaccessible-memory sentinel pattern 0xBAD0BAD0 before the application
        restores its MMU/cache mapping.

        We also intentionally do not arm the breakpoint before a subsequent
        reset: the ESP32-S3 reset path may clear the hardware breakpoint state.
        """
        self.client.clear_breakpoints()
        self.client.reset_halt()

        # Do not verify IROM yet: flash/MMU mapping may not be live at reset halt.
        self.client.set_hw_breakpoint(ENTRY_GATE)
        self.client.resume()
        self.client.wait_halt(timeout=5.0)
        self._assert_pc(ENTRY_GATE, "factory entry gate")

        # The application reached ENTRY_GATE under its own execution, so the
        # flash mapping is live.  Verify the known image before touching state.
        results = self.verify_firmware()

        stack_ptr = self.client.read_reg("a1")
        self.client.write_reg("a10", 1)
        self.client.write_byte(stack_ptr, 0xFF)

        self.client.clear_breakpoints()
        self.client.set_hw_breakpoint(RUNNER_START)
        self.client.resume()
        self.client.wait_halt(timeout=5.0)
        self._assert_pc(RUNNER_START, "factory runner start")
        self.client.clear_breakpoints()
        return results

    def run_stage(self, stage: FactoryStage, *, audio_observe_seconds: float = 20.0) -> str:
        if stage.mode == "refused":
            raise StageRefused(stage.note)

        self.prepare_factory_runner()

        if stage.mode == "async":
            return self._run_audio(stage, audio_observe_seconds=audio_observe_seconds)

        self.client.write_reg("pc", stage.entry)
        self.client.set_hw_breakpoint(stage.next_pc)
        self.client.resume()
        try:
            self.client.wait_halt(timeout=stage.timeout)
            self._assert_pc(stage.next_pc, stage.title)
        except Exception:
            try:
                self.client.halt()
            except Exception:
                pass
            raise
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
        self.client.wait_halt(timeout=stage.timeout)
        self._assert_pc(stage.next_pc, "audio launcher return")
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
