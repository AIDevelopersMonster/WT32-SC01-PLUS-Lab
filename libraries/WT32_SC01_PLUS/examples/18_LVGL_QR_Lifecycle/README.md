# 18_LVGL_QR_Lifecycle

Espressif QR-driven provisioning lifecycle for the WT32-SC01-PLUS LVGL HMI shell.

Target specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
ESP32-S3 + ST7796 480x320 + FT6336U-compatible touch
```

## Why this example exists

Earlier repository examples already demonstrate conventional Wi-Fi configuration through stored credentials, local setup pages and captive-portal style flows.

`18_LVGL_QR_Lifecycle` deliberately tests a different onboarding mechanism: the **Espressif provisioning protocol driven from a QR code**.

The user does not type an SSID/password into a hidden device webpage. Instead, the display presents an Espressif provisioning QR, the phone application reads the provisioning identity and PoP, and the selected Wi-Fi credentials are delivered through the Espressif provisioning protocol.

## Current transport: SoftAP

The first implementation used BLE transport with Arduino-ESP32 3.3.8. Physical testing on the reference ESP32-S3 specimen exposed a reproducible startup panic in the BLE controller before QR scanning could begin.

Observed sequence:

```text
PROV: starting Espressif BLE provisioning manager
btdm: ...
MAGIC fadebead VERSION ...
HLI Magic mismatch ...
Guru Meditation Error: Core 1 panic'ed (LoadProhibited)
```

The board then entered a reboot loop, producing synchronized display flicker and an audible speaker click.

This failure is retained as evidence. It is not classified as a QR-rendering failure and it occurred before phone interaction.

The corrected implementation uses the **official Espressif SoftAP provisioning transport** with the same QR-driven application flow:

```text
unconfigured device
        |
        v
Espressif SoftAP provisioning starts
        |
        v
LVGL displays provisioning QR
        |
        v
phone scans QR in Espressif provisioning app
        |
        v
app uses Security 1 + PoP and sends selected Wi-Fi credentials
        |
        v
ESP32-S3 joins Wi-Fi and obtains IP
        |
        v
