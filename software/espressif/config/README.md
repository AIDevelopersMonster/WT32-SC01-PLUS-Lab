# ESP-IDF configuration profiles

This directory contains minimal, reviewable `sdkconfig.defaults` profiles for known WT32-SC01-PLUS specimens.

## Current profile

`panlee-v15-230208-sample-a.idf6.0.2.sdkconfig.defaults`

Validated against:

- specimen: `panlee-v15-230208-sample-a`;
- marking: `Panlee / ZX3D50CE08S-V15-USRC / 230208`;
- ESP-IDF: `v6.0.2`;
- target: `esp32s3`;
- Flash: 16 MiB;
- PSRAM: enabled; runtime validation found 2 MiB Quad PSRAM at 40 MHz and the startup memory test passed.

## What the profile contains

The file intentionally stores only settings that differ from ESP-IDF defaults and are required to reproduce the validated build:

```text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM=y
```

For ESP-IDF 6.0.2 on ESP32-S3, the validated build resolved the dependent PSRAM settings to Quad mode, auto-detect and 40 MHz after `CONFIG_SPIRAM=y`.

## Recommended use inside a test project

The simplest method is to copy the profile to the project root as `sdkconfig.defaults`:

```powershell
Copy-Item ..\..\software\espressif\config\panlee-v15-230208-sample-a.idf6.0.2.sdkconfig.defaults .\sdkconfig.defaults
```

Adjust the relative path for the directory from which the command is run.

If a generated `sdkconfig` already exists, remove it before validating a new/default profile so stale values do not mask the profile:

```powershell
Remove-Item .\sdkconfig -Force -ErrorAction SilentlyContinue
Remove-Item .\sdkconfig.old -Force -ErrorAction SilentlyContinue
idf.py fullclean
idf.py build
```

ESP-IDF then creates a fresh local `sdkconfig` from the defaults plus normal Kconfig defaults.

## Alternative: use the profile without copying it

ESP-IDF supports selecting defaults through the CMake variable `SDKCONFIG_DEFAULTS`. For example from PowerShell:

```powershell
idf.py -D SDKCONFIG_DEFAULTS="..\..\software\espressif\config\panlee-v15-230208-sample-a.idf6.0.2.sdkconfig.defaults" reconfigure
idf.py build
```

Use a path appropriate to the project directory. A project may also set `SDKCONFIG_DEFAULTS` from its top-level `CMakeLists.txt` when a fixed lab profile is desired.

## Creating or updating a profile

When exploring a new board or IDF release:

1. configure the project with `idf.py menuconfig`;
2. prove the configuration by building and running on the physical board;
3. run `idf.py save-defconfig`;
4. inspect the generated `sdkconfig.defaults` and remove accidental settings;
5. delete the generated `sdkconfig` and perform a clean rebuild using only the candidate defaults;
6. record the physical run in `evidence/` before promoting the profile as validated.

Do not copy the full generated `sdkconfig` into this directory unless a specific forensic/debugging task requires an exact full snapshot. The canonical reusable profile should remain minimal.
