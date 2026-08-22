# 20_LVGL_GitHubOTA

Direct GitHub Release OTA update demonstration for the WT32-SC01-PLUS Arduino BSP.

Target specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3
16 MiB Flash
2 MiB QSPI PSRAM
```

## Current status

```text
GITHUB OTA DEMONSTRATION      PHYSICAL PASS
VALIDATED UPDATE PATH         0.1.2 -> 0.1.3
VALIDATION RUNS               PASSED TWICE ON REFERENCE BOARD
MANIFEST FETCH                PASS
HTTPS ASSET DOWNLOAD          PASS
BYTE COUNT CHECK              PASS
SHA-256 CHECK                 PASS
Update.end() / FINALIZE       PASS
INACTIVE SLOT ACTIVATION      PASS
REBOOT INTO NEW VERSION       PASS
RUNNING VERSION CHECK         PASS
CHECK GITHUB -> UP TO DATE    PASS
LVGL DISPLAY + TOUCH AFTER OTA PASS
BOOTLOADER AUTO-ROLLBACK      SEPARATE VALIDATION GATE
```

The full project-scope GitHub OTA task is therefore closed for this example: the board can fetch the GitHub Release manifest, download the firmware asset through HTTPS, verify the byte count and SHA-256, write the inactive OTA slot, finalize the update, reboot into the new version and then report that the installed firmware is up to date.

The only boundary deliberately not claimed here is automatic bootloader rollback. A/B OTA slots do not automatically prove rollback behavior. `PENDING_VERIFY -> rollback` must remain a separate validation gate unless the bootloader is built with rollback support and a deliberate failed-candidate experiment is physically executed.

## Goal

The first firmware installation is performed over USB. After that, the device updates application firmware directly from GitHub Releases over HTTPS without a private LAN OTA server.

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
      | byte count + SHA-256 PASS
      v
activate + reboot
      |
      v
new firmware version running
```

## Version history

### 0.1.0 — Part 1 baseline

Physically exercised over USB on the reference board.

```text
installed on board: 0.1.0
channel: stable
board: panlee-zx3d50ce08s-v15-usrc
```

Observed:

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

The `404` was an expected pre-release boundary result because no OTA Release/manifest existed yet.

Video evidence:

- [YouTube Shorts — WT32-SC01-PLUS GitHub OTA](https://youtube.com/shorts/gVSZsYNjtj4)

### 0.1.1 — failed early candidate, preserved as history

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

Forensic flash readback established that the tested `app1` image-sized range was still erased (`0xFF`). Its SHA-256 was:

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

This failed candidate is retained only as forensic history. It is not the current status of the OTA example and must not be used as the current project verdict.

### 0.1.2 — stack-budget fix baseline

The OTA download path performs TLS, HTTP, SHA-256, LVGL servicing and OTA buffering from the Arduino loop task. Arduino-ESP32 creates `setup()` / `loop()` inside `loopTask`, whose default stack was too small for the 0.1.1 path.

The fix is isolated in `ota_stack_config.cpp` and raises the Arduino loop-task budget to 16 KiB by overriding the weak Arduino hook:

```cpp
size_t getArduinoLoopTaskStackSize(void) {
    return 16U * 1024U;
}
```

`0.1.2` became the corrected USB-flashed baseline used for the successful GitHub OTA validation path.

### 0.1.3 — successful GitHub OTA candidate

The complete GitHub OTA path from `0.1.2` to `0.1.3` passed physically on the reference Panlee specimen. The validation was repeated successfully.

Acceptance items closed by the successful run:

- permanent GitHub `latest/download` manifest resolved;
- manifest schema, board id, channel and semantic version were accepted;
- firmware asset downloaded through GitHub HTTPS redirects;
- received byte count matched the manifest;
- streamed SHA-256 matched the manifest;
- inactive OTA slot write completed;
- update finalization completed;
- boot partition changed to the new slot;
- board rebooted into the new firmware;
- firmware reported the new version after reboot;
- LVGL display and touch remained operational;
- a second GitHub check reported `UP TO DATE`.

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

## Build

From repository root:

```powershell
& $CLI compile `
  --fqbn "esp32:esp32:esp32s3:PSRAM=enabled,FlashSize=16M" `
  --library "libraries/WT32_SC01_PLUS" `
  "libraries/WT32_SC01_PLUS/examples/20_LVGL_GitHubOTA"
```

## Final scope statement

```text
DECLARED GITHUB OTA PATH      CLOSED / PHYSICAL PASS
FAILED 0.1.1 CANDIDATE        HISTORICAL FORENSIC RECORD ONLY
CURRENT SUCCESSFUL PATH       0.1.2 -> 0.1.3
BOOTLOADER AUTO-ROLLBACK      NOT CLAIMED / SEPARATE VALIDATION GATE
SIGNED FIRMWARE AUTHENTICITY  FUTURE HARDENING
```
