# 20_LVGL_GitHubOTA

Direct GitHub Release OTA update experiment for the WT32-SC01-PLUS Arduino BSP.

Target specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3
16 MiB Flash
2 MiB QSPI PSRAM
```

## Goal

The first firmware installation is performed over USB. After that, the device should be able to check GitHub Releases and update its application firmware directly over HTTPS without a private LAN OTA server.

```text
WT32-SC01-PLUS
      |
      | HTTPS
      v
GitHub Releases
      |
      +-- panlee-github-ota.json
      +-- panlee-github-ota.bin
      |
      v
inactive OTA slot
      |
      | SHA-256 PASS
      v
activate + reboot
```

## Version state

The baseline physically exercised in Part 1 was:

```text
installed on board: 0.1.0
channel: stable
board: panlee-zx3d50ce08s-v15-usrc
```

The source branch is now prepared for the first real OTA candidate:

```text
candidate: 0.1.1
release tag: ota-v0.1.1
```

The board should remain on `0.1.0` until the GitHub Release for `0.1.1` exists. Do not flash `0.1.1` over USB if the goal is to validate the OTA transition itself.

## Part 1 physical result

Video evidence:

- [YouTube Shorts — WT32-SC01-PLUS GitHub OTA, Part 1](https://youtube.com/shorts/gVSZsYNjtj4)

Observed on the reference Panlee specimen:

```text
LVGL OTA interface              PHYSICAL PASS
Touch interaction               PHYSICAL PASS
Saved Wi-Fi reconnect           PHYSICAL PASS
16 MiB flash detected           PHYSICAL PASS
2 MiB QSPI PSRAM detected       PHYSICAL PASS
A/B OTA partition layout        PHYSICAL PASS
GitHub manifest request path    PHYSICAL PASS TO HTTP RESPONSE
Manifest result before Release  HTTP 404 — EXPECTED
Firmware download/install       NOT YET TESTED
SHA-256 physical verification   NOT YET TESTED
A/B slot switch after OTA       NOT YET TESTED
Full GitHub OTA cycle           NOT YET TESTED
```

The `404` in Part 1 is an expected boundary result, not a PASS for firmware download. At that moment no OTA Release/manifest had yet been published at the permanent `latest/download` URL.

## Manifest URL

The device checks:

```text
https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab/releases/latest/download/panlee-github-ota.json
```

Expected manifest:

```json
{
  "schema": 1,
  "board": "panlee-zx3d50ce08s-v15-usrc",
  "version": "0.1.1",
  "channel": "stable",
  "size": 1234567,
  "sha256": "64-lowercase-hex-digits",
  "firmware": "https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab/releases/download/ota-v0.1.1/panlee-github-ota.bin"
}
```

Before enabling INSTALL, the sketch validates:

- board profile ID;
- update channel;
- semantic version ordering;
- firmware byte size;
- SHA-256 field length;
- existence/capacity of the inactive OTA partition.

No GitHub token is stored on the device.

## HTTPS / integrity boundary

`setInsecure()` is intentionally not used.

The sketch verifies HTTPS certificates and follows GitHub redirects. Firmware bytes are streamed into the inactive OTA slot while SHA-256 is calculated. The inactive image is only finalized if the received byte count and SHA-256 match the manifest.

SHA-256 detects corruption or a manifest/image mismatch. It is not a substitute for signed firmware authenticity; signed-image support remains a later hardening step.

## OTA partition layout

The sketch contains its own `partitions.csv`:

```text
nvs       0x009000  0x005000
otadata   0x00E000  0x002000
app0      0x010000  0x640000  ota_0
app1      0x650000  0x640000  ota_1
spiffs    0xC90000  0x360000
coredump  0xFF0000  0x010000
```

Each application slot is:

```text
0x640000 = 6,553,600 bytes = 6.25 MiB
```

During Part 1 the generated build directory was inspected and confirmed to contain this exact partition table and a generated `*.partitions.bin`.

The Arduino CLI `96% / 1,310,720 bytes` message seen during compilation is only the generic board profile's `upload.maximum_size` reporting value. It does not change the sketch-local partition table actually compiled and flashed.

## GitHub Actions release pipeline

Workflow:

```text
.github/workflows/github-ota-release.yml
```

For an `ota-v*` tag it:

1. installs Arduino-ESP32 3.3.8 and LVGL 8.3.11;
2. reads `WT32_OTA_VERSION`;
3. refuses a tag that does not match the firmware version;
4. builds `20_LVGL_GitHubOTA`;
5. extracts the application OTA binary;
6. calculates SHA-256 and size;
7. generates `panlee-github-ota.json`;
8. publishes `panlee-github-ota.bin` and `panlee-github-ota.json` as GitHub Release assets.

## Part 2 — first real OTA cycle

The next physical test is deliberately simple and strong:

```text
board still running 0.1.0
        ↓
publish ota-v0.1.1
        ↓
CHECK GITHUB
        ↓
Available: 0.1.1
        ↓
DOWNLOAD & INSTALL
        ↓
HTTPS download
        ↓
SHA-256 PASS
        ↓
inactive OTA slot activated
        ↓
reboot
        ↓
installed version 0.1.1
        ↓
running slot changed
        ↓
CHECK GITHUB -> UP TO DATE
```

Physical PASS for Example 20 requires all of the following:

- manifest `0.1.1` is found from the permanent GitHub URL;
- firmware asset downloads successfully through GitHub redirects;
- byte count matches manifest;
- SHA-256 matches manifest;
- update finalizes successfully;
- board reboots into the other OTA slot;
- firmware reports `0.1.1` after reboot;
- LVGL display and touch remain operational;
- a second update check reports `UP TO DATE`.

## Rollback boundary

A/B OTA slots do not automatically prove bootloader rollback.

Automatic `PENDING_VERIFY -> rollback` behavior must not be claimed unless the actual bootloader is built with rollback support and a deliberate failed-candidate experiment has been physically executed.

Current scope:

```text
A/B inactive-slot OTA       IMPLEMENTED
HTTPS certificate check     IMPLEMENTED
manifest profile check      IMPLEMENTED
SHA-256 verification        IMPLEMENTED
slot activation + reboot    IMPLEMENTED
GitHub OTA physical cycle   PART 2 PENDING
bootloader auto-rollback    SEPARATE VALIDATION GATE
```

## Build

From repository root:

```powershell
& $CLI compile `
  --fqbn "esp32:esp32:esp32s3:PSRAM=enabled,FlashSize=16M" `
  --library "libraries/WT32_SC01_PLUS" `
  "libraries/WT32_SC01_PLUS/examples/20_LVGL_GitHubOTA"
```

For physical Part 2, do **not** upload candidate `0.1.1` by USB to the test board. The whole point is to let the existing `0.1.0` install it from GitHub.

## Current status

```text
PART 1 BASELINE 0.1.0        PHYSICALLY EXERCISED
PART 1 VIDEO                  DOCUMENTED
EXPECTED PRE-RELEASE 404      OBSERVED
SOURCE CANDIDATE 0.1.1        PREPARED
OTA-V0.1.1 RELEASE            TO BE PUBLISHED
FULL 0.1.0 -> 0.1.1 OTA       PHYSICAL VALIDATION REQUIRED
BOOTLOADER ROLLBACK           SEPARATE VALIDATION GATE
WEB FLASHER                   NOT YET ELIGIBLE
```
