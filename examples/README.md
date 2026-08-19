# Examples

Hardware-validation progression for the reference WT32-SC01-PLUS specimen.

## Browser flashing for validated tests

For the physically validated **Panlee / ZX3D50CE08S-V15-USRC / 230208** specimen, use the browser flasher for the published non-destructive Arduino BSP tests:

**[Open the WT32-SC01-PLUS Lab Web Flasher](https://aidevelopersmonster.github.io/WT32-SC01-PLUS-Lab/)**

The Web Flasher uses ESP Web Tools and includes a 115200-baud Web Serial monitor. It is the quickest path for repeating the lab tests without installing Arduino IDE, PlatformIO or esptool locally.

The destructive full-card SD qualification firmware is deliberately excluded from one-click browser flashing. Other OEM/revision boards should be identified before using the Panlee-specific firmware.

| Directory | Purpose | Status |
|---|---|---|
| `00_identity_probe` | MCU, flash, PSRAM, reset reason, heap | **PASS — physical specimen** |
| `01_display_test` | ST7796/I80 initialization and visual patterns | **PASS — physical specimen** |
| `02_touch_test` | FT6336U-compatible discovery and raw touch coordinates | **PASS — raw touch path** |
| `02_touch_orientation_test` | display-assisted five-point raw-to-landscape transform | **PASS — simple orientation transform** |
| `03_storage_test` | read-only SDSPI/card/sector probe | **PASS — physical specimen; media anomaly observed** |
| `04_wifi_test` | Wi-Fi scan/connectivity | TODO |
| `05_ble_test` | BLE advertising/GATT | TODO |
| `06_audio_test` | audio path identification/test | BLOCKED BY HW ID |
| `07_expansion_io_test` | safe GPIO/connector checks | BLOCKED BY PINOUT |
| `08_lvgl_demo` | integrated ST7796 + FT6336U-compatible LVGL touch UI | **READY FOR BUILD / PHYSICAL VALIDATION PENDING** |

Each example has its own README containing prerequisites, expected output, safety notes and hardware/evidence status.

A status is promoted to **PASS** only after the relevant firmware has been run on the named physical specimen and the required observations have been recorded under `evidence/`.
