# WT32_SC01_PLUS Arduino BSP

Experimental Arduino board-support library for the WT32-SC01-PLUS family, developed from physical validation of specific hardware rather than assumed community pinouts.

## Validated target

- Manufacturer marking: **Panlee**
- PCB marking: **ZX3D50CE08S-V15-USRC**
- Lot/date marking: **230208**
- MCU: **ESP32-S3**
- LCD: **ST7796**, 480x320 landscape, 8-bit I80

Factory reverse engineering is used only as hardware evidence for a clean Arduino BSP.

## Arduino IDE installation

The library can be distributed as a standalone ZIP without cloning the full repository.

GitHub Actions builds:

```text
WT32_SC01_PLUS-Arduino-v<version>.zip
```

from exactly:

```text
libraries/WT32_SC01_PLUS
```

Install it in Arduino IDE with:

```text
Sketch -> Include Library -> Add .ZIP Library...
```

The packaging workflow runs on relevant `main` updates and can also be started manually. A tag matching `arduino-v*` publishes the same ZIP as a GitHub Release asset.

Current library version is defined in `library.properties`.

Real Wi-Fi credentials are never packaged: `examples/08_WiFiTest/wifi_secrets.h` is local-only and explicitly excluded from the generated ZIP.

## Arduino examples

The library contains the diagnostic set plus a compact interactive demonstration:

```text
01_DisplayTest
02_TouchTest
03_StorageTest
04_SDDestructiveTest
05_AudioTest
06_RS485Test
07_IOTest
08_WiFiTest
09_BLETest
10_TestConsole
11_RainbowTouch
```

`10_TestConsole` is the combined modular launcher with Serial CLI and touch GUI. The individual examples remain independent deep/qualification tests.

`11_RainbowTouch` is intentionally different: it is a small application-level demonstration rather than a qualification test. It initializes the BSP display/backlight and touch subsystems, then lets the user paint by dragging a finger over the screen. The trail color is derived directly from the touch coordinates with three simple Arduino `map()` operations and packed into RGB565. No LovyanGFX configuration, LCD pin table, touch pin table, HSV conversion, palette, or lookup table is required in the sketch.

Use **ESP32S3 Dev Module**. Example directories contain `sketch.yaml`; host-specific COM numbers are intentionally not stored.

Combined-console video evidence:

