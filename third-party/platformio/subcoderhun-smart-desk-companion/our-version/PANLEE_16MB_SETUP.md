# Panlee 16 MB PlatformIO setup

Use this profile for the physically tested Panlee `ZX3D50CE08S-V15-USRC` adaptation:

```ini
[env:panlee-16mb]
platform = platformio/espressif32@6.5.0
board = esp32-s3-devkitc-1
framework = arduino
board_build.partitions = default_16MB.csv
board_build.flash_size = 16MB
board_upload.flash_size = 16MB
board_build.arduino.memory_type = qio_qspi

build_flags =
  -DBOARD_HAS_PSRAM
  -mfix-esp32-psram-cache-issue
  -I lib
  -D PLUS=1
  -D LV_MEM_SIZE="(96U * 1024U)"
```

PlatformIO may still display the base board name `ESP32-S3-DevKitC-1-N8`. The explicit flash-size and partition settings above override that base definition for the actual 16 MB build profile.

The successful build used vendored `lib/LovyanGFX/`. Do not also list `lovyan03/LovyanGFX@1.1.7` in `lib_deps`; registry resolution produced `UnknownPackageError`. Preserve the vendored library's license and nested third-party notices.

Before building, ensure `lib/lv_conf.h` sets `LV_COLOR_16_SWAP` to `0`.

## Verify and build from PowerShell

Run from the PlatformIO project directory:

```powershell
$PIO = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
& $PIO project config
& $PIO run -t clean
& $PIO run
```

Recorded successful build:

```text
RAM:   78.3% (256712 / 327680)
Flash: 44.0% (2885925 / 6553600)
[SUCCESS]
```

Upload example:

```powershell
& $PIO run -t upload --upload-port COM10
```

Replace `COM10` with the actual port for the connected board.
