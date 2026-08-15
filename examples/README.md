# Examples

Hardware-validation progression for the reference WT32-SC01-PLUS specimen:

| Directory | Purpose | Status |
|---|---|---|
| `00_identity_probe` | MCU, flash, PSRAM, reset reason, heap | **PASS — physical specimen** |
| `01_display_test` | ST7796/I80 initialization and visual patterns | **PASS — physical specimen** |
| `02_touch_test` | FT6336U-compatible discovery and raw touch coordinates | **PASS — raw touch path; orientation pending** |
| `03_storage_test` | read-only SDSPI/card/sector probe | **PASS — physical specimen; media anomaly observed** |
| `04_wifi_test` | Wi-Fi scan/connectivity | TODO |
| `05_ble_test` | BLE advertising/GATT | TODO |
| `06_audio_test` | audio path identification/test | BLOCKED BY HW ID |
| `07_expansion_io_test` | safe GPIO/connector checks | BLOCKED BY PINOUT |
| `08_lvgl_demo` | integrated LVGL demo | BLOCKED BY TOUCH ORIENTATION |

Each example has its own README containing prerequisites, expected output, safety notes and hardware/evidence status.

A status is promoted to **PASS** only after the relevant firmware has been run on the named physical specimen and the required observations have been recorded under `evidence/`.
