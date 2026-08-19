# Hardware Acceptance Start

Reference specimen: **Panlee / ZX3D50CE08S-V15-USRC / 230208**.

## Fast path: browser test firmware

For the physically validated Panlee reference specimen, the non-destructive Arduino BSP tests can be installed directly from a desktop Chrome/Edge browser:

**[Open the WT32-SC01-PLUS Lab Web Flasher](https://aidevelopersmonster.github.io/WT32-SC01-PLUS-Lab/)**

The page uses ESP Web Tools to flash the selected test and includes a 115200-baud Web Serial monitor for observing test output and saving logs. No Arduino IDE, PlatformIO or local esptool installation is required for this path.

The browser flasher is intentionally specimen-specific. Do not assume that another WT32-SC01-PLUS/OEM revision uses the same pinout or peripherals. The destructive full-card SD qualification firmware is not exposed through the one-click flasher.

## Rule

The lab distinguishes three levels:

- **OBSERVED** — directly visible marking or physical feature;
- **REPORTED** — vendor/community/datasheet statement not yet confirmed here;
- **VERIFIED** — reproduced on the named physical specimen.

## Sequence

### HW-00 — Physical identity

Record front/back photographs, all readable IC markings, connector labels and PCB revision strings.

### HW-01 — MCU identity

Build and flash the bootstrap probe. Record chip model/revision, flash size, PSRAM result, MAC/eFuse summary where available, and firmware SHA-256.

### HW-02 — Display

Identify bus, controller, reset/backlight behavior, resolution, orientation and stable clock range.

### HW-03 — Touch

Identify controller/interface, interrupt behavior, raw coordinate range, orientation and calibration.

### HW-04 — Onboard I/O

Test buttons, LEDs, backlight control and any onboard sensors.

### HW-05 — Storage

Identify and test SD/TF or other storage without destructive writes first.

### HW-06 — Audio

Identify codec/amplifier/I2S/DAC path and connector behavior.

### HW-07 — Radio

Run Wi-Fi and BLE independently, then coexistence stress testing.

### HW-08 — Expansion

Map exposed connectors and GPIO. Check boot-strapping pins and voltage domains before loopback tests.

### HW-09 — Integrated UI

Only after lower stages pass: run an integrated display + touch + storage + radio + LVGL stress test.

Evidence belongs under `evidence/specimens/<specimen-id>/`.
