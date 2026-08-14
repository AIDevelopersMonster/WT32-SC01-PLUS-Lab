#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

from firmware import KNOWN_FACTORY_FLASH_SHA256, sha256_file, target_matches, verify_target
from openocd import OpenOcdClient, OpenOcdError
from runner import FactoryRunner, FirmwareMismatch, StageRefused
from tests import STAGES, list_stages


VERSION = "0.1.1"


def print_banner() -> None:
    print(f"FactoryCTL v{VERSION} - safe JTAG factory-test runner")
    print("Flash writes: DISABLED BY DESIGN")
    print("eFuse writes: DISABLED BY DESIGN")


def print_stage_list() -> None:
    print("\nFactory stages:")
    for stage in list_stages():
        print(
            f"  {stage.key:8s} entry=0x{stage.entry:08X} next=0x{stage.next_pc:08X} "
            f"mode={stage.mode}"
        )
        print(f"           {stage.note}")
    print("  full     REFUSED in v0.1; use isolated stages only")


def confirm_sd(assume_yes: bool) -> bool:
    print("\nWARNING: factory SD code may create/replace:")
    print("  /sdcard/hello.txt")
    print("  /sdcard/foo.txt")
    print("Use an expendable SD card.")
    if assume_yes:
        return True
    try:
        answer = input("Continue? [y/N] ").strip().lower()
    except EOFError:
        return False
    return answer in ("y", "yes")


def make_client(args: argparse.Namespace) -> OpenOcdClient:
    return OpenOcdClient(
        args.host,
        args.port,
        args.target,
        args.timeout,
        transport=args.transport,
    )


def status(args: argparse.Namespace) -> int:
    print_banner()
    print(f"OpenOCD transport: {args.transport}")
    print(f"OpenOCD endpoint: {args.host}:{args.port}")
    print(f"Target: {args.target}")
    print("Safety profile: ESP32-S3 builtin USB-JTAG")
    print("USB factory test: REFUSED in v0.1")

    if args.firmware_bin:
        path = Path(args.firmware_bin)
        digest = sha256_file(path)
        print(f"Factory dump SHA256: {digest}")
        print(
            "Factory dump hash: "
            + ("VERIFIED" if digest == KNOWN_FACTORY_FLASH_SHA256 else "MISMATCH")
        )

    with make_client(args) as client:
        target = client.target_status()
        print(f"CPU state: {target.state}")
        if target.state == "halted":
            results = verify_target(client)
            for result in results:
                label = "MATCH" if result.matched else "MISMATCH"
                print(
                    f"Signature {result.signature.name}: {label} "
                    f"at 0x{result.signature.address:08X}"
                )
            print("Firmware signatures: " + ("VERIFIED" if target_matches(results) else "MISMATCH"))
        else:
            print("Firmware signatures: NOT CHECKED (target is not halted)")
            print("A run command will halt at the factory gate and verify signatures before changing PC/registers.")
    return 0


def run_stage(args: argparse.Namespace) -> int:
    if args.stage == "full":
        print("REFUSED: full-sequence automation is intentionally not implemented in v0.1.")
        return 4
    stage = STAGES[args.stage]
    if stage.mode == "refused":
        print(f"REFUSED: {stage.note}")
        return 4
    if stage.key == "sd" and not confirm_sd(args.yes):
        print("SD stage cancelled.")
        return 4

    print_banner()
    print(f"Requested stage: {stage.key}")
    print(stage.note)
    print(f"OpenOCD transport: {args.transport} ({args.host}:{args.port})")
    if stage.key == "audio":
        print("Do not touch the display during the audio observation window.")

    client = make_client(args)
    try:
        client.connect()
        runner = FactoryRunner(client)
        message = runner.run_stage(
            stage,
            audio_observe_seconds=args.observe_seconds,
        )
        print("\nRESULT:")
        print(message)
        print("Target is left HALTED for inspection.")
        return 0
    except FirmwareMismatch as exc:
        print(f"\n{exc}")
        return 3
    except StageRefused as exc:
        print(f"\nREFUSED: {exc}")
        return 4
    except KeyboardInterrupt:
        print("\nInterrupted by operator; attempting to halt target and clear breakpoints.")
        try:
            client.halt()
            client.clear_breakpoints()
        except Exception as cleanup_exc:
            print(f"Cleanup warning: {cleanup_exc}")
        return 130
    except (OpenOcdError, OSError, ValueError) as exc:
        print(f"\nERROR: {exc}")
        try:
            client.halt()
            client.clear_breakpoints()
        except Exception:
            pass
        return 2
    finally:
        client.close()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Firmware-guarded OpenOCD/JTAG runner for the verified Panlee "
            "ZX3D50CE08S-V15-USRC 230208 factory firmware."
        )
    )
    parser.add_argument("--host", default="127.0.0.1", help="OpenOCD host")
    parser.add_argument(
        "--transport",
        choices=("tcl", "telnet"),
        default="tcl",
        help="OpenOCD control interface (default: tcl RPC)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=6666,
        help="OpenOCD control port (default: 6666 for Tcl RPC; use 4444 with --transport telnet)",
    )
    parser.add_argument("--target", default="esp32s3.cpu0", help="OpenOCD target name")
    parser.add_argument("--timeout", type=float, default=3.0, help="OpenOCD command timeout")
    parser.add_argument(
        "--firmware-bin",
        help="Optional full 16 MiB factory dump to SHA256-check in status mode.",
    )

    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("list", help="List recovered stages and safety classifications")
    sub.add_parser("status", help="Show OpenOCD/target status without changing control flow")

    run = sub.add_parser("run", help="Run one isolated factory stage")
    run.add_argument("stage", choices=[*STAGES.keys(), "full"])
    run.add_argument("--yes", action="store_true", help="Accept the SD write warning")
    run.add_argument(
        "--observe-seconds",
        type=float,
        default=20.0,
        help="Audio observation window after launching the asynchronous task (default: 20).",
    )

    sub.add_parser("show-ok", help="Alias for 'run ok'")
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    if args.port <= 0 or args.timeout <= 0:
        parser.error("--port and --timeout must be positive")

    if args.transport == "telnet" and args.port == 6666:
        args.port = 4444

    if args.command == "list":
        print_banner()
        print_stage_list()
        return 0
    if args.command == "status":
        return status(args)
    if args.command == "show-ok":
        args.stage = "ok"
        args.yes = False
        args.observe_seconds = 20.0
        return run_stage(args)
    if args.command == "run":
        return run_stage(args)
    parser.error("unknown command")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
