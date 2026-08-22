# 21_LVGL_WidgetLoader

First physically validated declarative widget runtime for the Panlee WT32-SC01-PLUS Arduino BSP.

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
MAC: 48:27:e2:1f:30:5c
```

Status:

```text
SOURCE IMPLEMENTED
PHYSICAL PASS ON REFERENCE BOARD
WEB UPLOAD PASS
LITTLEFS PERSISTENCE PASS
RESET / AUTOLOAD PASS
```

## What this example proves

This example separates application content from firmware updates.

The firmware contains the BSP, LVGL, Wi-Fi, filesystem and a small `Widget Runtime`. A widget is a declarative JSON document, not a new ESP32 application binary.

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

The physically validated path is:

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
same widget autoloads from flash
```

The reset/autoload step is important evidence: the uploaded widget survived in non-volatile Flash and was reconstructed by the runtime after reboot.

## Physical validation result

Observed on the reference Panlee board:

```text
BSP display init                 PHYSICAL PASS
Touch                            PHYSICAL PASS
LittleFS mount                   PHYSICAL PASS
Widget Manager UI                PHYSICAL PASS
SoftAP start                     PHYSICAL PASS
SSID/password display            PHYSICAL PASS
Browser portal                   PHYSICAL PASS
widget-demo.json upload          PHYSICAL PASS
schema validation                PHYSICAL PASS
LittleFS install                 PHYSICAL PASS
LVGL widget render               PHYSICAL PASS
live uptime binding              PHYSICAL PASS
heap / PSRAM bindings            PHYSICAL PASS
brightness buttons               PHYSICAL PASS
RESET                            PHYSICAL PASS
widget autoload after RESET      PHYSICAL PASS
repeat workflow                  PHYSICAL PASS
```

This PASS applies to the tested v1 feature set only. It does not imply that future SD import, GitHub Widget Store, multi-widget management or signed widget packages are already validated.

## Storage

The example uses the 16 MiB partition table and the data partition named `spiffs`:

```text
spiffs, data, spiffs, 0xc90000, 0x360000
```

`LittleFS` mounts that partition.

Installed widget:

```text
/widgets/widget.json
```

Temporary upload before validation:

```text
/widgets/widget.tmp
```

The runtime limits one widget document to:

```text
32 KiB
```

The temporary file is only renamed to `widget.json` after successful validation. A rejected upload does not become the active widget.

---

# Web upload / SoftAP

Press on the display:

```text
WEB UPLOAD
```

The board starts a temporary password-protected SoftAP and shows three values on the screen:

```text
AP:   <SSID>
PASS: <password>
OPEN: http://192.168.4.1
```

The same credentials are printed to Serial.

## Credentials on the reference board

For the physically tested board with MAC:

```text
48:27:e2:1f:30:5c
```

the generated credentials are:

```text
SSID: WT32-WIDGET-305C
PASS: WT32-1F305C
URL:  http://192.168.4.1
```

The password is therefore:

```text
WT32-1F305C
```

## How the credentials are generated

The runtime derives the suffix from the last 24 bits of the ESP32 eFuse MAC:

```text
MAC suffix: 1F305C
```

Password:

```text
WT32- + six hexadecimal characters

WT32-1F305C
```

SSID uses the last four hexadecimal characters:

```text
WT32-WIDGET- + 305C

WT32-WIDGET-305C
```

So another physical board will normally have different credentials.

The display/Serial output is always the authoritative value for that particular specimen.

### Security note

This password is deterministic and derived from the device MAC. It is suitable for the current laboratory provisioning experiment, but it should **not** be treated as a high-security production secret.

A later production-oriented version should use one or more of:

- random per-device provisioning secret;
- printed/QR PoP code;
- configurable administrator password;
- short-lived provisioning token;
- HTTPS/authentication on the normal STA network.

The current SoftAP automatically stops after 10 minutes, or can be stopped manually with:

```text
STOP AP
```

---

# How to create your own `widget.json`

Widget Runtime v1 intentionally uses a small declarative language.

You describe **what should be displayed**. The firmware decides how to create the corresponding LVGL objects.

No C/C++ source, machine code, ELF file or Arduino sketch is executed from the widget document.

## Root structure

Every widget must look like this:

```json
{
  "schema": 1,
  "id": "my.widget",
  "name": "My Widget",
  "version": "1.0.0",
  "background": "#101820",
  "objects": [
    ...
  ]
}
```

### Root fields

| Field | Required | Meaning |
|---|---:|---|
| `schema` | yes | Must currently be exactly `1` |
| `id` | yes | Widget identifier, 1..48 characters |
| `name` | yes | Human-readable name, 1..64 characters |
| `version` | yes | Widget version string, 1..24 characters |
| `background` | no | Screen color in `#RRGGBB`; default is `#101820` |
| `objects` | yes | Array containing 1..24 UI objects |

