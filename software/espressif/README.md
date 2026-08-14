# Espressif / ESP-IDF assets

This directory stores reusable configuration assets for Espressif tooling used by the lab.

## Current validated environment

The first validated profile was produced with:

- ESP-IDF `v6.0.2`;
- target `esp32s3`;
- physical specimen `panlee-v15-230208-sample-a`;
- board marking `Panlee / ZX3D50CE08S-V15-USRC / 230208`.

The profile was derived from a working `menuconfig` configuration, reduced with `idf.py save-defconfig`, then verified by deleting the generated local `sdkconfig` and rebuilding successfully from the defaults file alone.

See [`config/README.md`](config/README.md) for application instructions.

## Policy

Keep only intentional, reviewable configuration here. Generated project state such as `build/`, `sdkconfig`, and `sdkconfig.old` remains local and should not be used as the canonical board profile.

A configuration file here is evidence-backed for the named specimen and ESP-IDF release. If a future board revision or ESP-IDF version needs different values, add a separate profile rather than silently changing the historical one.
