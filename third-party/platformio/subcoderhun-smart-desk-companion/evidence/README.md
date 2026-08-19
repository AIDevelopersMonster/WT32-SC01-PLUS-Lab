# Physical validation evidence

## Reference hardware

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3 (QFN56), revision v0.2
Flash: 16 MB
Embedded PSRAM: 2 MB (hardware observation; application runtime use not verified)
```

## Adaptation provenance

```text
upstream: https://github.com/SubCoderHUN/WT32-SC01-PLUS
snapshot: df8c3f251ee2d9fe8ab0961343251661d1c10e40
upstream license: Apache-2.0
```

## Physical evidence video

[https://youtube.com/shorts/4kxUpJS4kCk](https://youtube.com/shorts/4kxUpJS4kCk)

The recording shows the named Panlee specimen running the adapted 16 MB build and provides physical evidence for the UI/network/audio rows below.

## Validation matrix

| Stage | Status | Evidence/limit |
|---|---|---|
| BUILD | PASS | PlatformIO completed successfully |
| UPLOAD | PASS | Firmware upload completed |
| BOOT | PHYSICAL PASS | Recorded on named specimen |
| ST7796 / LVGL | PHYSICAL PASS | Physical demo video |
| TOUCH | PHYSICAL PASS | Physical demo video |
| UI NAVIGATION | PHYSICAL PASS | Physical demo video |
| BRIGHTNESS | PHYSICAL PASS | Physical demo video |
| WI-FI | PHYSICAL PASS | Physical demo video |
| WEATHER / MOSCOW | PHYSICAL PASS | OpenWeatherMap result shown; secret not published |
| ONLINE RADIO / I2S | PHYSICAL PASS | Playback demonstrated through I2S |
| 16 MB BUILD PROFILE | PASS | Explicit 16 MB profile and successful build |
| MOSCOW TIMEZONE | NEEDS REVIEW | Clock approximately 1 hour behind; not fixed |
| PSRAM RUNTIME USE | NOT YET VERIFIED | `BOARD_HAS_PSRAM` alone is not runtime evidence |

Recorded successful resource summary:

```text
RAM:   78.3% (256712 / 327680)
Flash: 44.0% (2885925 / 6553600)
[SUCCESS]
```

## Claim discipline

The video does not establish EEPROM persistence, SD logging, corrected Moscow time, or PSRAM allocation/use by the running application. Those remain separate tests.