Current limits:

```text
schema:                1
maximum JSON size:     32 KiB
maximum objects:       24
maximum bound labels:  8
```

## Coordinates

The physical display is:

```text
480 x 320
```

The runtime reserves a small top area for its own `WIDGET RUNTIME v1` / `MANAGER` controls.

Widget object `y` coordinates are internally shifted down by 36 pixels.

Conceptually:

```text
physical Y = widget Y + 36
```

So:

```json
"y": 0
```

starts immediately below the runtime header, not at physical display row 0.

Runtime constraints:

```text
x: 0..460
y: 0..270 before the +36 runtime offset
w: 20..460
h: 18..250
```

Values outside these ranges are constrained by the runtime.

## Colors

Colors use six hexadecimal RGB digits:

```json
"color": "#FFFFFF"
```

Examples:

```text
#FFFFFF  white
#000000  black
#80CBC4  cyan/teal
#90A4AE  grey
#FF0000  red
#00FF00  green
#0000FF  blue
```

`background` controls the widget screen background.

In v1, the per-object `color` field is visibly applied to `label` text. Bars and buttons currently use their normal LVGL theme styling; their detailed style API is a future schema extension.

---

# Object type: `label`

A label can contain static text:

```json
{
  "type": "label",
  "x": 18,
  "y": 8,
  "w": 330,
  "text": "HELLO WT32",
  "color": "#80CBC4"
}
```

Required:

```text
type = label
and at least one of:
  text
  bind
```

Useful fields:

| Field | Meaning |
|---|---|
| `x`, `y` | position |
| `w` | label width |
| `text` | static text |
| `bind` | dynamic runtime value |
| `prefix` | text before a binding value |
| `suffix` | text after a binding value |
| `color` | text color |

## Dynamic labels / bindings

Instead of a fixed `text`, use `bind`:

```json
{
  "type": "label",
  "x": 18,
  "y": 50,
  "w": 300,
  "bind": "system.uptime",
  "prefix": "UPTIME: ",
  "color": "#FFFFFF"
}
```

The displayed value becomes approximately:

```text
UPTIME: 00:12:43
```

Bindings are refreshed approximately once per second.

### Supported bindings in v1

#### `system.uptime`

Time since the current boot:

```json
"bind": "system.uptime"
```

Example output:

```text
00:12:43
```

#### `system.heap`

Current free internal heap:

```json
"bind": "system.heap"
```

Example:

```text
187 KiB
```

#### `system.psram`

Current free PSRAM:

```json
"bind": "system.psram"
```

#### `system.time`

System clock:

```json
"bind": "system.time"
```

Output format:

```text
HH:MM:SS
```

If system time has not been configured/synchronized, v1 displays:

```text
--:--:--
```

#### `wifi.rssi`

Wi-Fi signal level:

```json
"bind": "wifi.rssi"
```

Example:

```text
-54 dBm
```

If STA Wi-Fi is not connected:

```text
offline
```

#### `wifi.ip`

Current station IP address:

```json
"bind": "wifi.ip"
```

#### `wifi.ap_ip`

Current SoftAP IP address while the upload portal is active:

```json
"bind": "wifi.ap_ip"
```

Normally:

```text
192.168.4.1
```

When the portal is stopped:

```text
off
```

### Prefix and suffix

Bindings can be decorated without changing the underlying value:

```json
{
  "type": "label",
  "x": 18,
  "y": 90,
  "w": 350,
  "bind": "system.heap",
  "prefix": "FREE HEAP: ",
  "suffix": " available",
  "color": "#FFFFFF"
}
```

Unknown binding names do not execute anything. In the current v1 runtime they render as:

```text
unsupported
```

---

# Object type: `bar`

A v1 bar is currently a **static 0..100 value**.

Example:

```json
{
  "type": "bar",
  "x": 320,
  "y": 50,
  "w": 130,
  "h": 18,
  "value": 80
}
```

Fields:

```text
type:  bar
x/y:   position
w/h:   size
value: 0..100
```

Current v1 does not bind a bar dynamically. Dynamic bars are a natural future extension.

---

# Object type: `button`

Widget Runtime v1 deliberately allows only one button action:

```text
set_brightness
```

Example:

```json
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
```

Required fields:

```text
type:   button
text:   visible button text
action: set_brightness
value:  0..100
```

Another example:

```json
{
  "type": "button",
  "x": 300,
  "y": 182,
  "w": 150,
  "h": 44,
  "text": "BRIGHT 100%",
  "action": "set_brightness",
  "value": 100
}
```

Unsupported actions are rejected during widget validation.

---

# Minimal working widget

This is enough to create a valid widget:

```json
{
  "schema": 1,
  "id": "demo.hello",
  "name": "Hello Widget",
  "version": "1.0.0",
  "background": "#101820",
  "objects": [
    {
      "type": "label",
      "x": 40,
      "y": 80,
      "w": 380,
      "text": "HELLO FROM JSON",
      "color": "#80CBC4"
    }
  ]
}
```

