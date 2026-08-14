# Examples

Hardware-validation progression for the reference WT32-SC01-PLUS specimen:

| Directory | Purpose | Status |
|---|---|---|
| `00_identity_probe` | MCU, flash, PSRAM, reset reason, heap | **PASS — physical specimen** |
| `01_display_test` | ST7796/I80 initialization and visual patterns | **PASS — physical specimen** |
| `02_touch_test` | raw touch and calibration | BLOCKED BY HW ID |
| `03_storage_test` | read-only SDSPI/card/sector probe | **READY FOR BUILD / PHYSICAL VALIDATION PENDING** |
| `04_wifi_test` | Wi-Fi scan/connectivity | TODO |
| `05_ble_test` | BLE advertising/GATT | TODO |
| `06_audio_test` | audio path identification/test | BLOCKED BY HW ID |
| `07_expansion_io_test` | safe GPIO/connector checks | BLOCKED BY PINOUT |
| `08_lvgl_demo` | integrated LVGL demo | BLOCKED BY DISPLAY+TOUCH |

Each example has its own README containing prerequisites, expected output, safety notes and hardware/evidence status.

A status is promoted to **PASS** only after the relevant firmware has been run on the named physical specimen and the required observations have been recorded under `evidence/`.
