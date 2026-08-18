# Upstream reference

Canonical repository:

https://github.com/SubCoderHUN/WT32-SC01-PLUS

Author: SubCoderHUN

Validation snapshot:

```text
commit: df8c3f251ee2d9fe8ab0961343251661d1c10e40
checked: 2026-08-18
```

## License

Upstream repository license: **Apache License 2.0**.

This permits redistribution and modification subject to the Apache-2.0 terms, including preservation of applicable license/copyright notices and marking of modifications where required.

Do not assume every bundled third-party dependency or generated asset is automatically relicensed by the project license; retain relevant upstream notices when redistributing a modified application.

## Upstream environment

```text
PlatformIO
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.flash_size = 8MB
board_build.partitions = default_8MB.csv
```

Key declared dependencies include:

```text
lvgl/lvgl@8.3.6
lovyan03/LovyanGFX@1.1.7
plerup/EspSoftwareSerial@^8.2.0
maxgerhardt/ghostl@^1.0.1
squix78/JsonStreamingParser@^1.0.5
fbiego/ESP32Time@^2.0.4
thingpulse/ESP8266 Weather Station@^2.2.0
kiryanenko/SimpleTimer@^1.0.0
esphome/ESP32-audioI2S@^2.0.7
a7md0/WakeOnLan@^1.1.7
marian-craciunescu/ESP32Ping@^1.7
```

Build flags include `BOARD_HAS_PSRAM`, `PLUS=1`, and an LVGL memory size definition.

## Upstream application structure

The checked repository includes:

- `WT32-SC01-PLUS/platformio.ini`
- `WT32-SC01-PLUS/src/main.cpp`
- `WT32-SC01-PLUS/src/Managers/`
- `WT32-SC01-PLUS/src/Features/`
- LVGL/SquareLine generated UI sources under `src/`
- root SquareLine Studio `.spj` / `.sll` project files
- documentation and enclosure resources

## Source-handling policy for this lab

Unlike the first Sukesh Gist comparison, this project has an explicit permissive license. We may therefore create a properly attributed adapted version if useful.

However, the first validation stage remains upstream-first: build and run the canonical source before changing hardware abstraction, dependencies, UI, or application logic.

## Security note

The checked upstream weather implementation contains an API credential in source. The credential is intentionally not copied here. Use a user-controlled credential for network testing and move secrets out of tracked source in any maintained adaptation.
