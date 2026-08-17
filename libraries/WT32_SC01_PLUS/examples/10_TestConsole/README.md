# 10_TestConsole — modular Arduino diagnostics launcher

This example is a **launcher/quick-check console**, not a replacement for the dedicated qualification examples.

Each test lives in its own tab/file so it can be edited, extended or replaced independently:

```text
10_TestConsole.ino
TestFramework.h
ConsoleGui.h
test_01_display.h
test_02_touch.h
test_03_storage.h
test_04_audio.h
test_05_io.h
test_06_wifi.h
test_07_ble.h
test_08_rs485.h
test_09_system.h
```

## Two front ends

### Serial CLI

At 115200 baud:

```text
1..9  run one test by number
a     full sequential suite
q     automatic quick suite
?     print menu/status
```

The automatic quick suite skips interactive Touch, External IO and pending RS485 checks.

### Touch GUI

The LCD shows a 3x3 grid of large numbered buttons `1..9`.

- gray = idle
- blue = running
- green = pass
- red = fail
- yellow = pending

Tap a numbered tile to run that test. Test names and detailed diagnostics are always printed to Serial.

The GUI deliberately uses only the already-validated BSP display/touch primitives and a tiny internal numeric renderer. It does not depend on LVGL or an external font library.

## Test registry

| ID | Module | Combined-console role |
|---:|---|---|
| 1 | Display | draw-path quick check |
| 2 | Touch | new-touch event check |
| 3 | SD | read-only mount/root check |
| 4 | Audio | I2S tone execution |
| 5 | External IO | six-input one-hot interactive check |
| 6 | Wi-Fi | active radio scan |
| 7 | BLE | active BLE scan |
| 8 | RS485 | `PENDING` until external adapter/peer validation |
| 9 | System | board/flash/PSRAM/heap information |

## Why the dedicated tests remain separate

The existing examples remain the authoritative deep/qualification tests. For example:

- `03_StorageTest` and `04_SDDestructiveTest` contain deeper SD validation;
- `05_AudioTest` performs the high-power audio sequence;
- `08_WiFiTest` performs association, DHCP, DNS, TCP/HTTP and reconnect validation;
- `09_BLETest` performs advertising, connect, GATT READ/WRITE/NOTIFY and PING/PONG validation;
- `06_RS485Test` is the dedicated physical round-trip qualification once the USB-RS485 peer is available.

`10_TestConsole` provides a common operator interface and fast regression entry point while preserving those tests as independent source files.

## Safety

- The combined SD test is read-only. The destructive full-media SD test is **never** launched from the console.
- External IO accepts only a 3.3 V stimulus; never apply the Extended I/O `+5V` rail to an ESP32-S3 GPIO.
- RS485 remains pending until the external peer is physically available.