same LVGL QR area changes to information / project QR
```

This remains **Espressif protocol provisioning**, not a captive portal and not a browser configuration page.

## Provisioning payload

The corrected QR payload uses:

```json
{
  "ver": "v1",
  "name": "PROV_xxxxxx",
  "pop": "xxxxxxxx",
  "transport": "softap"
}
```

Security remains:

```text
Protocomm / Security 1
PoP embedded in QR payload
```

The service name and PoP are deterministically derived from the ESP32-S3 eFuse MAC for this laboratory example and are also printed to Serial.

## PSRAM requirement

The physical log from the failed BLE run also showed:

```text
octal_psram: PSRAM chip is not connected, or wrong PSRAM line mode
```

The reference Panlee board uses **2 MiB QSPI PSRAM**, not OPI PSRAM.

For Arduino IDE select:

```text
Board:  ESP32S3 Dev Module
PSRAM:  QSPI PSRAM
```

The example contains a compile-time guard requiring the QSPI/QUAD PSRAM configuration. Repository CI compiles example 18 with the Panlee QSPI PSRAM profile.

The successful physical run reported:

```text
BUILD: Espressif SoftAP QR provisioning / QSPI PSRAM
PSRAM: 2 MiB, free 2044 KiB
```

The earlier Octal/OPI initialization failure and BLE-controller reboot loop were no longer present.

## QR lifecycle

The HOME page intentionally does not show a provisioning QR immediately at boot.

The QR appears only after:

```text
ARDUINO_EVENT_PROV_START
```

When the station obtains an IP address, the same QR object changes from the provisioning payload to:

```text
https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab
```

So the reusable pattern is physically demonstrated as:

```text
PROVISIONING QR -> CONNECTED INFORMATION QR
```

The connected-state GitHub QR was successfully decoded during the physical test. Compatibility with arbitrary third-party barcode/QR scanner applications is not a PASS requirement; the provisioning QR is qualified against the Espressif provisioning application, while generic scanners may vary in camera, focus and QR decoding behavior.

## Retained HMI features

The example preserves:

- `HOME / REMOTE / SETTINGS / INFO` navigation;
- Light/Dark theme;
- persisted theme and brightness through `Preferences`;
- physical backlight control;
- live Device Info;
- touch navigation.

The SETTINGS page adds:

```text
RESET WIFI + REBOOT
```

which erases saved Wi-Fi credentials and reboots so the QR provisioning sequence can be repeated without deleting the separate UI Preferences namespace.

## Thread-safety rule

Provisioning/Wi-Fi events run from another FreeRTOS task.

The event handler therefore **does not call LVGL directly**. It publishes a small network state; the Arduino `loop()` applies that state to the UI.

## Dependencies

```text
Arduino-ESP32 3.3.8
LVGL 8.3.11
WiFiProv (included with Arduino-ESP32)
```

`build_opt.h`:

```text
-DLV_CONF_SKIP
-DLV_USE_QRCODE=1
```

## Current status

```text
SOURCE COMPLETE
CI TARGET ADDED WITH QSPI PSRAM PROFILE
SOFTAP QR PROVISIONING: PHYSICAL PASS
SECURITY 1 / PoP: PHYSICAL PASS
WI-FI CREDENTIAL DELIVERY: PHYSICAL PASS
WI-FI CONNECTION + IP: PHYSICAL PASS
QR -> GITHUB INFO URL: PHYSICAL PASS
RESET + REPROVISION: PHYSICAL PASS
WEB FLASHER CATALOGUED
```

The earlier BLE transport experiment remains recorded separately as a **PHYSICAL FAIL** on the tested Arduino-ESP32 3.3.8 / ESP32-S3 configuration. It does not reduce the PASS status of the corrected SoftAP implementation.

## Physical validation record

The corrected build was physically exercised on the reference Panlee specimen on **2026-08-20**.

Observed successful sequence included:

```text
WT32-SC01-PLUS 18_LVGL_QR_Lifecycle
BUILD: Espressif SoftAP QR provisioning / QSPI PSRAM
PROV SERVICE: PROV_E22748
PROV QR PAYLOAD: {"ver":"v1","name":"PROV_E22748","pop":"60737111","transport":"softap"}
PSRAM: 2 MiB, free 2044 KiB
PROV: starting Espressif SoftAP provisioning manager
PROV EVENT: START (SOFTAP)
READY: Espressif SoftAP QR lifecycle initialized
UI NETWORK STATE: PROVISIONING
PROV EVENT: CREDENTIALS RECEIVED
WIFI: CONNECTED 10.14.98.252
PROV EVENT: CREDENTIALS SUCCESS
```

The physical test confirmed:

- stable boot with the correct 2 MiB QSPI PSRAM configuration;
- provisioning QR rendered on the physical 480x320 display;
- phone-driven Espressif QR onboarding;
- SoftAP provisioning transport;
- Security 1 / Proof of Possession exchange;
- Wi-Fi credential delivery to the ESP32-S3;
- successful station association and IP acquisition;
- transition from provisioning QR to the GitHub/project information QR;
- successful decoding of the connected-state GitHub URL;
- `HOME / REMOTE / SETTINGS / INFO` navigation remained operational;
- `RESET WIFI + REBOOT` erased Wi-Fi credentials and started a fresh provisioning cycle;
- the second provisioning cycle again accepted credentials and reconnected successfully;
- no Guru Meditation or reset loop occurred in the corrected SoftAP run.

### Video evidence

- [YouTube Shorts — WT32-SC01-PLUS Espressif QR provisioning + LVGL lifecycle](https://youtube.com/shorts/Fngs_ii1uKk)

## Web Flasher

The corrected example is physically validated and included in the repository Web Flasher catalog. The Web Flasher build uses the Panlee QSPI PSRAM / 16 MiB Flash compile profile required by this example.

## Claim boundary

The physical PASS certifies the corrected **Espressif SoftAP QR provisioning lifecycle** on the named Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen:

- QR-driven onboarding through a compatible Espressif provisioning application;
- Security 1 / PoP provisioning;
- credential delivery;
- station connection and IP acquisition;
- transition to the project information QR;
- reset and repeat provisioning.

It does not certify:

- the earlier BLE transport implementation;
- every Android/iOS device or every third-party QR/barcode scanner;
- enterprise Wi-Fi;
- production PoP/key-management policy;
- arbitrary QR payload compatibility;
- captive portal behavior;
- OTA behavior.
