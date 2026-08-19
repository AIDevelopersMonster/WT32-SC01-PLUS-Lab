# Panlee adaptation workspace

This directory is reserved for the controlled adaptation of ESP32-TUX to the physically validated Panlee reference specimen.

## Baseline changes required before first flash

1. Select ESP32-S3 / WT32-SC01 Plus explicitly.
2. Declare the real **16 MB Flash** device instead of the upstream 8 MB default.
3. Review the partition table and generated image offsets before upload.
4. Preserve the upstream WT32-SC01 Plus pin mapping because it matches the lab's validated Panlee mapping.
5. Treat the upstream 40 MHz LCD bus setting as unverified on this specimen; begin conservatively if toolchain adaptation requires changing it.
6. Replace demo timezone/OTA settings with deliberate test configuration.
7. Do not encode Wi-Fi credentials in committed source or sdkconfig files.

## First-build policy

The first successful build should minimize semantic changes to ESP32-TUX. The purpose is to answer whether the upstream architecture can run on this Panlee revision with its actual memory configuration, not to modernize the whole application at once.

If ESP-IDF API drift prevents an unchanged build, compatibility fixes must be documented separately from hardware-profile changes.

## Status

`CONFIGURATION_DESIGN_IN_PROGRESS`

No adapted firmware in this directory has yet been physically validated.
