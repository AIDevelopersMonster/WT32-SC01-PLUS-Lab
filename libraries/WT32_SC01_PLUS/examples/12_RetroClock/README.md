# 12_RetroClock

End-user demonstration for the validated **Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208** profile.

Status: **SOURCE + CI INTEGRATION / PHYSICAL VALIDATION PENDING**.

## Video demonstration

[YouTube Shorts — WT32-SC01-PLUS Retro Clock: Web Setup, Wi-Fi and NTP](https://youtube.com/shorts/vJq456XD2HA)

The video shows the real Panlee reference specimen running the RetroClock application, including the on-device Wi-Fi setup instructions and the final NTP-synchronized seven-segment clock display.

This example is deliberately different from the qualification tests. It demonstrates a complete no-IDE user path:

```text
Web Flasher
  -> first boot
  -> setup Wi-Fi access point
  -> captive/browser setup page
  -> home Wi-Fi credentials
  -> city/timezone selection
  -> NTP synchronization
  -> seven-segment clock + date
  -> later reconfiguration through the local web page
```

## First boot

When no usable saved Wi-Fi configuration exists, the board starts its own setup network and shows the connection details on the LCD:

```text
SSID: WT32-CLOCK-XXXX
PASS: WT32SETUP
OPEN: 192.168.4.1
```

`XXXX` is derived from the ESP32-S3 hardware identity so multiple clocks nearby are easier to distinguish.

The firmware also runs a wildcard DNS server and common captive-portal probe routes. Many phones/computers should therefore offer to open the setup page automatically; entering `http://192.168.4.1/` remains the deterministic fallback.

## Setup web page

The browser page lets the user:

- choose a scanned home Wi-Fi SSID or enter a hidden/manual SSID;
- enter the Wi-Fi password;
- choose a city/timezone preset;
- optionally use an advanced custom POSIX timezone string;
- save the settings and start the connection.

Initial presets:

- UTC;
- Moscow (UTC+3, no DST);
- Stockholm / Central Europe (DST-aware);
- London (DST-aware);
- New York (DST-aware);
- Tokyo;
- Custom POSIX TZ.

Wi-Fi and timezone settings are stored with Arduino-ESP32 `Preferences` (NVS) and survive power cycling.

## After successful configuration

The board connects to the home network, configures NTP with the selected POSIX timezone, starts HTTP on the station address, and advertises:

```text
http://wt32-clock.local/
```

The temporary setup AP is intentionally kept alive for about 60 seconds after station connection. This gives the browser enough time to learn the new home-network IP before the user reconnects the phone/PC to the normal Wi-Fi network.

The WT32 display also shows the new station IP and `wt32-clock.local` destination.

## Clock display

The clock does not require LVGL or an external font package.

- hour/minute digits are drawn as seven rectangular segments through the BSP `fillRect()` API;
- seconds use smaller seven-segment digits;
- the colon blinks once per second;
- inactive segments remain dimly visible to reproduce the appearance of classic electronic displays;
- date, city/timezone label, IP and setup hint use a tiny built-in 5x7 bitmap font.

## Timekeeping

NTP servers:

```text
pool.ntp.org
time.nist.gov
```

Local time is derived with `configTzTime()` and POSIX timezone rules rather than storing a fixed UTC offset. This allows DST-capable presets to change offsets automatically.

## Recovery / reconfiguration

If saved Wi-Fi credentials fail at boot, the board falls back to setup AP mode again.

When connected normally, open either:

```text
http://wt32-clock.local/
```

or the IP displayed on the WT32 screen to change settings.

## Validation checklist

Before promoting this example to **PHYSICAL PASS**, verify on the named Panlee specimen:

- [ ] Web Flasher installs the generated firmware successfully;
- [ ] LCD shows the correct AP SSID, password and setup IP;
- [ ] AP is visible from phone and PC;
- [ ] captive portal opens or `192.168.4.1` works manually;
- [ ] Wi-Fi scan list is populated;
- [ ] credentials survive reboot;
- [ ] Moscow remains UTC+3 with no DST;
- [ ] at least one DST-aware timezone behaves correctly;
- [ ] NTP reaches a valid date/time;
- [ ] `wt32-clock.local` resolves on a compatible client;
- [ ] seven-segment digits and date render correctly;
- [ ] failed home Wi-Fi credentials leave the user recoverable through AP setup.

## Safety / scope

This firmware is targeted at the physically validated Panlee reference profile. Do not generalize its display/touch/pin assumptions to every WT32-SC01-PLUS/OEM revision without identification and validation.
