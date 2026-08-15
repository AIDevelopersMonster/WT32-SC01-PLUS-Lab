# 02_touch_test

Read-only touch discovery and raw-coordinate validation firmware for the reference **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** specimen.

**Current status:** `READY FOR BUILD / PHYSICAL VALIDATION PENDING`

## Evidence basis

Factory-firmware reverse engineering already established that the physical specimen has a working touch path and that the factory test tracks coordinates across 44 target regions. The factory image also contains FT5x06-family driver strings, but that does **not** by itself prove the exact controller model.

Known board-family touch wiring used by this first independent test:

| Signal | GPIO |
|---|---:|
| TP SDA | 6 |
| TP SCL | 5 |
| TP INT | 7 |
| TP RST | 4 |

GPIO4 is shared with LCD reset. This first touch test therefore deliberately does **not** drive the reset line and does not initialize the LCD.

## Test philosophy

The first independent touch test is intentionally conservative:

1. initialize I2C at 100 kHz on GPIO6/GPIO5;
2. scan the legal 7-bit I2C address range;
3. record whether address `0x38` responds;
4. only when `0x38` ACKs, read a few FT5x06-compatible register locations as non-authoritative hints;
5. poll the FT5x06-compatible touch-count and first-point fields for 30 seconds;
6. print raw coordinates, event bits, track ID and INT state;
7. report observed raw X/Y ranges;
8. perform no controller-register writes, calibration writes, NVS writes, filesystem writes, Wi-Fi or BLE operations.

Espressif's FT5x06 component uses I2C address `0x38`, 100 kHz, touch-count register `0x02`, first point beginning at `0x03`, and identification/firmware-related fields around `0xA3`, `0xA6` and `0xA8`. This project reads those locations directly but does not instantiate the FT5x06 component because its normal initialization writes controller tuning registers. The goal here is observation before configuration.

## Claim discipline

A successful `0x38` response plus plausible coordinate traffic supports an **FT5x06-compatible read-path** interpretation for this specimen.

It does **not** by itself establish the exact touch-controller part number. Exact model identity remains unresolved until supported by stronger physical or register-level evidence.

This test also does not yet claim the final 480x320 coordinate transform. Raw coordinate ranges are collected first; orientation, swap/mirror rules and calibration are derived only after the physical run.

## Build

From an activated ESP-IDF 6.0.2 shell:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Lab
git fetch origin
git switch --track origin/agent/02-touch-test
cd .\examples\02_touch_test
idf.py fullclean
idf.py build
```

Do not flash until the build completes successfully and its warnings/errors have been reviewed.

## Physical run procedure

After a successful build:

```powershell
idf.py -p COM10 flash monitor
```

During the 30-second observation window:

- touch near all four corners;
- touch near the center;
- drag horizontally and vertically;
- optionally draw a diagonal across most of the panel.

The serial log should show raw `x`, `y`, `event`, `track` and `INT` values.

## PASS candidate criteria

The first-stage raw touch path can be promoted toward PASS when the real specimen demonstrates:

- I2C bus initialization succeeds;
- a stable responding touch address is observed;
- for the FT5x06-compatible path, `0x38` ACKs;
- touching the panel produces changing raw coordinate samples;
- broad panel touches produce a correspondingly broad raw coordinate range;
- no recurring I2C timeout/error storm occurs;
- the program reaches `END 02_touch_test` normally.

If another address responds instead of `0x38`, stop at discovery evidence and investigate the actual controller before reading model-specific registers.

## Safety boundary

The firmware intentionally:

- does not drive shared reset GPIO4;
- does not initialize the LCD;
- does not write touch-controller registers;
- does not store calibration;
- does not use NVS or a filesystem;
- does not initialize SD, audio, RS-485, Wi-Fi or BLE.

## Next stage after physical run

Use the measured raw ranges and corner ordering to determine the specimen's actual transform to the 480x320 landscape coordinate system. Only then add an optional display-overlay / target-grid validation stage.
