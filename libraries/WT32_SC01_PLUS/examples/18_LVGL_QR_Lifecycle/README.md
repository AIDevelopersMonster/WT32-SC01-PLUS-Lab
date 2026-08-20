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

The physical log from the failed run also showed:

```text
octal_psram: PSRAM chip is not connected, or wrong PSRAM line mode
```

The reference Panlee board uses **2 MiB QSPI PSRAM**, not OPI PSRAM.

For Arduino IDE select:

```text
Board:  ESP32S3 Dev Module
PSRAM:  QSPI PSRAM
```

The example now contains a compile-time guard requiring the QSPI/QUAD PSRAM configuration. Repository CI compiles example 18 separately with:

```text
esp32:esp32:esp32s3:PSRAM=enabled,FlashSize=16M
```

This prevents the previously observed OPI/Octal mismatch from being silently accepted.

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

So the reusable pattern remains:

```text
PROVISIONING QR -> CONNECTED INFORMATION QR
```

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
SOURCE CORRECTED AFTER PHYSICAL FAIL
CI TARGET ADDED WITH QSPI PSRAM PROFILE
BLE TRANSPORT: PHYSICAL FAIL ON ARDUINO-ESP32 3.3.8 / ESP32-S3
SOFTAP QR TRANSPORT: PHYSICAL VALIDATION REQUIRED
WEB FLASHER: NOT YET ELIGIBLE
```

## Physical validation checklist for corrected build

1. In Arduino IDE select **ESP32S3 Dev Module** and **QSPI PSRAM**.
2. Flash the corrected `18_LVGL_QR_Lifecycle`.
3. Confirm boot log no longer reports `octal_psram` initialization failure.
4. Confirm Serial prints:

```text
BUILD: Espressif SoftAP QR provisioning / QSPI PSRAM
PSRAM: 2 MiB, free ... KiB
PROV: starting Espressif SoftAP provisioning manager
PROV EVENT: START (SOFTAP)
```

5. Confirm the display remains stable with no reboot loop or synchronized speaker click.
6. Confirm HOME changes to `ESPRESSIF QR PROVISIONING` and displays a clean QR.
7. Open a compatible Espressif provisioning application on the phone.
8. Scan the QR from the physical display.
9. Confirm the app recognizes the device using the embedded service name / PoP.
10. Select a real 2.4 GHz Wi-Fi network and provide its password.
11. Confirm Serial reports credential reception and success.
12. Confirm the board obtains a station IP address.
13. Confirm HOME changes to `DEVICE ONLINE`.
14. Confirm the QR changes to the repository/project information URL.
15. Reboot without clearing Wi-Fi and confirm reconnection.
16. Use `RESET WIFI + REBOOT` and confirm QR onboarding starts again.
17. Confirm theme/brightness persistence and navigation still operate.

Useful markers:

```text
WT32-SC01-PLUS 18_LVGL_QR_Lifecycle
BUILD: Espressif SoftAP QR provisioning / QSPI PSRAM
PROV SERVICE: PROV_xxxxxx
PROV QR PAYLOAD: {"ver":"v1",..."transport":"softap"}
PROV EVENT: START (SOFTAP)
PROV EVENT: CREDENTIALS RECEIVED
PROV EVENT: CREDENTIALS SUCCESS
WIFI: CONNECTED x.x.x.x
UI NETWORK STATE: CONNECTED
READY: Espressif SoftAP QR lifecycle initialized
```

## Claim boundary

The initial BLE implementation is **not a PASS**. It physically failed during BLE-controller startup on the tested Arduino-ESP32 3.3.8 configuration.

A PASS for the corrected example requires successful phone-driven QR provisioning through the Espressif SoftAP transport on the named Panlee specimen.

A visible QR alone, successful compilation, or manual Wi-Fi configuration is not sufficient.

After the corrected full phone-driven flow passes, the example becomes eligible for the main README and Web Flasher catalog.
