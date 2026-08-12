# Contributing

Hardware facts in this repository should be traceable to one of three sources: a physical measurement, a readable PCB/component marking, or a cited external source.

When adding a new board variant, add or update:

1. a board passport under `docs/board-passports/`;
2. a machine-readable profile under `config/board_profiles/`;
3. relevant evidence under `evidence/specimens/`;
4. variant notes under `docs/board-variants/`.

Use status words such as `UNVERIFIED`, `READY FOR HARDWARE TEST`, `PASS`, and `FAIL` explicitly. Do not mark a hardware path as PASS unless it has been tested on a named specimen.
