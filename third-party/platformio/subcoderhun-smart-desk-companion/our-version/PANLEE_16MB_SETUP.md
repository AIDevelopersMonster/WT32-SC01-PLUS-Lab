# Panlee 16 MB Setup

This note records the minimal changes needed to build the SubCoderHUN Smart Desk Companion for the tested Panlee WT32-SC01-PLUS.

## 1. Open the correct project

```powershell
cd C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Panlee
$PIO = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
```

Do not build from the original `WT32-SC01-PLUS-SubCoderHUN\WT32-SC01-PLUS` folder when validating the Panlee fork.

## 2. platformio.ini

Use:

```ini
platform = platformio/espressif32@6.5.0
board = esp32-s3-devkitc-1
framework = arduino

board_build.partitions = default_16MB.csv
board_build.flash_size = 16MB
board_upload.flash_size = 16MB
board_build.arduino.memory_type = qio_qspi
```

Keep:

```ini
build_flags =
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
    -I lib
    -D PLUS=1
    -D LV_MEM_SIZE="(96U * 1024U)"
```

The toolchain is pinned because the unpinned project resolved to Arduino-ESP32 3.x and failed in old LovyanGFX APIs.

## 3. LovyanGFX 1.1.7

Working setup:

```text
lib\LovyanGFX\
```

If vendored locally, remove this line from `lib_deps`:

```ini
lovyan03/LovyanGFX@1.1.7
```

because PlatformIO Registry returned `UnknownPackageError` for that old package spec in our clean rebuild.

## 4. LVGL/SquareLine color config

File:

```text
lib\lv_conf.h
```

Must contain:

```c
#define LV_COLOR_16_SWAP 0
```

## 5. Validate configuration

```powershell
& $PIO project config |
    Select-String "partitions|flash_size|memory_type|Lovyan"
```

Expected:

```text
default_16MB.csv
16MB
qio_qspi
```

## 6. Build

```powershell
& $PIO pkg install
& $PIO run -t clean
& $PIO run
```

Validated build:

```text
RAM:   78.3% (256712 / 327680)
Flash: 44.0% (2885925 / 6553600)
[SUCCESS]
```

## 7. Upload

```powershell
& $PIO run -t upload --upload-port COM10
```

Replace `COM10` with the real port.

## 8. Moscow

The tested fork replaces the first hardcoded location with `Moscow`.

To find every place involved in location selection:

```powershell
Get-ChildItem .\src -Recurse -File -Include *.c,*.cpp,*.h |
    Select-String -Pattern "Moscow|Rackeve|Budapest|Kiskunlachaza|Location"
```

Update both:

- the LVGL/SquareLine dropdown option list;
- the restore/index mapping in application logic.

After changing the order, select the city once in the UI so EEPROM contains the new value.

## 9. Radio stations

Locate station names and stream URLs:

```powershell
Get-ChildItem .\src\Features\radio -Recurse -File |
    Select-String -Pattern "http://|https://|connecttohost|station|dropdown|SetupRadio"
```

Change station display name and the corresponding direct stream URL together. After reordering stations, select a station again so the stored index is refreshed.

## 10. Moscow timezone

The weather/location path is physically working. Time is approximately one hour behind Moscow.

Inspect:

```powershell
Get-ChildItem .\src\Managers\TimeManager -Recurse -File |
    Select-String -Pattern "configTime|configTzTime|gmt|offset|daylight|3600|7200|10800"
```

For Moscow the intended civil-time configuration is UTC+3 with no daylight-saving offset. Keep timezone configuration independent from weather location.

## 11. Memory

A 16 MB flash profile does not increase internal SRAM.

Current link result still uses 78.3% of internal RAM. The upstream LVGL buffer is a major candidate:

```cpp
static lv_color_t buf[screenWidth * 100];
```

At 480 px and RGB565 that is about 96 KB.

Before moving buffers to PSRAM, verify real runtime PSRAM:

```cpp
Serial.printf("Flash: %u\n", ESP.getFlashChipSize());
Serial.printf("PSRAM found: %s\n", psramFound() ? "YES" : "NO");
Serial.printf("PSRAM total: %u\n", ESP.getPsramSize());
Serial.printf("PSRAM free: %u\n", ESP.getFreePsram());
```

Do not move DMA/display buffers to PSRAM blindly.
