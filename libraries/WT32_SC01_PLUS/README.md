# WT32_SC01_PLUS Arduino BSP

Experimental Arduino board-support library for the WT32-SC01-PLUS family, developed from physical validation of specific hardware rather than assumed community pinouts.

## Validated target

The first bring-up target is the physically investigated specimen:

- Manufacturer marking: **Panlee**
- PCB marking: **ZX3D50CE08S-V15-USRC**
- Lot/date marking: **230208**
- MCU: **ESP32-S3**
- LCD controller/path: **ST7796**, 480x320 landscape, 8-bit Intel 8080

This is intentionally **not** a copy of the factory firmware. Factory reverse engineering and laboratory tests are used only as hardware evidence for a clean Arduino BSP.

## v0.1 status

| Subsystem | Status | Notes |
|---|---|---|
| Board identity | VALIDATED | Panlee V15 / 230208 specimen |
| LCD | **PHYSICAL PASS** | ST7796, 480x320, I80 |
| Backlight | **PHYSICAL PASS** | PWM brightness test accepted |
| Touch | PENDING | Next validation increment |
| SD | PENDING | Next validation increment |
| Audio | **EXPERIMENTAL TEST READY** | GPIO35/36/37 candidate mapping; physical V15 acceptance required |
| RS485 | PENDING | Not yet promoted into BSP |
| Combined SelfTest | PENDING | Built only from individually validated drivers |

### Display acceptance record

The Arduino `01_DisplayTest` has passed all three gates used for this first BSP increment:

1. **GitHub Actions compile PASS** using Arduino-ESP32 3.3.8.
2. **Local Arduino IDE compile PASS** on the development workstation:
   - program storage: **294821 bytes (22%)** of 1310720 bytes;
   - global variables: **23016 bytes (7%)** of 327680 bytes.
3. **Physical operator PASS** on the Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen: colors, combined test pattern and backlight behavior were accepted.

This promotes the display/backlight slice from experimental bring-up to **validated for this hardware profile**. It does not claim compatibility with every WT32-SC01-PLUS OEM revision.

## Validated LCD mapping

| Signal | GPIO |
|---|---:|
| BL | 45 |
| RST | 4 |
| DC | 0 |
| WR | 47 |
| CS | tied / unused (-1) |
| TE | 48 (unused by v0.1) |
| D0 | 9 |
| D1 | 46 |
| D2 | 3 |
| D3 | 8 |
| D4 | 18 |
| D5 | 17 |
| D6 | 16 |
| D7 | 15 |

Display parameters: **480 x 320**, RGB565, 8-bit I80, **10 MHz** write clock.

## Experimental audio mapping

The current audio experiment uses these candidate I2S pins:

| Signal | GPIO | Status |
|---|---:|---|
| LRCK / WS | 35 | experimental on V15 |
| BCLK | 36 | experimental on V15 |
| DOUT | 37 | experimental on V15 |

The mapping agrees with public WT32-SC01 Plus references, including a Panlee V13 I2S demo, but it is deliberately **not marked validated for the V15/230208 specimen yet**.

`05_AudioTest` is an isolated stability test. It does not initialize LCD, touch, Wi-Fi, LVGL, SD or RS485. It generates controlled PCM tone bursts rather than playing the factory MP3 and continuously reports Serial/heap diagnostics so that USB-Serial loss, panic, watchdog or brownout resets are easier to identify.

Audio physical PASS requires:

1. I2S initializes without reboot;
2. low/medium/normal tone bursts are audible;
3. the 10-second 1 kHz stress tone completes;
4. Serial heartbeat remains continuous during PCM transfer;
5. COM does not disappear because of controller reset;
6. post-test heartbeat continues after I2S deinitialization;
7. if a reset occurs, its reported reset reason is recorded instead of treating the test as PASS.

Only after physical acceptance should GPIO35/36/37 be promoted into the validated Panlee V15 profile.

## Arduino IDE

Copy or junction `libraries/WT32_SC01_PLUS` into your Arduino libraries directory and restart Arduino IDE if necessary.

Validated display example:

`File -> Examples -> WT32_SC01_PLUS -> 01_DisplayTest`

Experimental audio example:

`File -> Examples -> WT32_SC01_PLUS -> 05_AudioTest`

For the reference specimen select **ESP32S3 Dev Module**, choose the correct serial port, compile, upload, and open Serial Monitor at 115200 baud.

Selecting the generic `ESP32 Dev Module` is incorrect for this board and causes `esptool` to reject the ESP32-S3 during upload.

## Display PASS criteria

A physical PASS requires all of the following:

1. black, white, red, green and blue screens are correct;
2. the combined pattern shows clean color bars and a smooth grayscale;
3. geometry is stable with no persistent missing region or gross flicker;
4. backlight visibly changes through 100% -> 10% -> 50% -> 100%;
5. serial output reports successful initialization.

A compile PASS alone is not a hardware PASS.

## Hardware profile warning

The pin mapping in `WT32_SC01_PLUS_Pins.h` is for the Panlee `ZX3D50CE08S-V15-USRC / 230208` profile under investigation. Do not assume that every OEM board sold as WT32-SC01-PLUS uses the same mapping.

New pin mappings are first treated as experimental evidence and are promoted into the validated profile only after a physical test on the reference specimen.

## Safety boundary

The Arduino BSP does not reproduce factory-only destructive or fixture-oriented operations. In particular, the factory USB connect/disconnect test that manipulates GPIO19/20 is not part of the normal SelfTest design.