[YouTube Shorts — WT32-SC01-PLUS 10_TestConsole combined test](https://youtube.com/shorts/vCfhNmuI3KY)

## v0.1 status

| Subsystem | Status | Notes |
|---|---|---|
| Board identity | VALIDATED | Panlee V15 / 230208 specimen |
| LCD | **PHYSICAL PASS** | ST7796, 480x320, I80 |
| Backlight | **PHYSICAL PASS** | PWM brightness accepted |
| Touch | **PHYSICAL PASS** | FT6336U-compatible, five-point Arduino test passed |
| SD read path | **PHYSICAL PASS** | SDSPI GPIO39/40/38/41 @ 10 MHz; FAT mount + raw/file reads |
| SD full-media write/verify | **PHYSICAL PASS** | Autonomous 8 GB qualification: full 0x00/0xAA/0x55 write + readback, FAT restored |
| SD media anomaly | **WARNING (separate card)** | Earlier ~52 GB-class card reported contradictory raw/FAT geometry; not a board failure |
| Audio | **PHYSICAL PASS** | I2S GPIO35/36/37; full high-power run completed |
| Native USB Serial with audio | **PHYSICAL PASS** | Continuous Serial heartbeat through I2S stress |
| External IO | **PHYSICAL PASS** | GPIO10/11/12/13/14/21 one-hot input validation |
| Wi-Fi | **PHYSICAL PASS** | Scan + association + DHCP + DNS + TCP/HTTP + reconnect |
| BLE | **PHYSICAL PASS** | Scan + advertise + connect + GATT PING/PONG |
| RS485 | PENDING | Dedicated test included; external peer validation pending |
| Combined TestConsole | AVAILABLE | Modular CLI + touch-GUI launcher |
| RainbowTouch demo | SOURCE ADDED / PHYSICAL RUN PENDING | Interactive BSP example; not yet promoted to physical pass |

## Validated LCD mapping

| Signal | GPIO |
|---|---:|
| BL | 45 |
| RST | 4 |
| DC | 0 |
| WR | 47 |
| CS | tied / unused (-1) |
| TE | 48 |
| D0..D7 | 9, 46, 3, 8, 18, 17, 16, 15 |

Display: **480x320**, RGB565, 8-bit I80, **10 MHz**.

## Validated touch mapping

| Signal | GPIO |
|---|---:|
| SDA | 6 |
| SCL | 5 |
| INT | 7 |
| RST | 4 (shared with LCD reset) |

`Wire1`, address `0x38`, 400 kHz. Observed FT6336U-compatible identity: chip `0x02`, firmware `0x03`, FocalTech `0x11`.

Validated landscape mapping:

```text
LCD_X = raw_Y
LCD_Y = 319 - raw_X
```

## Validated SD path

| Signal | GPIO |
|---|---:|
| SCK | 39 |
| MOSI | 40 |
| MISO | 38 |
| CS | 41 |

### Read-only validation

The Arduino `03_StorageTest` physically passed on the reference specimen at **10 MHz**. It validated:

- SDHC initialization and Arduino `SD` FAT mount;
- raw sector 0 read;
- root directory enumeration;
- file readback without modifying the card.

The earlier card used for this run reported contradictory raw/FAT geometry. That warning is retained as media-specific evidence and is not treated as a board-path failure.

### Autonomous full-media qualification

The Arduino `04_SDDestructiveTest` subsequently completed a full destructive qualification on a separate 8 GB-class card.

The test runs autonomously on the WT32-SC01-PLUS using the LCD and touch interface. It uses 64-sector / 32 KiB multi-sector transfers at **10 MHz** and performs:

```text
1/7  full-card WRITE  0x00
2/7  full-card VERIFY 0x00
3/7  full-card WRITE  0xAA
4/7  full-card VERIFY 0xAA
5/7  full-card WRITE  0x55
6/7  full-card VERIFY 0x55
7/7  FAT restore + probe-file write/read/delete
```

Physical final screen:

```text
PASS
00 AA 55 VERIFIED
FAT RESTORED
CARD EMPTY AND READY
7680 MiB / 15728640 SECTORS
```

This promotes the named specimen's SD write path to **PHYSICAL PASS** for the tested 8 GB card at 10 MHz.

Evidence:

[`evidence/specimens/panlee-v15-230208-sample-a/03_storage_test/arduino-sd-destructive-full-pass-8gb.jpg`](../../evidence/specimens/panlee-v15-230208-sample-a/03_storage_test/arduino-sd-destructive-full-pass-8gb.jpg)

The test does not certify every SD-card model, maximum SDSPI clock, card-detect/write-protect behavior, or all WT32-SC01-PLUS OEM revisions.

## Validated audio mapping

| Signal | GPIO |
|---|---:|
| LRCK / WS | 35 |
| BCLK | 36 |
| DOUT | 37 |

`05_AudioTest` physically passed the 20–100% amplitude ramp, 15 s sustained 90% load, repeated 100% bursts, native USB Serial coexistence, and I2S deinit without observed reboot, panic, watchdog or brownout during the controlled audio qualification run.

## Hardware profile warning

The pin mapping is validated only for the Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen/profile. Do not assume every WT32-SC01-PLUS OEM revision is identical.

## Safety boundary

Normal BSP examples avoid factory fixture-only/destructive operations. `04_SDDestructiveTest` is intentionally separate because it overwrites the entire inserted card. It requires on-device operator confirmation before the destructive sequence begins.
