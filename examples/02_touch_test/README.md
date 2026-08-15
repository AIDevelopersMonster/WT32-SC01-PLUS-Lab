# 02_touch_test

Independent touch-controller discovery and raw-coordinate validation firmware for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Current status:** `PASS — RAW TOUCH PATH PHYSICALLY VALIDATED`  
**Orientation/calibration:** `PENDING`

Physical validation protocol:

- [`../../evidence/specimens/panlee-v15-230208-sample-a/02_touch_test/README.md`](../../evidence/specimens/panlee-v15-230208-sample-a/02_touch_test/README.md)

## Validated hardware path

The physical specimen was tested under ESP-IDF 6.0.2 using:

| Signal | GPIO |
|---|---:|
| TP SDA | 6 |
| TP SCL | 5 |
| TP INT | 7 |
| TP RST | 4 |

Runtime configuration:

```text
I2C controller : I2C1
I2C frequency  : 400 kHz
I2C address    : 0x38
```

GPIO4 is shared by LCD reset and touch reset.

## Startup behavior discovered

The touch controller did not respond immediately after application startup. Neither the address scan nor direct register reads at `0x38` succeeded before reset release.

The test then applied an explicit active-low shared-reset sequence on GPIO4:

```text
HIGH -> LOW 20 ms -> HIGH -> wait 200 ms
```

After that sequence:

```text
[I2C SCAN - AFTER GPIO4 RESET]
  ACK at 0x38
```

and direct register reads succeeded.

This establishes an important initialization requirement for this specimen: the independent touch path must release/reset the controller through GPIO4 before reliable I2C communication.

## FT6336U-compatible signature

Read-only register values observed on the physical specimen:

```text
0xA0 CIPHER_LOW / chip code : 0x02
0xA3 CIPHER_HIGH             : 0x64
0xA6 firmware ID             : 0x03
0xA8 FocalTech ID            : 0x11
```

The investigated external FT6336U reference driver uses `0xA0 == 0x02` as its FT6336U chip code. The specimen therefore has a **physically validated FT6336U-compatible register signature**.

The exact physical package marking has not yet been read, so this repository deliberately avoids upgrading that statement to an unconditional package-identification claim.

## Raw touch validation

During the 30-second physical run:

```text
Direct register path     : PASS
FT6336U reference code   : MATCH
Samples with touch       : 37
I2C read errors          : 0
Observed raw X range     : 35 .. 319
Observed raw Y range     : 51 .. 433
```

Example raw samples:

```text
x=104 y=247
x=175 y=202
x=265 y=51
x=319 y=424
x=35  y=433
x=61  y=140
```

The application reached:

```text
RESULT: TOUCH RAW READ PATH PASS CANDIDATE
END 02_touch_test
```

and returned normally from `app_main()`.

Accordingly the lab-level hardware status is promoted to:

```text
HW-03 raw touch path = PASS
```

## What the PASS establishes

For this physical specimen, the test directly validates:

- TP SDA on GPIO6;
- TP SCL on GPIO5;
- responsive touch-related INT input on GPIO7;
- shared reset GPIO4 as an active-low touch reset/release path;
- I2C communication at 400 kHz;
- responding touch address `0x38`;
- FT6336U-compatible identity signature (`0xA0 == 0x02`);
- raw touch count and X/Y reads;
- repeated touch/release operation;
- zero I2C read errors during the measured run.

## Coordinate-system observation

The physical raw samples are consistent with an approximately 320x480 native touch coordinate space:

```text
raw X ~ 0..319
raw Y ~ 0..479
```

The LCD is independently validated in 480x320 landscape mode. Therefore a swap/mirror transform is expected, but its exact form is **not yet claimed**.

## Claim boundary

This PASS does **not** yet establish:

- the exact `raw 320x480 -> LCD 480x320` transform;
- swap/mirror flags;
- edge scale/offset calibration;
- multi-touch correctness;
- interrupt edge polarity or ISR timing;
- gesture support;
- exact physical IC package marking;
- all WT32-SC01-PLUS/OEM revisions.

## Next stage

The next test is a display-assisted five-point orientation validation. It will show targets at known LCD coordinates:

```text
TOP-LEFT
TOP-RIGHT
CENTER
BOTTOM-LEFT
BOTTOM-RIGHT
```

For each target, the firmware will capture a stable raw touch sample. The resulting five correspondences will be used to determine the unique swap/mirror mapping and check whether simple axis scaling is sufficient.

## Build

From an activated ESP-IDF 6.0.2 shell:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Lab
git switch agent/02-touch-test
cd .\examples\02_touch_test
idf.py build
```

Flash/monitor:

```powershell
idf.py -p COM10 flash monitor
```
