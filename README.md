# WT32-SC01-PLUS Lab

Practical hardware/software laboratory for the **WT32-SC01-PLUS** family of ESP32 display modules.

This repository follows the successful organization of [`esp32-2432s028-lab`](https://github.com/AIDevelopersMonster/esp32-2432s028-lab), but adds stricter separation between **claims**, **board variants**, and **measured evidence**.

## Have a WT32-SC01-PLUS? Report your board

We are building a map of real WT32-SC01-PLUS hardware variants rather than assuming that every board sold under the same name is identical.

**No Git knowledge is required.** Windows users can run the supplied passive audit script, photograph their board, and submit the results through a GitHub Issue.

Start here:

- [`docs/VISITOR-HARDWARE-VALIDATION.md`](docs/VISITOR-HARDWARE-VALIDATION.md) — step-by-step visitor guide.
- [`tools/windows/wt32-sc01-plus-audit.ps1`](tools/windows/wt32-sc01-plus-audit.ps1) — read-only Windows host/USB audit with optional chip/flash identification.
- [`docs/software/README.md`](docs/software/README.md) — software tools: what each tool is for and where to download it.
- [`docs/research/wt32-sc01-plus-project-landscape-2026-08-13.md`](docs/research/wt32-sc01-plus-project-landscape-2026-08-13.md) — research snapshot of public WT32-SC01 Plus projects, stacks, complexity and reuse value.
- [`docs/videos.md`](docs/videos.md) — project videos and YouTube Shorts grouped by subsystem.
- [`evidence/specimens/panlee-v15-230208-sample-a/hw01-chip/factory-flash-analysis.md`](evidence/specimens/panlee-v15-230208-sample-a/hw01-chip/factory-flash-analysis.md) — verified HW-01 chip/memory/factory-flash analysis for the reference specimen.
- **Issues -> New issue -> “Report your WT32-SC01-PLUS board”** — attach photos and the generated `.txt` report.

The normal reporting workflow does not erase or write flash. Contributors should still review public attachments for MAC addresses or other identifiers before posting.

## Our reference specimen

The first physical specimen in this lab is marked:

- **Panlee**
- **ZX3D50CE08S-V15-USRC**
- **230208**

These markings are treated as specimen evidence, not as proof that every WT32-SC01-PLUS uses the same PCB, display, touch controller, pinout, memory configuration, or peripherals.

### Verified HW-01 memory identity

For this specimen, direct tool measurements currently establish:

- ESP32-S3 QFN56 revision v0.2;
- 40 MHz crystal;
- **16 MiB SPI Flash**;
- Flash ID `0x5E:0x4018`, Quad, 3.3 V;
- **2 MiB embedded PSRAM (`AP_3v3`)**;
- two complete 16 MiB factory-flash reads with identical SHA-256.

Verified factory-image SHA-256:

```text
3772C1BF7D6D2B713973212DDF5C671E3C844A13A8464F675343D9AED4E7F044
```

See the full analysis in [`evidence/specimens/panlee-v15-230208-sample-a/hw01-chip/factory-flash-analysis.md`](evidence/specimens/panlee-v15-230208-sample-a/hw01-chip/factory-flash-analysis.md).

## Factory-test reverse-engineering milestone

The original factory firmware of the reference specimen has now been reverse-engineered far enough to reconstruct the hidden production-test structure, identify the native service channel, and physically execute several original diagnostics through JTAG without reflashing the board.

**Canonical factory-test README:**

- [`tools/factory-test/README.md`](tools/factory-test/README.md) — recovered test order, FactoryCTL usage, physical validation results, RS-485 entry protocol, exact observed boot probe, and the current hypothesis for the complete stock factory workflow.

Detailed evidence:

- [`evidence/specimens/panlee-v15-230208-sample-a/factory-mode-reverse-engineering.md`](evidence/specimens/panlee-v15-230208-sample-a/factory-mode-reverse-engineering.md)
- [`evidence/specimens/panlee-v15-230208-sample-a/uart-protocol-command-map.md`](evidence/specimens/panlee-v15-230208-sample-a/uart-protocol-command-map.md)

### What we were able to determine

Recovered fixed factory-runner order:

```text
AUDIO -> DISPLAY -> TOUCH -> IO -> SD -> USB Con/Dis -> OK
```

Recovered stage boundaries:

```text
AUDIO    0x42007060 -> 0x42007063
DISPLAY  0x42007063 -> 0x42007066
TOUCH    0x42007066 -> 0x42007069
IO       0x42007069 -> 0x4200706C
SD       0x4200706C -> 0x4200706F
USB      0x4200706F -> 0x42007072
OK       0x42007072 -> 0x42007075
```

Physically validated on the reference board:

- hidden factory entry using volatile JTAG state only;
- final green `OK` function;
- interactive LCD RGB/grayscale + blue/green/red test;
- 44-target touch-panel test;
- audible playback of the embedded factory MP3;
- deterministic reuse of the DISPLAY -> TOUCH runner boundary;
- OpenOCD Tcl-RPC based diagnostic control.

The recovered IO, SD and USB stages were also statically mapped. IO requires an external six-line one-hot fixture; the USB stage deliberately remuxes GPIO19/GPIO20 and is therefore not run while built-in USB-JTAG is active. SD is documented with a warning that the original factory routine has weaker post-mount error propagation than its apparent file-system test sequence suggests.

### Native factory entry: RS-485

The hidden selector uses a separate service channel rather than the Type-C USB Serial/JTAG connection:

```text
UART1
115200 8N1
RS-485 half-duplex
TX  = GPIO42
RX  = GPIO1
RTS = GPIO2
```

The exact board-generated startup probe was captured live:

```text
AA 55 00 09 00 FF 00 63 5F
```

This is the board -> fixture request. The exact factory-fixture response is **not yet known**. The receive gate requires a decoded selector value `0xFF`, so a valid response must satisfy that condition, but an exact echo of the request is currently only an experimental candidate.

The next phase of this project begins when a USB-RS485 adapter/cable is available. The goal will be to reproduce **native factory entry without JTAG selector injection** and compare the observed behavior with the reconstructed workflow documented in [`tools/factory-test/README.md`](tools/factory-test/README.md).

### Current stopping point

```text
FACTORY TEST STRUCTURE       RECOVERED
OK / DISPLAY / TOUCH / AUDIO PHYSICALLY VALIDATED
RS485 BOOT PROBE             EXACTLY CAPTURED
RS485 FIXTURE REPLY          OPEN
FULL NATIVE FACTORY ENTRY    OPEN
FULL PRODUCTION FIXTURE      OPEN
```

This repository therefore already contains a complete worked example of practical reverse engineering: preserving the factory flash, reconstructing hidden control flow and peripheral tests, validating recovered functions on the physical board, and turning the findings into a repeatable diagnostic tool. Full native production-fixture emulation is intentionally deferred until the required external hardware is available.

## Arduino BSP milestone

A reusable Arduino library now lives in [`libraries/WT32_SC01_PLUS`](libraries/WT32_SC01_PLUS).

The current Arduino example set is:

```text
01_DisplayTest
02_TouchTest
03_StorageTest
04_SDDestructiveTest
05_AudioTest
06_RS485Test
07_IOTest
08_WiFiTest
09_BLETest
10_TestConsole
```

Physical functional validation on the reference specimen has been completed for display, touch, SD read/write, audio, external GPIO, Wi-Fi and BLE. `06_RS485Test` is included but remains pending until an external RS-485 peer is available.

`10_TestConsole` provides a common modular operator interface: individual tests remain separate source modules while the console can launch them by number from Serial CLI or from the touch-screen GUI.

Combined-test video evidence:

- [YouTube Shorts — WT32-SC01-PLUS Arduino 10_TestConsole](https://youtube.com/shorts/vCfhNmuI3KY)

### Arduino library package

The repository includes an automatic packaging workflow at [`.github/workflows/arduino-library-package.yml`](.github/workflows/arduino-library-package.yml).

It builds an Arduino-installable archive from `libraries/WT32_SC01_PLUS`. Tagging a version as `arduino-v*` publishes the ZIP as a GitHub Release asset. The package intentionally excludes local `wifi_secrets.h` credentials.

Install the release ZIP in Arduino IDE using:

```text
Sketch -> Include Library -> Add .ZIP Library...
```

## Repository philosophy

1. **Identify before assuming.** Every physical board gets a passport.
2. **Separate reported specs from verified facts.** Datasheet/vendor claims go to `docs/`; measurements and dumps go to `evidence/`.
3. **Keep examples incremental.** Start with identity/serial, then display, touch, storage, audio, networking and expansion I/O.
4. **Represent hardware variants explicitly.** OEM/manufacturer/revision differences belong in `config/board_profiles/` and `docs/board-variants/`.
5. **Never convert an untested example into a PASS by documentation alone.** Hardware status must come from a real run.

## Start here

- [`docs/HARDWARE-ACCEPTANCE-START.md`](docs/HARDWARE-ACCEPTANCE-START.md) — acceptance workflow.
- [`docs/VISITOR-HARDWARE-VALIDATION.md`](docs/VISITOR-HARDWARE-VALIDATION.md) — how visitors can report their own board.
- [`docs/board-passports/README.md`](docs/board-passports/README.md) — how to register a specimen.
- [`docs/hardware/01-hardware-overview.md`](docs/hardware/01-hardware-overview.md) — current hardware knowledge and unknowns.
- [`docs/pinout.md`](docs/pinout.md) — pinout working document.
- [`docs/software/README.md`](docs/software/README.md) — software/toolchain index with official download links.
- [`docs/research/wt32-sc01-plus-project-landscape-2026-08-13.md`](docs/research/wt32-sc01-plus-project-landscape-2026-08-13.md) — dated research report on the WT32-SC01 Plus project ecosystem.
- [`docs/videos.md`](docs/videos.md) — project videos and YouTube Shorts.
- [`tools/factory-test/README.md`](tools/factory-test/README.md) — recovered stock factory test and FactoryCTL.
- [`evidence/specimens/panlee-v15-230208-sample-a/README.md`](evidence/specimens/panlee-v15-230208-sample-a/README.md) — specimen-specific acceptance evidence.
- [`libraries/WT32_SC01_PLUS/README.md`](libraries/WT32_SC01_PLUS/README.md) — Arduino BSP status, examples and installation.
- [`examples/README.md`](examples/README.md) — test-sequence notes.

## Structure

```text
WT32-SC01-PLUS-Lab/
├── .github/
│   ├── ISSUE_TEMPLATE/
│   └── workflows/
├── config/
│   └── board_profiles/       # machine-readable profiles per variant/specimen
├── docs/
│   ├── board-passports/      # specimen identity records
│   ├── board-variants/       # OEM/revision comparison
│   ├── hardware/             # subsystem documentation
│   ├── milestones/           # staged lab progress
│   ├── research/             # dated ecosystem/reference-project research
│   └── software/             # framework/toolchain notes
├── evidence/
│   ├── specimens/            # hashes, measurements and acceptance evidence
│   └── README.md
├── examples/                 # reproducible hardware tests
├── hardware/
│   └── images/               # own board photos / annotated images
├── libraries/                # reusable board support code
├── src/                      # future integrated PlatformIO diagnostic app
├── tools/                    # host-side probes and evidence tooling
├── platformio.ini            # intentionally conservative bootstrap
└── README.md
```

## Planned acceptance sequence

| Stage | Target | Current status |
|---|---|---|
| HW-00 | Photograph and identify the specimen | IN PROGRESS |
| HW-01 | Chip / flash / PSRAM / factory backup | **PASS** |
| HW-02 | Display bus and controller | **PASS — factory-fw/JTAG + Arduino verified** |
| HW-03 | Touch controller and coordinates | **PASS — factory 44-target + Arduino five-point verified** |
| HW-04 | Backlight / buttons / onboard I/O | **PARTIAL — backlight PASS; button coverage not closed** |
| HW-05 | Storage | **PASS — Arduino SDSPI read + full-media write/verify + FAT restore** |
| HW-06 | Audio | **PASS — factory-fw/JTAG + Arduino I2S verified** |
| HW-07 | Wi-Fi / BLE | **PASS — Arduino scan + Wi-Fi infrastructure + BLE GATT verified** |
| HW-08 | Expansion connectors / exposed GPIO / RS485 | **PARTIAL — six GPIO inputs PASS; RS485 peer test PENDING** |
| HW-09 | Integrated diagnostic UI / coexistence | **PARTIAL — modular Arduino 10_TestConsole implemented and physically exercised; dedicated LVGL/stress qualification remains open** |

## Important

The repository remains conservative about cross-board defaults. Pin assignments, controller identity and peripheral behavior verified on the **Panlee ZX3D50CE08S-V15-USRC 230208** specimen must not be generalized automatically to every WT32-SC01-PLUS/OEM revision.

For the reference specimen, display and touch are now verified through both the recovered factory workflow and independent Arduino diagnostics. Storage has progressed beyond the recovered factory path to independent Arduino read validation and a complete destructive full-media write/verify qualification on a separate card, followed by FAT restoration.

Audio is physically verified through both the recovered factory firmware path and the Arduino I2S test. External GPIO10/11/12/13/14/21, Wi-Fi and BLE have also completed independent Arduino functional validation.

RS-485 remains the principal unclosed external-interface item: the pin mapping and factory configuration are recovered and an Arduino peer-test sketch is present, but end-to-end physical acceptance awaits an external RS-485 adapter/peer.

## License

Code and original text are intended for release under the MIT License. Third-party schematics, photos, firmware and vendor files retain their original licenses and should not be copied into the repository without checking redistribution rights.
