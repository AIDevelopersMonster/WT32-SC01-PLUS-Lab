#!/usr/bin/env python3
"""Package Arduino ESP32-S3 build outputs for the WT32-SC01-PLUS Web Flasher."""

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path


def one(paths: list[Path], label: str) -> Path:
    paths = [p for p in paths if p.is_file()]
    if len(paths) != 1:
        rendered = ", ".join(str(p) for p in paths) or "none"
        raise SystemExit(f"expected exactly one {label}, found: {rendered}")
    return paths[0]


def find_boot_app0() -> Path:
    roots = [Path.home() / ".arduino15", Path.home() / ".platformio"]
    matches: list[Path] = []
    for root in roots:
        if root.exists():
            matches.extend(root.glob("**/tools/partitions/boot_app0.bin"))
    if not matches:
        raise SystemExit("boot_app0.bin was not found in Arduino/PlatformIO package directories")
    matches.sort(key=lambda p: ("3.3.8" not in str(p), len(str(p))))
    return matches[0]


def copy_part(source: Path, destination: Path) -> str:
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    return destination.name


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-root", default=".web-build")
    parser.add_argument("--static-root", default="web-flasher")
    parser.add_argument("--output", default=".web-site")
    args = parser.parse_args()

    build_root = Path(args.build_root)
    static_root = Path(args.static_root)
    output = Path(args.output)

    if output.exists():
        shutil.rmtree(output)
    shutil.copytree(static_root, output)
    (output / ".nojekyll").write_text("", encoding="utf-8")

    catalog = json.loads((static_root / "tests.json").read_text(encoding="utf-8"))
    boot_app0 = find_boot_app0()

    for test in catalog["tests"]:
        test_id = test["id"]
        build_dir = build_root / test_id
        if not build_dir.is_dir():
            raise SystemExit(f"missing build directory: {build_dir}")

        firmware = one(list(build_dir.glob("*.ino.bin")), f"application binary for {test_id}")
        bootloader = one(list(build_dir.glob("*.ino.bootloader.bin")), f"bootloader binary for {test_id}")
        partitions = one(list(build_dir.glob("*.ino.partitions.bin")), f"partition binary for {test_id}")

        firmware_dir = output / "firmware" / test_id
        manifest_dir = output / "manifests"
        manifest_dir.mkdir(parents=True, exist_ok=True)

        bl_name = copy_part(bootloader, firmware_dir / "bootloader.bin")
        pt_name = copy_part(partitions, firmware_dir / "partitions.bin")
        ota_name = copy_part(boot_app0, firmware_dir / "boot_app0.bin")
        fw_name = copy_part(firmware, firmware_dir / "firmware.bin")

        manifest = {
            "name": f"WT32-SC01-PLUS Lab — {test['label']}",
            "version": "main",
            "new_install_prompt_erase": True,
            "builds": [
                {
                    "chipFamily": "ESP32-S3",
                    "parts": [
                        {"path": f"../firmware/{test_id}/{bl_name}", "offset": 0},
                        {"path": f"../firmware/{test_id}/{pt_name}", "offset": 0x8000},
                        {"path": f"../firmware/{test_id}/{ota_name}", "offset": 0xE000},
                        {"path": f"../firmware/{test_id}/{fw_name}", "offset": 0x10000},
                    ],
                }
            ],
        }
        (manifest_dir / f"{test_id}.json").write_text(
            json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
        )
        print(f"packaged {test_id}: {firmware.stat().st_size:,} byte application")

    print(f"web flasher site prepared at {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
