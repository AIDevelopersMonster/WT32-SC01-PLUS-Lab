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
| Touch | **PHYSICAL PASS** | FT6336U-compatible, Wire1 @ 400 kHz, five-point landscape test passed |
| SD | PENDING | Next validation increment |
| Audio | **PHYSICAL PASS** | I2S GPIO35/36/37; full high-power run completed |
| Native USB Serial with audio | **PHYSICAL PASS** | Continuous Serial heartbeat through I2S stress and after deinit |
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

## Validated touch mapping

| Signal | GPIO |
|---|---:|
| SDA | 6 |
| SCL | 5 |
| INT | 7 |
| RST | 4 (shared with LCD reset) |

Touch bus parameters:

- `Wire1`
- I2C address `0x38`
- 400 kHz
- FT6336U-compatible identity observed: chip code `0x02`, firmware ID `0x03`, FocalTech ID `0x11`

The Arduino `02_TouchTest` was physically accepted on the Panlee V15 / 230208 specimen using a coherent `TD_STATUS + P1 + P2` frame read and requiring three consecutive hits per target.

Five targets passed:

```text
TOP_LEFT     dx=-20 dy=17
TOP_RIGHT    dx=-6  dy=22
CENTER       dx=1   dy=22
BOTTOM_LEFT  dx=0   dy=30
BOTTOM_RIGHT dx=-5  dy=13
```

The physically validated landscape mapping is:

```text
LCD_X = raw_Y
LCD_Y = 319 - raw_X
```

This certifies the Arduino I2C touch path, FT6336U-compatible identity, coherent point-frame reads and the simple landscape transform for the reference specimen. It is not a precision edge calibration claim and should not be generalized automatically to every WT32-SC01-PLUS OEM revision.

## Validated audio mapping

| Signal | GPIO |
|---|---:|
| LRCK / WS | 35 |
| BCLK | 36 |
| DOUT | 37 |

The Arduino `05_AudioTest` was physically accepted on the Panlee V15 / 230208 specimen. The accepted run exercised:

- a 1 kHz amplitude staircase at 20%, 35%, 50%, 65%, 80%, 95% and 100%;
- a sustained 15-second 1 kHz tone at 90%;
- five repeated 1-second full-scale 100% bursts separated by short silence gaps;
- I2S initialization and deinitialization;
- native USB Serial diagnostics before, during and after I2S activity.

The complete run finished without an observed controller reboot, panic, watchdog reset or brownout. Serial heartbeat remained visible throughout the sustained high-power section and the full-scale burst sequence, continued after I2S deinitialization, and remained stable for more than one minute after the test.

Heap behavior was also stable during the run: free heap was approximately 348172 bytes before I2S initialization, approximately 340952 bytes throughout active playback, and returned to approximately 348140 bytes after I2S deinitialization. This is consistent with temporary I2S allocation being released after the test; no progressive heap loss was observed in the recorded run.

The connected serial device was the ESP32-S3 native USB interface (`VID_303A`, `PID_1001`). Therefore the accepted run certifies simultaneous onboard audio and native USB Serial operation for this specimen under the tested Arduino configuration.

The reset-reason diagnostic may report `ESP_RST_USB` when the board is reset through its USB peripheral path. This is distinct from panic, watchdog and brownout reset reasons.

The test deliberately isolates audio from LCD, touch, Wi-Fi, LVGL, SD and RS485. This makes the result evidence for the onboard I2S/audio path plus native USB Serial coexistence, not yet for a combined-system workload.

## Arduino IDE

Copy or junction `libraries/WT32_SC01_PLUS` into your Arduino libraries directory and restart Arduino IDE if necessary.

Validated examples:

- `File -> Examples -> WT32_SC01_PLUS -> 01_DisplayTest`
- `File -> Examples -> WT32_SC01_PLUS -> 02_TouchTest`
- `File -> Examples -> WT32_SC01_PLUS -> 05_AudioTest`

For the reference specimen select **ESP32S3 Dev Module**, choose the correct serial port, compile and upload. Example directories also contain `sketch.yaml` metadata with the generic ESP32-S3 FQBN and 115200 monitor baud rate; host-specific COM numbers are intentionally not stored.

Selecting the generic `ESP32 Dev Module` is incorrect for this board and causes `esptool` to reject the ESP32-S3 during upload.

## Hardware profile warning

The pin mapping in `WT32_SC01_PLUS_Pins.h` is for the Panlee `ZX3D50CE08S-V15-USRC / 230208` profile under investigation. Do not assume that every OEM board sold as WT32-SC01-PLUS uses the same mapping.

New pin mappings are first treated as experimental evidence and are promoted into the validated profile only after a physical test on the reference specimen.

## Safety boundary

The Arduino BSP does not reproduce factory-only destructive or fixture-oriented operations. In particular, the factory USB connect/disconnect test that manipulates GPIO19/20 is not part of the normal SelfTest design.
