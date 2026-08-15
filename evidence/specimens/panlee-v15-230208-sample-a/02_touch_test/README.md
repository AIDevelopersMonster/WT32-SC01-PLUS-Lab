# 02_touch_test — physical validation protocol

**Status:** `PASS — RAW TOUCH PATH`  
**Acceptance stage:** `HW-03`  
**Orientation/calibration:** `PENDING`  

**Specimen:** `panlee-v15-230208-sample-a`  
**Board marking:** Panlee / `ZX3D50CE08S-V15-USRC` / `230208`  
**Physical run date:** 2026-08-15  
**Test project:** [`examples/02_touch_test`](../../../../examples/02_touch_test/)  
**ESP-IDF:** `v6.0.2`

## Purpose

This run independently validates the physical capacitive-touch read path on the reference specimen without relying on the vendor factory application.

The test verifies:

- touch I2C wiring on GPIO6/GPIO5;
- touch interrupt input on GPIO7;
- shared touch/LCD reset on GPIO4;
- controller response at I2C address `0x38` after reset release;
- FT6336U-compatible identity signature;
- raw touch-coordinate acquisition;
- repeated touch/release operation without I2C read errors.

It does **not** yet certify the transform from the controller's native coordinate system to the LCD's 480x320 landscape coordinate system.

## Test configuration

```text
I2C controller : I2C1
SDA            : GPIO6
SCL            : GPIO5
INT            : GPIO7
RST            : GPIO4, shared with LCD reset
I2C frequency  : 400000 Hz
I2C address    : 0x38
Observation    : 30000 ms
```

The display was not initialized during this raw-path run.

No touch-controller configuration registers were written. GPIO4 was driven only for the explicit reset-recovery sequence.

## Discovery before reset

Immediately after application startup, the touch controller did not respond:

```text
[I2C SCAN - BEFORE RESET]
  No responding addresses detected by address-only probe

[DIRECT REGISTER READ @ 0x38]
  0xA0 CIPHER_LOW / chip code : read failed
  0xA3 CIPHER_HIGH             : read failed
  0xA6 firmware ID             : read failed
  0xA8 FocalTech ID            : read failed
```

This establishes that the independent test must actively release the shared reset path before touch communication can be relied upon on this specimen.

## Shared reset recovery

The test then drove GPIO4 using the declared active-low recovery sequence:

```text
HIGH -> LOW 20 ms -> HIGH -> wait 200 ms
```

GPIO4 is shared by the LCD and touch reset signals on this board family. Because the LCD was not initialized in this test, the diagnostic reset pulse did not disturb an active display session.

After the reset sequence:

```text
[I2C SCAN - AFTER GPIO4 RESET]
  ACK at 0x38
```

Thus GPIO4 reset release is directly demonstrated as necessary for the tested independent touch startup sequence.

## Controller signature

Read-only register access at `0x38` then succeeded:

```text
0xA0 CIPHER_LOW / chip code : 0x02
0xA3 CIPHER_HIGH             : 0x64
0xA6 firmware ID             : 0x03
0xA8 FocalTech ID            : 0x11
```

The external FT6336U reference driver used during investigation identifies `0xA0 == 0x02` as its FT6336U chip code.

Therefore this physical run supports the classification:

```text
FT6336U-compatible signature: MATCH
```

This is strong register-level evidence for an FT6336U-compatible controller path, but the exact physical IC marking has not been read from the assembled specimen. The evidence is intentionally worded as **FT6336U-compatible** rather than claiming that package identity has been visually established.

## Raw touch result

Physical touches generated repeated point-count transitions and raw coordinate samples. Examples included:

```text
points=0
points=1
touch raw: x=104 y=247
touch raw: x=175 y=202
touch raw: x=265 y=51
touch raw: x=319 y=424
touch raw: x=35  y=433
touch raw: x=61  y=140
```

The complete 30-second run reported:

```text
Direct register path     : PASS
FT6336U reference code   : MATCH
Samples with touch       : 37
I2C read errors          : 0
Observed raw X range     : 35 .. 319
Observed raw Y range     : 51 .. 433
```

Final application verdict:

```text
I2C touch read path      : PASS CANDIDATE
RESULT: TOUCH RAW READ PATH PASS CANDIDATE
END 02_touch_test
```

The application returned normally from `app_main()`.

## Physical PASS conclusion

**HW-03 raw touch path: PASS for `panlee-v15-230208-sample-a`.**

Directly validated:

- TP SDA GPIO6;
- TP SCL GPIO5;
- TP INT GPIO7 is electrically readable and changes state during touch activity;
- shared TP/LCD reset GPIO4 successfully releases the touch controller using an active-low pulse;
- I2C address `0x38` responds after reset;
- direct read-only controller-register access works at 400 kHz;
- register `0xA0` returns `0x02`, matching the investigated FT6336U reference signature;
- raw touch point count changes on touch/release;
- raw X/Y coordinate reads respond to physical touch location;
- 37 touch samples were acquired with zero I2C read errors;
- the application completed normally.

## Coordinate-system observation

The observed raw ranges are consistent with a controller coordinate space whose axes are approximately:

```text
raw X: ~0 .. 319
raw Y: ~0 .. 479
```

This is consistent with the native 320x480 panel geometry, while the LCD is used by this project as 480x320 landscape.

That consistency is an observation, not yet a certified coordinate transform. Swap/mirror orientation will be determined independently with a five-target display-assisted test.

## INT interpretation

GPIO7 changed state during touch traffic. Its instantaneous sampled level did not always line up one-for-one with the point-count register because the GPIO level and I2C registers are sampled asynchronously in the polling loop.

Accordingly, this run validates GPIO7 as a responsive touch-related input but does not yet characterize interrupt timing, edge polarity, latency, or ISR behavior.

## Claim boundary

This PASS does **not** yet validate:

- the final `raw -> 480x320 landscape` coordinate transform;
- X/Y swap or mirror flags;
- edge calibration or scale correction;
- multi-touch behavior;
- interrupt-driven operation;
- gesture recognition;
- exact physical package marking of the touch IC;
- every WT32-SC01-PLUS OEM revision.

The next validation step is a display-assisted five-point orientation test using known LCD target coordinates at top-left, top-right, center, bottom-left and bottom-right.
