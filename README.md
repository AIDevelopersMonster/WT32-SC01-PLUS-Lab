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
- **Issues -> New issue -> “Report your WT32-SC01-PLUS board”** — attach photos and the generated `.txt` report.

The normal reporting workflow does not erase or write flash. Contributors should still review public attachments for MAC addresses or other identifiers before posting.

## Our reference specimen

The first physical specimen in this lab is marked:

- **Panlee**
- **ZX3D50CE08S-V15-USRC**
- **230208**

These markings are treated as specimen evidence, not as proof that every WT32-SC01-PLUS uses the same PCB, display, touch controller, pinout, memory configuration, or peripherals.

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
- [`examples/README.md`](examples/README.md) — planned test sequence.

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
│   └── software/             # framework/toolchain notes
├── evidence/
│   ├── specimens/            # dumps, hashes, measurements and acceptance evidence
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

| Stage | Target | Initial status |
|---|---|---|
| HW-00 | Photograph and identify the specimen | IN PROGRESS |
| HW-01 | Chip / flash / PSRAM / serial identity | TODO |
| HW-02 | Display bus and controller | TODO |
| HW-03 | Touch controller and coordinates | TODO |
| HW-04 | Backlight / buttons / onboard I/O | TODO |
| HW-05 | Storage | TODO |
| HW-06 | Audio | TODO |
| HW-07 | Wi-Fi / BLE | TODO |
| HW-08 | Expansion connectors / exposed GPIO | TODO |
| HW-09 | Integrated LVGL/UI stress test | TODO |

## Important

The repository is intentionally conservative at bootstrap. Pin assignments, display controller, touch controller and memory configuration must be confirmed for the **Panlee ZX3D50CE08S-V15-USRC** specimen before they are promoted into a default board profile.

## License

Code and original text are intended for release under the MIT License. Third-party schematics, photos, firmware and vendor files retain their original licenses and should not be copied into the repository without checking redistribution rights.
