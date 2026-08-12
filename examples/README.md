# Examples

Planned progression:

| Directory | Purpose | Status |
|---|---|---|
| `00_identity_probe` | MCU, flash, PSRAM, reset reason, heap | TODO |
| `01_display_test` | display initialization and patterns | BLOCKED BY HW ID |
| `02_touch_test` | raw touch and calibration | BLOCKED BY HW ID |
| `03_storage_test` | safe read-only storage probe | BLOCKED BY HW ID |
| `04_wifi_test` | Wi-Fi scan/connectivity | TODO |
| `05_ble_test` | BLE advertising/GATT | TODO |
| `06_audio_test` | audio path identification/test | BLOCKED BY HW ID |
| `07_expansion_io_test` | safe GPIO/connector checks | BLOCKED BY PINOUT |
| `08_lvgl_demo` | integrated LVGL demo | BLOCKED BY DISPLAY+TOUCH |

Each example should have its own README containing prerequisites, expected output, safety notes and hardware status.
