# Hardware overview

## Current confirmed facts

For the physical reference specimen we currently accept only the markings directly provided from the board:

- `Panlee`
- `ZX3D50CE08S-V15-USRC`
- `230208`

## Working hypothesis

The project targets a WT32-SC01-PLUS-class ESP32 display module. Exact MCU, display controller, touch controller, memory population and pin mapping remain acceptance items for this specimen.

## Why this matters

Boards sold under one commercial name can change display/touch modules, PCB revisions, flash/PSRAM population, USB implementation and connector mappings. Therefore the lab uses variant profiles rather than one universal hard-coded pin table.

## Documentation map

Future subsystem documents should be added as numbered files:

- `02-pcb-component-atlas.md`
- `03-connector-reference.md`
- `04-power-system.md`
- `05-display-system.md`
- `06-touch-system.md`
- `07-storage-system.md`
- `08-audio-system.md`
- `09-usb-programming-and-serial-system.md`
- `10-expansion-gpio-and-bus-system.md`
- `11-wireless-system.md`
