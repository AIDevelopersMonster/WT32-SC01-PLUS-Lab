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

## Version history

### 0.1.0 — Part 1 baseline

Physically exercised over USB on the reference board.

```text
installed on board: 0.1.0
channel: stable
board: panlee-zx3d50ce08s-v15-usrc
```

### 0.1.1 — first GitHub OTA candidate

Published as:

```text
tag: ota-v0.1.1
asset: panlee-github-ota.bin
manifest: panlee-github-ota.json
```

GitHub Actions completed successfully and the permanent `latest/download` manifest resolved correctly. The device physically detected `0.1.1` from GitHub.

The first `DOWNLOAD & INSTALL` physical attempt failed with:

```text
Guru Meditation Error: Core 1 panic'ed
Debug exception reason: Stack canary watchpoint triggered (loopTask)
```

Forensic flash readback established that the entire tested `app1` image-sized range was still erased (`0xFF`). Its SHA-256 was:

```text
9986568ba714104dc25a7fd47df612fec466d9289f7b9178301a2950a719e89f
```

That hash matches exactly 1,274,960 bytes of `0xFF`, proving that no candidate firmware bytes had reached `app1` before the crash.

The published `0.1.1` asset had:

```text
size: 1274960 bytes
SHA-256: c702e53e49e2972f5cfedfb48b84a9673ae47cec0601b76d94d77b149f49310b
ELF SHA256: 0071b32e08374561...
```

The crashing running image reported a different ELF SHA prefix (`6b983111a`), so the board never booted the published `0.1.1` candidate.

Therefore `0.1.1` remains a preserved failed physical candidate. It must not be relabeled as a successful OTA release.

### 0.1.2 — stack-budget fix candidate

The source branch now carries a loop-task stack fix in `ota_stack_config.cpp`.

Arduino-ESP32 creates `setup()` / `loop()` inside `loopTask`; the core default is 8 KiB. The `0.1.1` download path performs TLS, HTTP, SHA-256, LVGL servicing and OTA buffering inside that same task. The fix overrides the weak Arduino stack-size hook and raises the loop-task budget to 16 KiB.

```text
0.1.2 status: SOURCE FIX PREPARED
physical status: NOT YET TESTED
```

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
```

The `404` in Part 1 was an expected boundary result because no OTA Release/manifest existed yet.

## Part 2 physical result — 0.1.1

```text
GitHub Actions / Release        PASS
latest manifest                PASS
manifest HTTPS fetch           PHYSICAL PASS
0.1.1 version detection        PHYSICAL PASS
DOWNLOAD & INSTALL path        ENTERED
loopTask stack canary          PHYSICAL FAIL
candidate bytes in app1        0 bytes observed
SHA-256 verification           NOT REACHED
Update.end()                   NOT REACHED
slot activation                NOT REACHED
boot into 0.1.1                NOT REACHED
full GitHub OTA cycle          NOT PASSED
```

This is intentionally recorded as a failed physical gate rather than hidden or converted into a source-only success.

## Manifest URL

The device checks:

```text
https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab/releases/latest/download/panlee-github-ota.json
```

Manifest fields:

```json
{
  "schema": 1,
  "board": "panlee-zx3d50ce08s-v15-usrc",
  "version": "0.1.x",
  "channel": "stable",
  "size": 1234567,
  "sha256": "64-lowercase-hex-digits",
  "firmware": "https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab/releases/download/ota-v0.1.x/panlee-github-ota.bin"
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

The sketch verifies HTTPS certificates and follows GitHub redirects. Firmware bytes are streamed toward the inactive OTA slot while SHA-256 is calculated. The inactive image is only finalized if the received byte count and SHA-256 match the manifest.

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

The generated build directory was physically inspected during Part 1 and confirmed to contain this exact partition table and a generated `*.partitions.bin`.

The Arduino CLI `96% / 1,310,720 bytes` message is the generic board profile's `upload.maximum_size` reporting value. It does not replace the sketch-local 16 MiB A/B partition table.

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

## Next validation sequence

The failed `0.1.1` release is preserved as evidence. The next clean validation sequence is:

```text
USB flash fixed baseline 0.1.2
        ↓
verify UI + CHECK GITHUB stability
        ↓
publish a later OTA candidate
        ↓
DOWNLOAD & INSTALL
        ↓
HTTPS download
        ↓
SHA-256 PASS
        ↓
inactive OTA slot activated
        ↓
reboot into new version
        ↓
running slot changed
        ↓
CHECK GITHUB -> UP TO DATE
```

A future physical PASS requires all of the following:

- manifest is found from the permanent GitHub URL;
- firmware asset downloads successfully through GitHub redirects;
- byte count matches manifest;
- SHA-256 matches manifest;
- update finalizes successfully;
- board reboots into the other OTA slot;
- firmware reports the new version after reboot;
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
SHA-256 verification        IMPLEMENTED IN SOURCE
slot activation + reboot    IMPLEMENTED IN SOURCE
0.1.1 physical OTA          FAILED BEFORE FIRST APP1 WRITE
0.1.2 stack fix             SOURCE PREPARED
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

## Current status

```text
PART 1 BASELINE 0.1.0        PHYSICALLY EXERCISED
PART 1 VIDEO                  DOCUMENTED
EXPECTED PRE-RELEASE 404      OBSERVED
OTA-V0.1.1 RELEASE            PUBLISHED / CI PASS
0.1.0 -> 0.1.1 OTA            PHYSICAL FAIL: loopTask stack overflow
APP1 FORENSIC READBACK        ALL 0xFF / NO CANDIDATE BYTES WRITTEN
SOURCE CANDIDATE 0.1.2        STACK FIX PREPARED
0.1.2 PHYSICAL TEST           REQUIRED
FULL GITHUB OTA               NOT YET PASSED
BOOTLOADER ROLLBACK           SEPARATE VALIDATION GATE
WEB FLASHER                   NOT YET ELIGIBLE
```
