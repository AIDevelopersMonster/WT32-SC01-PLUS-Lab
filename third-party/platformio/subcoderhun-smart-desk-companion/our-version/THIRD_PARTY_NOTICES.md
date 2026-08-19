# Third-Party Notices — WT32-SC01-PLUS Panlee

This derivative work is based on `SubCoderHUN/WT32-SC01-PLUS`.

## Upstream application

- Project: `SubCoderHUN/WT32-SC01-PLUS`
- License: Apache License 2.0
- Use: source baseline for the clock / weather / radio / LVGL application
- Required practice: preserve the Apache-2.0 license, copyright/attribution notices, and mark modified files in distributed derivatives.

Suggested attribution:

```text
Based on SubCoderHUN/WT32-SC01-PLUS.
Original project licensed under Apache License 2.0.
Modified for Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC.
```

## LVGL

- License: MIT
- Keep the upstream license notice when redistributing source or vendored code.

## LovyanGFX

- Main library uses a permissive BSD-family license.
- The repository also contains third-party components/assets with their own notices.
- If `lib/LovyanGFX/` is vendored, keep its complete LICENSE / credits / third-party notices rather than copying isolated source files without metadata.

## ESP32-audioI2S

- License: GPL-3.0
- This is the main license gate for a closed-source commercial firmware.
- For research/open-source use, comply with GPL-3.0 distribution obligations.
- For a proprietary product, re-evaluate the audio layer or obtain appropriate legal/licensing advice.

## SquareLine Studio

- SquareLine is a tool with plan-dependent licensing.
- The generated UI code and the right to use the tool commercially should be checked against the plan used for editing/export.

## OpenWeatherMap

- Service/API terms apply separately.
- Use your own API key.
- Do not publish another person's key found in an upstream source tree.
- Keep secrets out of Git.

## Assets

Do not assume every font, icon, image, weather asset, or 3D file is automatically covered by the top-level Apache-2.0 license. Preserve provenance and review third-party asset notices before commercial redistribution.

This file is an engineering license inventory, not legal advice.

## License files checked

The following upstream license files were checked before this inventory was updated:

- SubCoderHUN/WT32-SC01-PLUS `LICENSE` at `df8c3f251ee2d9fe8ab0961343251661d1c10e40` — Apache-2.0;
- LVGL `LICENCE.txt` at `v8.3.6` — MIT;
- LovyanGFX `license.txt` at `1.1.7` — bundled MIT/BSD-family and component-specific notices;
- ESP32-audioI2S upstream `LICENSE` — GPL-3.0.

Recheck the exact vendored snapshots and all asset notices before a release. This inventory is not legal advice.
