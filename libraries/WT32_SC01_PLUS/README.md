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
| Audio | **PHYSICAL PASS** | I2S GPIO35/36/37; high-power test completed without controller reset |
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

## Validated audio mapping

| Signal | GPIO |
|---|---:|
| LRCK / WS | 35 |
| BCLK | 36 |
| DOUT | 37 |

The Arduino `05_AudioTest` was physically accepted on the Panlee V15 / 230208 specimen. The test exercised a loudness staircase through 100% PCM amplitude, a sustained 15-second 90% tone, and repeated 100% bursts. Audible output increased as expected and the ESP32-S3 did not reboot or show an observed brownout/watchdog failure during the run.

The test deliberately isolates audio from LCD, touch, Wi-Fi, LVGL, SD and RS485. This makes the result evidence for the onboard I2S/audio path rather than for a combined-system workload.

### Serial diagnostic caveat

The physical high-power audio run did not produce visible `Serial` output on the monitored COM port. The connected port was later identified as the ESP32-S3 native USB interface (`VID_303A`, `PID_1001`). Therefore simultaneous `Serial` diagnostics over native USB are **not yet certified by the audio PASS**; USB CDC routing/settings remain a separate interface-validation task. This does not invalidate the observed audio output or no-reset result.

## Arduino IDE

Copy or junction `libraries/WT32_SC01_PLUS` into your Arduino libraries directory and restart Arduino IDE if necessary.

Validated examples:

- `File -> Examples -> WT32_SC01_PLUS -> 01_DisplayTest`
- `File -> Examples -> WT32_SC01_PLUS -> 05_AudioTest`

For the reference specimen select **ESP32S3 Dev Module**, choose the correct serial port, compile and upload. Example directories also contain `sketch.yaml` metadata with the generic ESP32-S3 FQBN and 115200 monitor baud rate; host-specific COM numbers are intentionally not stored.

Selecting the generic `ESP32 Dev Module` is incorrect for this board and causes `esptool` to reject the ESP32-S3 during upload.

## Hardware profile warning

The pin mapping in `WT32_SC01_PLUS_Pins.h` is for the Panlee `ZX3D50CE08S-V15-USRC / 230208` profile under investigation. Do not assume that every OEM board sold as WT32-SC01-PLUS uses the same mapping.

New pin mappings are first treated as experimental evidence and are promoted into the validated profile only after a physical test on the reference specimen.

## Safety boundary

The Arduino BSP does not reproduce factory-only destructive or fixture-oriented operations. In particular, the factory USB connect/disconnect test that manipulates GPIO19/20 is not part of the normal SelfTest design.
