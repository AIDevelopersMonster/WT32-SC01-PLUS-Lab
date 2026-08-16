# WT32_SC01_PLUS Arduino BSP

Experimental Arduino board-support library for the WT32-SC01-PLUS family, developed from physical validation of specific hardware rather than assumed community pinouts.

## Validated target

- Manufacturer marking: **Panlee**
- PCB marking: **ZX3D50CE08S-V15-USRC**
- Lot/date marking: **230208**
- MCU: **ESP32-S3**
- LCD: **ST7796**, 480x320 landscape, 8-bit I80

Factory reverse engineering is used only as hardware evidence for a clean Arduino BSP.

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
| External IO diagnostic | **PHYSICAL TEST VALIDATED** | `07_IOTest`; GPIO10/11/12/13/14/21 independently detected as stable one-hot inputs |
| RS485 | PENDING | Arduino `06_RS485Test` prepared; external adapter required for physical round-trip validation |
| Combined SelfTest | PENDING | Next integration stage built only from individually validated paths |

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

Video evidence showing completion and the final green PASS screen:

[YouTube Shorts — WT32-SC01-PLUS SD full qualification PASS](https://youtube.com/shorts/HVje_STZrCI)

The test does not certify every SD-card model, maximum SDSPI clock, card-detect/write-protect behavior, or all WT32-SC01-PLUS OEM revisions.

## Validated audio mapping

| Signal | GPIO |
|---|---:|
| LRCK / WS | 35 |
| BCLK | 36 |
| DOUT | 37 |

`05_AudioTest` physically passed the 20–100% amplitude ramp, 15 s sustained 90% load, repeated 100% bursts, native USB Serial coexistence, and I2S deinit without observed reboot, panic, watchdog or brownout.

## External IO diagnostic

Factory-firmware analysis recovered the six production `IO Test` inputs:

```text
GPIO10 GPIO11 GPIO12 GPIO13 GPIO14 GPIO21
```

Arduino `07_IOTest` configures all six as `INPUT_PULLDOWN` and requires a stable strict one-hot HIGH state before accepting a channel.

Physical diagnostic validation was completed on the reference board. Explicit PASS events were observed for all six GPIOs across two manual probing runs:

```text
GPIO10 PASS
GPIO11 PASS
GPIO12 PASS
GPIO13 PASS
GPIO14 PASS
GPIO21 PASS
```

One incidental brownout reset occurred while manually moving probes on the small 1.25 mm connector. The test restarted normally and no GPIO detection or test-logic failure was observed. Because every channel independently produced its expected stable one-hot PASS event, `07_IOTest` is classified as **PHYSICAL TEST VALIDATED**.

This status primarily certifies the Arduino diagnostic logic and recovered BSP pin mapping on real hardware. It does not claim 5 V tolerance, output-drive capability or reproduction of the original factory fixture.

Detailed protocol: [`examples/07_IOTest/README.md`](examples/07_IOTest/README.md).

## Arduino IDE

Validated examples:

- `01_DisplayTest`
- `02_TouchTest`
- `03_StorageTest`
- `04_SDDestructiveTest`
- `05_AudioTest`
- `07_IOTest` — physical diagnostic validation complete

Prepared / awaiting external fixture:

- `06_RS485Test` — requires an external RS485 peer/USB-RS485 adapter

Use **ESP32S3 Dev Module**. Example directories contain `sketch.yaml`; host-specific COM numbers are intentionally not stored.

## Hardware profile warning

The pin mapping is validated only for the Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen/profile. Do not assume every WT32-SC01-PLUS OEM revision is identical.

## Safety boundary

Normal BSP examples avoid factory fixture-only/destructive operations. `04_SDDestructiveTest` is intentionally separate because it overwrites the entire inserted card. It requires on-device operator confirmation before the destructive sequence begins. `07_IOTest` is input-only and must use a known-safe 3.3 V stimulus; the Extended I/O connector `+` rail must not be used as a GPIO stimulus source.