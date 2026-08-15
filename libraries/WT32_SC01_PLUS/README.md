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
| SD media integrity | **WARNING** | Tested card reports contradictory raw/FAT capacity; not a board failure |
| SD write/full-media test | PENDING | Must be a separately armed destructive test |
| Audio | **PHYSICAL PASS** | I2S GPIO35/36/37; full high-power run completed |
| Native USB Serial with audio | **PHYSICAL PASS** | Continuous Serial heartbeat through I2S stress |
| RS485 | PENDING | Not yet promoted into BSP |
| Combined SelfTest | PENDING | Built only from individually validated drivers |

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

## Validated SD read path

| Signal | GPIO |
|---|---:|
| SCK | 39 |
| MOSI | 40 |
| MISO | 38 |
| CS | 41 |

The Arduino `03_StorageTest` physically passed on the reference specimen at **10 MHz**. It validated:

- SDHC initialization and Arduino `SD` FAT mount;
- raw capacity report: `106496000` sectors × 512 bytes (~52000 MiB);
- raw sector 0 read with signature `0xAA55`;
- root directory enumeration;
- readback of existing `FOO.TXT` (10 bytes) with checksum `0x65D91DDD`;
- no write operation performed by the test.

The same card reports FAT total space of about **61423 MiB**, greater than its raw card-reported capacity (~52000 MiB). This is retained as a **media geometry warning**. The board-side SDSPI read path is PASS, while the card's true capacity/integrity is not certified.

A full write/read qualification must be a separate explicitly destructive test because it destroys the partition table, filesystem and all files.

## Validated audio mapping

| Signal | GPIO |
|---|---:|
| LRCK / WS | 35 |
| BCLK | 36 |
| DOUT | 37 |

`05_AudioTest` physically passed the 20–100% amplitude ramp, 15 s sustained 90% load, repeated 100% bursts, native USB Serial coexistence, and I2S deinit without observed reboot, panic, watchdog or brownout.

## Arduino IDE

Validated examples:

- `01_DisplayTest`
- `02_TouchTest`
- `03_StorageTest`
- `05_AudioTest`

Use **ESP32S3 Dev Module**. Example directories contain `sketch.yaml`; host-specific COM numbers are intentionally not stored.

## Hardware profile warning

The pin mapping is validated only for the Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen/profile. Do not assume every WT32-SC01-PLUS OEM revision is identical.

## Safety boundary

Normal BSP examples avoid factory fixture-only/destructive operations. Any full-media SD overwrite test must live in a separately named destructive example and require explicit operator arming before the first write.