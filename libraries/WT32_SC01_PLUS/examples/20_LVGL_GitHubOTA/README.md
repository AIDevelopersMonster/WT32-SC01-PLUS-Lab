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

The first firmware installation is performed over USB (or later through the repository Web Flasher). After that, the device can check GitHub Releases and update its application firmware directly over HTTPS without a private LAN OTA server.

```text
WT32-SC01-PLUS
      |
      | HTTPS
      v
GitHub Releases
      |
      +-- panlee-github-ota.json
      |
      +-- panlee-github-ota.bin
      |
      v
inactive OTA slot
      |
      | SHA-256 PASS
      v
activate + reboot
```

The example is deliberately separate from the earlier ESP32-TUX OTA UI. Project 4 exposed OTA architecture but its upstream example URL was a private LAN address; it was not physically validated as a public update service. Example 20 is intended to close that gap with a reproducible GitHub-hosted pipeline.

## Firmware version

The installed application version is declared in:

[`firmware_version.h`](firmware_version.h)

Current baseline:

```text
version: 0.1.0
channel: stable
board: panlee-zx3d50ce08s-v15-usrc
```

A tagged OTA release must use the matching tag:

```text
ota-v0.1.0
```

When preparing the next OTA test, increment the header first, for example:

```text
0.1.0 -> 0.1.1
```

and publish:

```text
ota-v0.1.1
```

## GitHub manifest

The device checks the permanent GitHub Release alias:

```text
https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab/releases/latest/download/panlee-github-ota.json
```

Expected manifest shape:

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

- required manifest fields;
- exact board profile ID;
- update channel;
- semantic version comparison;
- firmware byte size;
- 64-character SHA-256 field;
- existence and capacity of an inactive OTA partition.

No GitHub token is stored on the device.

## HTTPS boundary

`setInsecure()` is intentionally not used.

The first implementation embeds the required public root CA trust chain and uses `NetworkClientSecure` certificate verification. `HTTPClient` is configured for strict redirects because GitHub release downloads may redirect to GitHub-managed asset storage.

The SHA-256 manifest field is checked independently while the firmware bytes are streamed into the inactive OTA slot.

This provides two different checks:

```text
HTTPS certificate validation
            +
firmware SHA-256 validation
```

The SHA-256 check detects accidental/corrupted or manifest-mismatched firmware. It is not a substitute for a cryptographic firmware-signature scheme; signed images remain a future hardening step.

## OTA partition layout

The sketch contains its own [`partitions.csv`](partitions.csv), derived from the Arduino-ESP32 3.3.8 official `default_16MB.csv` layout:

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
0x640000 = 6.25 MiB
```

The sketch checks `esp_ota_get_next_update_partition()` at runtime and refuses an update if there is no inactive OTA slot or if the advertised image does not fit.

## Wi-Fi

No SSID or Wi-Fi password is stored in this example.

It calls:

```cpp
WiFi.begin();
```

and therefore expects credentials already saved by an earlier successful connection. The physically validated `18_LVGL_QR_Lifecycle` can be used to provision those credentials before the GitHub OTA experiment.

If no saved station connection exists, the UI reports that Wi-Fi provisioning is required and no flash write is attempted.

## UI

The 480x320 LVGL screen shows:

- installed version;
- available GitHub version;
- stable channel;
- current and next OTA partition;
- OTA slot capacity;
- `CHECK GITHUB`;
- `DOWNLOAD & INSTALL`;
- download progress;
- byte counters;
- current OTA status/error.

Typical lifecycle:

```text
READY
  -> CONNECTING WIFI
  -> CHECKING GITHUB
  -> UPDATE AVAILABLE
  -> DOWNLOADING
  -> SHA256 PASS
  -> UPDATE READY
  -> REBOOT
```

## GitHub Actions release pipeline

Workflow:

```text
.github/workflows/github-ota-release.yml
```

It installs:

```text
Arduino-ESP32 3.3.8
LVGL 8.3.11
```

then builds `20_LVGL_GitHubOTA`, extracts the application `.ino.bin`, calculates SHA-256 and byte size, generates `panlee-github-ota.json`, and publishes both files as release assets when an `ota-v*` tag is pushed.

The workflow refuses a tag whose version does not match `firmware_version.h`.

Example release pair:

```text
panlee-github-ota.bin
panlee-github-ota.json
```

## Rollback boundary

A/B OTA slots do not automatically prove bootloader rollback.

The sketch reports the compile-time state of:

```text
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
```

If it is not enabled in the actual bootloader, automatic `PENDING_VERIFY -> rollback` semantics must **not** be claimed.

Current Example 20 scope is therefore:

```text
A/B inactive-slot OTA       IMPLEMENTED
HTTPS certificate check     IMPLEMENTED
manifest profile check      IMPLEMENTED
SHA-256 verification        IMPLEMENTED
slot activation + reboot    IMPLEMENTED
bootloader auto-rollback    SEPARATE GATE
```

A later hardening step can move the Arduino application into a build environment where the bootloader is rebuilt with ESP-IDF rollback support and then physically exercise a deliberately invalid candidate image.

## Build

From the repository root:

```powershell
arduino-cli compile `
  --fqbn esp32:esp32:esp32s3:PSRAM=enabled,FlashSize=16M `
  --library libraries/WT32_SC01_PLUS `
  libraries/WT32_SC01_PLUS/examples/20_LVGL_GitHubOTA
```

The sketch-local `partitions.csv` is the authoritative OTA layout for this example.

## First physical validation sequence

### Phase A — USB baseline

1. Compile version `0.1.0`.
2. Flash it over USB.
3. Confirm the serial log reports 16 MiB Flash and 2 MiB QSPI PSRAM.
4. Confirm a running slot and a different next OTA slot are reported.
5. Confirm the LVGL OTA UI renders and touch works.
6. Confirm `CHECK GITHUB` does not perform any flash write if no valid manifest is available.

### Phase B — create newer GitHub release

1. Change `WT32_OTA_VERSION` to `0.1.1`.
2. Build/CI-check the new source.
3. Create the `ota-v0.1.1` tag only after the source is ready.
4. Confirm the release contains both OTA assets.
5. Inspect generated manifest size and SHA-256 before using it on hardware.

### Phase C — physical GitHub OTA

On the board still running `0.1.0`:

1. Press `CHECK GITHUB`.
2. Confirm `Available: 0.1.1`.
3. Press `DOWNLOAD & INSTALL`.
4. Observe progress to 100%.
5. Confirm Serial reports expected and calculated SHA-256 values equal.
6. Confirm `SHA256 PASS`.
7. Confirm the board reboots.
8. Confirm firmware reports `VERSION: 0.1.1`.
9. Confirm the running OTA slot changed.
10. Confirm display and touch still work after the update.
11. Press `CHECK GITHUB` again and confirm `UP TO DATE`.

## Current status

```text
SOURCE COMPLETE
VERSIONED FIRMWARE BASELINE CREATED
16 MB DUAL-SLOT OTA LAYOUT ADDED
GITHUB RELEASE PIPELINE ADDED
HTTPS + REDIRECT HANDLING ADDED
SHA-256 STREAM VERIFICATION ADDED
CI TARGET ADDED
PHYSICAL VALIDATION REQUIRED
GITHUB OTA RELEASE: NOT YET PUBLISHED
BOOTLOADER ROLLBACK: SEPARATE VALIDATION GATE
WEB FLASHER: NOT YET ELIGIBLE
```

Do not promote this example to PHYSICAL PASS or add it to the Web Flasher catalog until the complete `0.1.0 -> newer release` GitHub OTA cycle has run successfully on the named Panlee specimen.
