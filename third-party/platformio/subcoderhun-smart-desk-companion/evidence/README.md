# Physical validation evidence

This directory records reproducible evidence for the SubCoderHUN Smart Desk Companion on the reference Panlee WT32-SC01-PLUS specimen.

## Reference hardware

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3 (QFN56), revision v0.2
Flash: 16 MB
Embedded PSRAM: 2 MB
```

## Upstream snapshot

```text
repository: https://github.com/SubCoderHUN/WT32-SC01-PLUS
commit: df8c3f251ee2d9fe8ab0961343251661d1c10e40
license: Apache-2.0
```

## Evidence stages

Record each stage independently rather than treating the whole application as a single pass/fail result.

| Stage | Status | Evidence |
|---|---|---|
| PlatformIO dependency resolution | PENDING | build log |
| Upstream compile | PENDING | build log |
| Upload / boot | PENDING | upload + serial log |
| LCD / LVGL UI | PENDING | photo/video |
| Capacitive touch | PENDING | photo/video |
| UI navigation | PENDING | photo/video |
| Brightness control | PENDING | observation |
| Wi-Fi association | PENDING | serial/UI evidence |
| NTP/time | PENDING | serial/UI evidence |
| Weather | PENDING | user-controlled API key; no secret committed |
| EEPROM persistence | PENDING | reboot test |
| SD logging | PENDING | optional test |
| I2S online radio | PENDING | optional external-audio test |

## Claim discipline

A successful display/touch boot is not automatically a pass for network, weather, SD, EEPROM, or audio. Promote each subsystem only when direct evidence exists.

Video links and relevant logs will be added here after each physical run.
