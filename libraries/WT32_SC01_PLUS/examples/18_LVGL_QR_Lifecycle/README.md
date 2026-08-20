# 18_LVGL_QR_Lifecycle

Espressif BLE provisioning QR lifecycle for the WT32-SC01-PLUS LVGL HMI shell.

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

The intended user flow is:

```text
unconfigured device
        |
        v
Espressif BLE provisioning starts
        |
        v
LVGL displays provisioning QR
        |
        v
phone scans QR in Espressif provisioning app
        |
        v
phone sends selected Wi-Fi credentials over BLE provisioning protocol
        |
        v
ESP32-S3 joins Wi-Fi and obtains IP
        |
        v
same LVGL QR area changes to information / project QR
```

This is **not** a hidden web page and **not** a Wi-Fi captive portal.

## Espressif provisioning protocol

The example uses Arduino-ESP32 `WiFiProv` with:

```text
Transport: BLE
Security:  Protocomm / Security 1
PoP:       embedded in the QR payload
```

The provisioning payload follows the Espressif format:

```json
{
  "ver": "v1",
  "name": "PROV_xxxxxx",
  "pop": "xxxxxxxx",
  "transport": "ble"
}
```

The service name and PoP are derived deterministically from the ESP32-S3 eFuse MAC for this laboratory example. They are also printed to Serial for evidence/debugging.

The QR should be scanned using a compatible official Espressif provisioning client such as **ESP BLE Provisioning**, rather than with a normal camera app alone.

## QR lifecycle

The HOME page intentionally does not show a provisioning QR immediately at boot.

The QR appears only after the Arduino provisioning stack reports:

```text
ARDUINO_EVENT_PROV_START
```

This prevents an already-provisioned unit from briefly showing a false onboarding QR.

When the station obtains an IP address, the same QR object changes from the provisioning payload to:

```text
https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab
```

This establishes the reusable lifecycle pattern:

```text
PROVISIONING QR -> CONNECTED INFORMATION QR
```

A future product can replace the landing URL with a stable product/help/firmware redirect without changing the provisioning architecture.

## Retained HMI features

The example preserves the previously validated application layers:

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

which erases saved Wi-Fi credentials and reboots so the provisioning flow can be tested repeatedly without erasing the separate UI Preferences namespace.

## Thread-safety rule

Arduino provisioning/Wi-Fi events run from another FreeRTOS task.

The event handler therefore **does not call LVGL directly**. It only publishes a small network state. The Arduino `loop()` consumes that state and updates LVGL from the UI task context.

This separation is intentional and should be retained in later networked HMI examples.

## Dependencies

```text
Arduino-ESP32 3.3.8
LVGL 8.3.11
WiFiProv (included with Arduino-ESP32)
```

The example `build_opt.h` contains:

```text
-DLV_CONF_SKIP
-DLV_USE_QRCODE=1
```

because the LVGL 8.3 default configuration leaves the QR-code widget disabled.

## Current status

```text
SOURCE CREATED
CI TARGET TO BE ADDED
PHYSICAL VALIDATION REQUIRED
WEB FLASHER: NOT YET ELIGIBLE
```

## Physical validation checklist

For the reference Panlee specimen, the important test is the complete **phone -> QR -> BLE provisioning -> Wi-Fi** flow.

1. Start from a board with no saved Wi-Fi credentials. If necessary use `SETTINGS -> RESET WIFI + REBOOT`.
2. Confirm Serial reports the generated `PROV_...` service name and QR payload.
3. Confirm the HOME page changes from `NETWORK STARTING` to `ESPRESSIF BLE PROVISIONING`.
4. Confirm a QR is rendered cleanly on the 480x320 display.
5. Open the Espressif provisioning application on the phone.
6. Scan the QR from the physical WT32-SC01-PLUS screen.
7. Confirm the app recognizes the provisioning service without manually typing the service name/PoP.
8. Select a real 2.4 GHz Wi-Fi network and provide its password in the app.
9. Confirm Serial reports credential reception/success.
10. Confirm the board obtains a station IP address.
11. Confirm the HOME page changes to `DEVICE ONLINE`.
12. Confirm the QR changes from the Espressif provisioning payload to the project/info URL.
13. Scan the connected-state QR with a normal QR scanner and confirm the project URL is encoded.
14. Reboot without resetting Wi-Fi and confirm the board reconnects instead of asking for provisioning again.
15. Use `RESET WIFI + REBOOT` and confirm the provisioning QR lifecycle can be started again.
16. Confirm theme/brightness persistence and the remaining navigation still operate.
17. Confirm no panic, reset loop, stuck touch or visible QR/render corruption occurs.

Useful Serial markers include:

```text
WT32-SC01-PLUS 18_LVGL_QR_Lifecycle
PROV SERVICE: PROV_xxxxxx
PROV QR PAYLOAD: {"ver":"v1",..."transport":"ble"}
PROV EVENT: START
PROV EVENT: CREDENTIALS RECEIVED
PROV EVENT: CREDENTIALS SUCCESS
WIFI: CONNECTED x.x.x.x
UI NETWORK STATE: CONNECTED
READY: Espressif provisioning QR lifecycle initialized
```

## Claim boundary

A physical PASS for this example requires successful provisioning through the QR-driven Espressif BLE flow on the named specimen.

Source compilation, a visible QR alone, or manual Wi-Fi configuration are **not sufficient** for PASS.

This example does not yet certify:

- provisioning from every Android/iOS phone;
- SoftAP provisioning;
- captive portal behavior;
- custom provisioning endpoints;
- production PoP/key management;
- enterprise Wi-Fi;
- QR URL redirect infrastructure;
- OTA provisioning or firmware update.

After the full physical phone-driven flow passes, the example becomes eligible for the main README and Web Flasher catalog.
