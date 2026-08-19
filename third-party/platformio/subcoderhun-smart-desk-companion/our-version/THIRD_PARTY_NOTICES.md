# Third-party notices and release gates

This is a technical inventory, not legal advice. Verify the exact source snapshot, bundled files, and applicable terms before distributing source or firmware.

| Component/service | Verified source or terms | License/term note |
|---|---|---|
| SubCoderHUN/WT32-SC01-PLUS | Upstream `LICENSE` at snapshot `df8c3f251ee2d9fe8ab0961343251661d1c10e40` | Apache License 2.0; preserve the license and applicable notices and identify modifications. |
| LVGL 8.3.6 | Upstream `LICENCE.txt` at tag `v8.3.6` | MIT; retain copyright and permission notice. |
| LovyanGFX 1.1.7 | Upstream `license.txt` at tag `1.1.7` | Aggregates permissive MIT/BSD-family and component-specific notices; retain the complete file and bundled third-party credits. |
| ESP32-audioI2S | Upstream `LICENSE` | GPL-3.0; redistribution of firmware/source requires a dedicated compliance review. |
| SquareLine Studio | Generated UI/tool workflow | Rights and obligations depend on the tool version, plan, exported assets, and SquareLine terms; verify before redistribution. |
| OpenWeatherMap | External API/service | Use is governed by current API/service terms and the account's plan; never publish API keys. |

## GPL-3.0 commercial gate

`ESP32-audioI2S` is a release gate for any proposed closed-source commercial firmware. Do not assume a binary containing or linking this dependency can remain proprietary. Obtain a project-specific license/compliance assessment or replace the dependency with a suitably licensed implementation before making a closed distribution claim.

## Attribution statement

Based on SubCoderHUN/WT32-SC01-PLUS. Original project licensed under Apache License 2.0. Modified for Panlee WT32-SC01-PLUS / `ZX3D50CE08S-V15-USRC`.