Save it as, for example:

```text
hello-widget.json
```

The local filename does not matter during upload. After validation the runtime stores the active document internally as:

```text
/widgets/widget.json
```

---

# Example: small live dashboard

```json
{
  "schema": 1,
  "id": "demo.status",
  "name": "Status Widget",
  "version": "1.0.0",
  "background": "#101820",
  "objects": [
    {
      "type": "label",
      "x": 18,
      "y": 8,
      "w": 350,
      "text": "PANLEE STATUS",
      "color": "#80CBC4"
    },
    {
      "type": "label",
      "x": 18,
      "y": 60,
      "w": 300,
      "bind": "system.uptime",
      "prefix": "UPTIME: ",
      "color": "#FFFFFF"
    },
    {
      "type": "label",
      "x": 18,
      "y": 100,
      "w": 300,
      "bind": "wifi.rssi",
      "prefix": "RSSI: ",
      "color": "#FFFFFF"
    },
    {
      "type": "button",
      "x": 300,
      "y": 150,
      "w": 150,
      "h": 44,
      "text": "BRIGHT 50%",
      "action": "set_brightness",
      "value": 50
    }
  ]
}
```

---

# How to install a JSON widget

1. Boot `21_LVGL_WidgetLoader`.
2. Open `WT32 WIDGET MANAGER`.
3. Press `WEB UPLOAD`.
4. Read SSID and password from the display.
5. Connect the phone/PC to that Wi-Fi network.
6. Open:

```text
http://192.168.4.1
```

7. Select your `.json` file.
8. Press:

```text
UPLOAD & INSTALL
```

9. The runtime writes the upload to a temporary LittleFS file.
10. JSON and schema are validated.
11. On PASS the temporary file becomes `/widgets/widget.json`.
12. The widget is rendered without reflashing firmware.
13. Reset the board to verify persistence.

Expected after RESET:

```text
WIDGET AUTOLOAD: PASS
```

and the same widget appears again.

---

# Common JSON errors

## Wrong schema

Wrong:

```json
"schema": 2
```

Current runtime accepts only:

```json
"schema": 1
```

## Invalid color

Wrong:

```json
"color": "red"
```

Correct:

```json
"color": "#FF0000"
```

## Empty objects array

Wrong:

```json
"objects": []
```

At least one object is required.

## Unsupported object

Wrong in v1:

```json
{
  "type": "image"
}
```

Only these are implemented:

```text
label
bar
button
```

## Unsupported button action

Wrong:

```json
"action": "reboot"
```

Current v1 accepts only:

```json
"action": "set_brightness"
```

## Brightness outside range

Wrong:

```json
"value": 150
```

Allowed:

```text
0..100
```

## Too many objects

Maximum:

```text
24
```

## Too many dynamic labels

Only the first eight bound labels can be actively registered in the current v1 runtime:

```text
8
```

Keep widget designs within that limit.

---

# Supplied test widget

Use:

```text
widget-demo.json
```

It creates a physically validated small system dashboard containing:

- static title;
- live uptime;
- free heap;
- free PSRAM;
- Wi-Fi RSSI;
- static progress bar;
- brightness 30% button;
- brightness 100% button.

This file is the canonical v1 example to copy when creating a new widget.

---

# Security boundary

The runtime does **not** execute native code from widget files.

`widget.json` can only request a small explicit set of UI objects, values and actions known to the firmware.

Current boundary:

```text
schema: 1
max document: 32 KiB
max objects: 24
max dynamic labels: 8
objects: label / bar / button
action: set_brightness
native code: NOT LOADED
```

This restriction is intentional. A malformed or unsupported JSON file should fail validation rather than become arbitrary executable code.

---

# Build

From repository root:

```powershell
& $CLI compile `
  --fqbn "esp32:esp32:esp32s3:PSRAM=enabled,FlashSize=16M,PartitionScheme=default_16MB" `
  --library "libraries/WT32_SC01_PLUS" `
  "libraries/WT32_SC01_PLUS/examples/21_LVGL_WidgetLoader"
```

The example also carries its own `partitions.csv` and a 16 KiB `loopTask` stack override for the WebServer/JSON/LVGL workload.

---

# Next layers

Now that the first Flash + SoftAP/Web path is physically validated, the same widget model can be extended without changing the core idea:

```text
multiple installed widgets
widget selector / launcher
SD card import
normal STA-network web upload
GitHub Widget Store
remote widget updates
images/assets inside .wtw packages
signed widget manifests
widget capabilities / permissions
dynamic bars, arcs, switches and sliders
HTTP/RS485/GPIO data bindings
```

These are future validation gates. They are not included in the current v1 PASS until physically tested.
