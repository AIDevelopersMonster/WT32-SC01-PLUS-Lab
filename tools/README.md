# Tools

Host-side tools automate identity capture, firmware hashing, acceptance manifests and evidence folder generation.

## `run_hw01_identity.py`

Read-only first-pass identity capture for WT32-SC01-PLUS / ESP32-S3-class boards using Espressif `esptool`.

The tool is intended for the safest initial USB or USB-TTL interrogation of an incompletely identified OEM specimen. It does **not** erase or write flash and does not burn eFuses.

Typical Windows usage from the repository root:

```powershell
python tools/run_hw01_identity.py --port COM7
```

If the serial port is omitted, `esptool` is allowed to auto-detect a candidate port:

```powershell
python tools/run_hw01_identity.py
```

Optional unique/security-related reads are deliberately opt-in:

```powershell
python tools/run_hw01_identity.py --port COM7 --include-mac
python tools/run_hw01_identity.py --port COM7 --include-efuse
```

The default capture records available chip identity/revision information, SPI flash identification, security information and flash status. Reports are written as timestamped text and JSON files under `board-info/` unless another output directory is selected.

The workflow follows the evidence-oriented approach used in `esp32-2432s028-lab`, adapted for ESP32-S3-class identification and this repository's specimen/profile schema.
