#!/usr/bin/env python3
"""Read-only WT32-SC01-PLUS / ESP32-S3 identity capture via esptool.

This tool intentionally does not erase, write, or modify flash/eFuses.
It is designed for the first passive USB/USB-TTL interrogation of an
incompletely identified board specimen.

Examples:
    python tools/run_hw01_identity.py
    python tools/run_hw01_identity.py --port COM7
    python tools/run_hw01_identity.py --port COM7 --include-mac
    python tools/run_hw01_identity.py --port COM7 --include-efuse
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable


SCRIPT_VERSION = "0.1.0"
DEFAULT_BAUD = 115200


def run_command(argv: list[str], *, timeout: int = 45) -> dict:
    """Run a command, capture combined output, and never raise on tool failure."""
    printable = subprocess.list2cmdline(argv)
    print(f"\n$ {printable}")
    print("-" * 78)

    try:
        cp = subprocess.run(
            argv,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
            timeout=timeout,
            check=False,
        )
        output = cp.stdout or ""
        print(output.rstrip())
        return {
            "command": printable,
            "argv": argv,
            "returncode": cp.returncode,
            "output": output,
        }
    except subprocess.TimeoutExpired as exc:
        output = exc.stdout or ""
        if isinstance(output, bytes):
            output = output.decode(errors="replace")
        msg = f"TIMEOUT after {timeout} seconds"
        print(msg)
        return {
            "command": printable,
            "argv": argv,
            "returncode": 124,
            "output": f"{output}\n{msg}\n",
        }
    except OSError as exc:
        msg = f"ERROR: {exc}"
        print(msg)
        return {
            "command": printable,
            "argv": argv,
            "returncode": 127,
            "output": msg + "\n",
        }


def esptool_base(port: str | None, baud: int) -> list[str]:
    cmd = [sys.executable, "-m", "esptool"]
    if port:
        cmd += ["--port", port]
    cmd += ["--baud", str(baud)]
    return cmd


def detect_esptool_major() -> tuple[int | None, dict]:
    result = run_command([sys.executable, "-m", "esptool", "version"], timeout=15)
    match = re.search(
        r"(?:esptool(?:\.py)?\s+)?v?(\d+)\.(\d+)(?:\.(\d+))?",
        result["output"],
        re.I,
    )
    return (int(match.group(1)) if match else None), result


def command_name(major: int, modern: str, legacy: str) -> str:
    return modern if major >= 5 else legacy


def safe_slug(value: str) -> str:
    value = re.sub(r"[^A-Za-z0-9._-]+", "-", value.strip())
    return value.strip("-.") or "board"


def write_text_report(path: Path, metadata: dict, results: Iterable[dict]) -> None:
    lines = [
        "WT32-SC01-PLUS HW01 identity capture",
        "=" * 78,
        f"Generated: {metadata['generated_at']}",
        f"Script version: {metadata['script_version']}",
        f"Python: {metadata['python']}",
        f"Requested port: {metadata['port'] or 'AUTO'}",
        f"Baud: {metadata['baud']}",
        f"esptool major: {metadata.get('esptool_major')}",
        f"Include MAC: {metadata['include_mac']}",
        f"Include eFuse summary: {metadata['include_efuse']}",
        "",
        "SAFETY: read-only probe; no erase/write/burn commands are issued.",
        "PRIVACY: MAC/eFuse capture is opt-in because it can contain unique identifiers.",
        "",
    ]

    for result in results:
        lines.extend(
            [
                "=" * 78,
                result["command"],
                "=" * 78,
                result["output"].rstrip(),
                f"RETURN CODE: {result['returncode']}",
                "",
            ]
        )

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def find_espefuse_command() -> list[str] | None:
    """Prefer the module installed alongside esptool; fall back to console script."""
    probe = run_command([sys.executable, "-m", "espefuse", "--help"], timeout=15)
    if probe["returncode"] == 0:
        return [sys.executable, "-m", "espefuse"]

    exe = shutil.which("espefuse") or shutil.which("espefuse.py")
    if exe:
        return [exe]
    return None


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Read-only WT32-SC01-PLUS / ESP32 identity capture via esptool. "
            "No flash/eFuse writes are performed."
        )
    )
    parser.add_argument(
        "--port",
        help="Serial port, e.g. COM7 or /dev/ttyUSB0. Omit for esptool auto-detect.",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help=f"Baud rate (default: {DEFAULT_BAUD}).",
    )
    parser.add_argument(
        "--include-mac",
        action="store_true",
        help="Also read the factory MAC address (unique identifier).",
    )
    parser.add_argument(
        "--include-efuse",
        action="store_true",
        help=(
            "Also save an eFuse summary. Read-only, but may contain "
            "unique/security-relevant metadata."
        ),
    )
    parser.add_argument(
        "--output-dir",
        default="board-info",
        help="Directory for reports (default: board-info).",
    )
    parser.add_argument(
        "--label",
        default="wt32-sc01-plus",
        help="Filename label for the capture (default: wt32-sc01-plus).",
    )
    args = parser.parse_args()

    if args.baud <= 0:
        parser.error("--baud must be positive")

    timestamp = dt.datetime.now().astimezone().strftime("%Y%m%d-%H%M%S")
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    stem = f"{safe_slug(args.label)}-hw01-{timestamp}"
    txt_path = output_dir / f"{stem}.txt"
    json_path = output_dir / f"{stem}.json"

    print("WT32-SC01-PLUS HW01 identity capture")
    print("Read-only mode: no flash/eFuse modification commands are used.")
    print(f"Output directory: {output_dir.resolve()}")

    major, version_result = detect_esptool_major()
    results: list[dict] = [version_result]

    if major is None or version_result["returncode"] != 0:
        print("\nERROR: esptool is not available through this Python interpreter.")
        print(f"Try: {sys.executable} -m pip install -U esptool")
        return 2

    base = esptool_base(args.port, args.baud)
    read_only_commands = [
        command_name(major, "chip-id", "chip_id"),
        command_name(major, "flash-id", "flash_id"),
        command_name(major, "get-security-info", "get_security_info"),
        command_name(major, "read-flash-status", "read_flash_status"),
    ]
    if args.include_mac:
        read_only_commands.append(command_name(major, "read-mac", "read_mac"))

    for subcommand in read_only_commands:
        results.append(run_command(base + [subcommand]))

    efuse_json_path: Path | None = None
    if args.include_efuse:
        espefuse = find_espefuse_command()
        if espefuse is None:
            results.append(
                {
                    "command": "espefuse summary --format json",
                    "argv": [],
                    "returncode": 127,
                    "output": "espefuse command/module not found\n",
                }
            )
        else:
            efuse_json_path = output_dir / f"{stem}-efuses.json"
            efuse_cmd = espefuse[:]
            if args.port:
                efuse_cmd += ["--port", args.port]
            efuse_cmd += [
                "--baud",
                str(args.baud),
                "summary",
                "--format",
                "json",
                "--file",
                str(efuse_json_path),
            ]
            results.append(run_command(efuse_cmd))

    metadata = {
        "schema": "wt32-sc01-plus-lab/hw01-identity-capture/v1",
        "script_version": SCRIPT_VERSION,
        "generated_at": dt.datetime.now().astimezone().isoformat(),
        "python": sys.version.replace("\n", " "),
        "platform": sys.platform,
        "port": args.port,
        "baud": args.baud,
        "esptool_major": major,
        "include_mac": args.include_mac,
        "include_efuse": args.include_efuse,
        "efuse_json": str(efuse_json_path) if efuse_json_path else None,
    }

    write_text_report(txt_path, metadata, results)
    json_path.write_text(
        json.dumps({"metadata": metadata, "commands": results}, indent=2, ensure_ascii=False)
        + "\n",
        encoding="utf-8",
    )

    failures = [r for r in results[1:] if r["returncode"] != 0]
    print("\n" + "=" * 78)
    print(f"Text report: {txt_path}")
    print(f"JSON report: {json_path}")
    if efuse_json_path and efuse_json_path.exists():
        print(f"eFuse JSON: {efuse_json_path}")
    if failures:
        print(
            f"Completed with {len(failures)} command failure(s); "
            "inspect the report for details."
        )
        print(
            "This can be normal if a command is unavailable in the chip's "
            "current download/security mode."
        )
        return 1

    print("Capture completed successfully.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
