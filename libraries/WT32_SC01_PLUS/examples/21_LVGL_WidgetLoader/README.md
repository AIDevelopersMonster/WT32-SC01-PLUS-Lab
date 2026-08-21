# 21_LVGL_WidgetLoader

First declarative widget runtime for the Panlee WT32-SC01-PLUS Arduino BSP.

Target specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3
16 MiB Flash
2 MiB QSPI PSRAM
ST7796 480x320
FT6336U / FT5x06 touch
```

Status:

```text
SOURCE IMPLEMENTED
PHYSICAL VALIDATION REQUIRED
```

## Goal

Separate application content from firmware updates.

The firmware contains the BSP, LVGL, Wi-Fi, filesystem and a small `Widget Runtime`. A widget is a declarative JSON file, not a new application binary.

```text
firmware
  |
  +-- BSP / display / touch
  +-- LVGL
  +-- Wi-Fi + WebServer
  +-- LittleFS
  +-- Widget Runtime
          |
          +-- /widgets/widget.json
```

A new widget can therefore be installed without reflashing the ESP32 application.

## First physical installation path

Version 1 deliberately starts with the simplest strong test:

```text
WT32 Widget Runtime
        |
        | WEB UPLOAD
        v
password-protected SoftAP
        |
        v
phone / PC browser
        |
        | upload widget.json
        v
schema validation
        |
        v
LittleFS /widgets/widget.json
        |
        v
LVGL render
        |
        v
RESET
        |
        v
widget autoloads from flash
```

The reset/autoload step is part of the physical gate. It proves the uploaded widget survived in flash rather than only existing in RAM.

## Storage

The OTA branch already carries a 16 MiB partition table with a data partition named `spiffs`:

```text
spiffs, data, spiffs, 0xc90000, 0x360000
```

`LittleFS` mounts that partition. No partition-table change is required for this experiment.

Widget files are stored at:

```text
/widgets/widget.json
```

The runtime limits one widget document to 32 KiB.

## Web upload

Press:

```text
WEB UPLOAD
```

The board starts a temporary WPA2 SoftAP. The SSID and generated password are shown on the display and printed to Serial.

Example shape:

```text
SSID: WT32-WIDGET-305C
PASS: WT32-XXXXXX
OPEN: http://192.168.4.1
```

The portal provides a single `widget.json` upload form.

The SoftAP automatically stops after 10 minutes unless stopped manually with `STOP AP`.

## Security boundary

The runtime does **not** execute native code from widget files.

`widget.json` may only request a small, explicit set of UI objects and bindings known to the firmware.

Current schema:

```text
schema: 1
max document: 32 KiB
max objects: 24
max dynamic labels: 8
```

Supported object types in v1:

```text
label
bar
button
```

Supported label bindings:

```text
system.uptime
system.heap
system.psram
system.time
wifi.rssi
wifi.ip
wifi.ap_ip
```

Supported button action:

```text
set_brightness
```

Unsupported object types or actions are rejected during schema validation.

This is intentional. Arbitrary C/C++ or ELF execution is outside the v1 boundary.

## Example widget

Use the supplied:

```text
widget-demo.json
```

It creates a small system dashboard using only declarative data:

- uptime;
- free heap;
- free PSRAM;
- Wi-Fi RSSI;
- a static progress bar;
- brightness 30% button;
- brightness 100% button.

## Example schema

```json
{
  "schema": 1,
  "id": "demo.system",
  "name": "System Dashboard",
  "version": "1.0.0",
  "background": "#101820",
  "objects": [
    {
      "type": "label",
      "x": 18,
      "y": 50,
      "w": 300,
      "bind": "system.uptime",
      "prefix": "UPTIME: ",
      "color": "#FFFFFF"
    },
    {
      "type": "button",
      "x": 300,
      "y": 128,
      "w": 150,
      "h": 44,
      "text": "BRIGHT 30%",
      "action": "set_brightness",
      "value": 30
    }
  ]
}
```

## Build

From repository root:

```powershell
& $CLI compile `
  --fqbn "esp32:esp32:esp32s3:PSRAM=enabled,FlashSize=16M,PartitionScheme=default_16MB" `
  --library "libraries/WT32_SC01_PLUS" `
  "libraries/WT32_SC01_PLUS/examples/21_LVGL_WidgetLoader"
```

The compile command intentionally requests the 16 MiB default partition layout so the `spiffs` data partition exists for LittleFS.

## Physical validation checklist

Do not mark this example PASS from compilation alone.

Required observations:

```text
BSP display init                 PHYSICAL PASS REQUIRED
Touch                            PHYSICAL PASS REQUIRED
LittleFS mount                   PHYSICAL PASS REQUIRED
Widget Manager UI                PHYSICAL PASS REQUIRED
SoftAP starts                    PHYSICAL PASS REQUIRED
SSID/password displayed          PHYSICAL PASS REQUIRED
Browser portal opens             PHYSICAL PASS REQUIRED
widget-demo.json upload          PHYSICAL PASS REQUIRED
schema validation                PHYSICAL PASS REQUIRED
file stored in LittleFS          PHYSICAL PASS REQUIRED
LVGL widget renders              PHYSICAL PASS REQUIRED
live uptime binding changes      PHYSICAL PASS REQUIRED
brightness buttons operate       PHYSICAL PASS REQUIRED
RESET                            REQUIRED
widget autoload after RESET      PHYSICAL PASS REQUIRED
```

## Future layers — not part of v1 PASS

Once the flash/web path is physically validated, the same widget package can be transported through additional channels without changing the runtime model:

```text
SD card import
normal STA-network web upload
GitHub Widget Store
signed widget manifests
multiple installed widgets
assets/images inside .wtw packages
widget permissions/capabilities
```

Those are future validation gates and must not be described as implemented by this example yet.
