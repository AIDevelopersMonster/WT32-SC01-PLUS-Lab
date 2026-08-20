# WT32_SC01_PLUS Arduino BSP

Experimental Arduino board-support library for the WT32-SC01-PLUS family, developed from physical validation of specific hardware rather than assumed community pinouts.

## Project credits

- **Project lead, hardware work and physical validation:** Alex Malachevsky
- **Engineering collaboration:** Commander Sol
- **Repository:** `AIDevelopersMonster/WT32-SC01-PLUS-Lab`
- **License:** MIT

## Validated target

- Manufacturer marking: **Panlee**
- PCB marking: **ZX3D50CE08S-V15-USRC**
- Lot/date marking: **230208**
- MCU: **ESP32-S3**
- LCD: **ST7796**, 480x320 landscape, 8-bit I80

Factory reverse engineering is used only as hardware evidence for a clean Arduino BSP.

## Arduino IDE installation

The library can be distributed as a standalone ZIP without cloning the full repository.

GitHub Actions builds:

```text
WT32_SC01_PLUS-Arduino-v<version>.zip
```

from exactly:

```text
libraries/WT32_SC01_PLUS
```

Install it in Arduino IDE with:

```text
Sketch -> Include Library -> Add .ZIP Library...
```

The packaging workflow runs on relevant `main` updates and can also be started manually. A tag matching `arduino-v*` publishes the same ZIP as a GitHub Release asset.

Current library version is defined in `library.properties`.

Real Wi-Fi credentials are never packaged: `examples/08_WiFiTest/wifi_secrets.h` is local-only and explicitly excluded from the generated ZIP.

## Arduino examples

The library contains the diagnostic set plus compact application-level demonstrations:

```text
01_DisplayTest
02_TouchTest
03_StorageTest
04_SDDestructiveTest
05_AudioTest
06_RS485Test
07_IOTest
08_WiFiTest
09_BLETest
10_TestConsole
11_RainbowTouch
12_RetroClock
13_LVGL_BasicUI
14_LVGL_NavigationShell
15_LVGL_DeviceInfo
16_LVGL_ThemeSwitch
17_LVGL_SettingsPersistence
18_LVGL_QR_Lifecycle
19_LVGL_Orientation
```

`10_TestConsole` is the combined modular launcher with Serial CLI and touch GUI. The individual examples remain independent deep/qualification tests.

`11_RainbowTouch` is intentionally different: it is a small application-level demonstration rather than a qualification test. It initializes the BSP display/backlight and touch subsystems, then lets the user paint by dragging a finger over the screen. The trail color is derived directly from the touch coordinates with three simple Arduino `map()` operations and packed into RGB565. No LovyanGFX configuration, LCD pin table, touch pin table, HSV conversion, palette, or lookup table is required in the sketch.

`12_RetroClock` demonstrates a complete no-IDE product path: install from the browser Web Flasher, configure the board through its temporary Wi-Fi AP and local web page, select city/timezone, synchronize from NTP, and run a seven-segment clock with date. See [`examples/12_RetroClock/README.md`](examples/12_RetroClock/README.md).

`13_LVGL_BasicUI` is the first generic LVGL application example. It connects LVGL 8 rendering and pointer input to the BSP, then demonstrates a clickable button, live event counter and brightness slider. It requires `lvgl@8.3.11`; repository CI and the Web Flasher install that version automatically. The corrected example has been physically validated on the reference Panlee specimen. See [`examples/13_LVGL_BasicUI/README.md`](examples/13_LVGL_BasicUI/README.md).

`14_LVGL_NavigationShell` builds on the validated LVGL bridge and adds a reusable four-page HMI frame with persistent `HOME / REMOTE / SETTINGS / INFO` navigation, active-page state, touch-driven page switching, generic remote command slots and a hardware backlight control in Settings. It has been physically validated on the reference Panlee specimen and is eligible for the Web Flasher catalog. See [`examples/14_LVGL_NavigationShell/README.md`](examples/14_LVGL_NavigationShell/README.md).

`15_LVGL_DeviceInfo` keeps the validated navigation shell and turns the INFO page into a live runtime diagnostic screen. It reports ESP32-S3 model/revision/cores/CPU frequency, Arduino and ESP-IDF versions, Flash, PSRAM, heap, sketch/free-slot space, uptime and touch-controller identity. The example has been physically validated on the reference Panlee specimen and is included in the Web Flasher catalog. See [`examples/15_LVGL_DeviceInfo/README.md`](examples/15_LVGL_DeviceInfo/README.md).

`16_LVGL_ThemeSwitch` adds a centralized runtime Light/Dark appearance layer to the validated LVGL shell while retaining navigation, live Device Info, touch input and physical backlight control. Theme changes are applied safely through deferred LVGL page rebuilds. The example has been physically validated on the reference Panlee specimen and is included in the Web Flasher catalog. See [`examples/16_LVGL_ThemeSwitch/README.md`](examples/16_LVGL_ThemeSwitch/README.md).

`17_LVGL_SettingsPersistence` extends the validated theme layer with non-volatile UI settings. It stores the selected Light/Dark theme and backlight brightness in ESP32 NVS through Arduino `Preferences`, debounces brightness writes, and restores both values after restart or power cycle. This persistence behavior has been physically validated on the reference Panlee specimen and the example is included in the Web Flasher catalog. See [`examples/17_LVGL_SettingsPersistence/README.md`](examples/17_LVGL_SettingsPersistence/README.md).

`18_LVGL_QR_Lifecycle` adds the physically validated Espressif QR-driven onboarding flow. It renders an Espressif SoftAP provisioning QR on the LVGL HOME page, uses Security 1 / Proof of Possession with the official provisioning protocol, accepts Wi-Fi credentials from the phone application, joins the target network, then changes the same QR area into a GitHub/project information QR. `RESET WIFI + REBOOT` allows the complete onboarding sequence to be repeated. The validated build uses the Panlee 2 MiB QSPI PSRAM profile and is included in the Web Flasher catalog. See [`examples/18_LVGL_QR_Lifecycle/README.md`](examples/18_LVGL_QR_Lifecycle/README.md).

`19_LVGL_Orientation` validates LVGL 8 runtime software rotation at `0 / 90 / 180 / 270` degrees while keeping the BSP's physically validated landscape touch mapping as the native coordinate layer. LVGL performs the pointer-coordinate rotation, and the example verifies touch hit-testing with five targets in every orientation, logical resolution changes, QR geometry, theme/backlight controls and orientation persistence through NVS. The example has been physically validated on the reference Panlee specimen and is included in the Web Flasher catalog. See [`examples/19_LVGL_Orientation/README.md`](examples/19_LVGL_Orientation/README.md).

The display API also exposes `drawRGB565(x, y, w, h, pixels)` for RGB565 rectangle blits. LCD color transfers are synchronized with the ESP LCD completion callback so caller-owned buffers are not reused while DMA is still active.

Use **ESP32S3 Dev Module**. Host-specific COM numbers are intentionally not stored.

### Video evidence

- [YouTube Shorts — WT32-SC01-PLUS 10_TestConsole combined test](https://youtube.com/shorts/vCfhNmuI3KY)
- [YouTube Shorts — WT32-SC01-PLUS Retro Clock: Web Setup, Wi-Fi and NTP](https://youtube.com/shorts/vJq456XD2HA)
- [YouTube Shorts — WT32-SC01-PLUS + LVGL 8 Basic UI](https://youtube.com/shorts/1nZqa2jilpw)
- [YouTube Shorts — WT32-SC01-PLUS + LVGL Device Info](https://youtube.com/shorts/vlxDE6bILbU)
- [YouTube Shorts — WT32-SC01-PLUS + LVGL Light/Dark Theme Switch](https://youtube.com/shorts/e1_FdMlRdpw)
- [YouTube Shorts — WT32-SC01-PLUS Espressif QR provisioning + LVGL lifecycle](https://youtube.com/shorts/Fngs_ii1uKk)
- [YouTube Shorts — WT32-SC01-PLUS + LVGL 0/90/180/270 orientation and touch](https://youtube.com/shorts/ttZOVsNHwy4)

## v0.1 status

| Subsystem | Status | Notes |
|---|---|---|
| Board identity | VALIDATED | Panlee V15 / 230208 specimen |
| LCD | **PHYSICAL PASS** | ST7796, 480x320, I80 |
| Backlight | **PHYSICAL PASS** | PWM brightness accepted |
| Touch | **PHYSICAL PASS** | FT6336U-compatible, five-point Arduino test passed |
| SD read path | **PHYSICAL PASS** | SDSPI GPIO39/40/38/41 @ 10 MHz; FAT mount + raw/file reads |
| SD full-media write/verify | **PHYSICAL PASS** | Autonomous 8 GB qualification: full 0x00/0xAA/0x55 write + readback, FAT restored |
| SD media anomaly | **WARNING (separate card)** | Earlier ~52 GB-class card reported contradictory raw/FAT geometry; not a board failure |
| Audio | **PHYSICAL PASS** | I2S GPIO35/36/37; full high-power run completed |
| Native USB Serial with audio | **PHYSICAL PASS** | Continuous Serial heartbeat through I2S stress |
| External IO | **PHYSICAL PASS** | GPIO10/11/12/13/14/21 one-hot input validation |
| Wi-Fi | **PHYSICAL PASS** | Scan + association + DHCP + DNS + TCP/HTTP + reconnect |
| BLE | **PHYSICAL PASS** | Scan + advertise + connect + GATT PING/PONG |
| RS485 | PENDING | Dedicated test included; external peer validation pending |
| Combined TestConsole | AVAILABLE | Modular CLI + touch-GUI launcher |
| RainbowTouch demo | **PHYSICAL PASS** | Interactive BSP demo: touch painting and coordinate-mapped RGB565 trail physically observed on Panlee V15 / 230208 |
| RetroClock demo | **PHYSICAL DEMONSTRATION AVAILABLE** | Web AP setup, Wi-Fi/NTP clock and seven-segment display shown on the reference specimen; full checklist remains separately tracked |
| LVGL Basic UI | **PHYSICAL PASS** | LVGL 8 rendering, touch pointer input, button events, live counter and backlight slider physically exercised on Panlee V15 / 230208 |
| LVGL Navigation Shell | **PHYSICAL PASS** | Four-page persistent navigation shell, touch page switching, active-page highlighting, remote placeholders and Settings backlight control physically exercised on Panlee V15 / 230208 |
| LVGL Device Info | **PHYSICAL PASS** | Live ESP32-S3 runtime information, Flash/PSRAM/heap/sketch/uptime and touch-controller data displayed on the validated navigation shell |
| LVGL Theme Switch | **PHYSICAL PASS** | Runtime Dark/Light switching, theme-aware navigation/pages, Device Info continuity and physical backlight control exercised on Panlee V15 / 230208; Web Flasher catalogued |
| LVGL Settings Persistence | **PHYSICAL PASS** | Theme and backlight brightness persisted through ESP32 NVS/Preferences and restored after restart/power cycle on Panlee V15 / 230208; Web Flasher catalogued |
| LVGL QR Lifecycle | **PHYSICAL PASS** | Espressif SoftAP QR provisioning, Security 1/PoP, credential delivery, Wi-Fi/IP, GitHub info QR transition and reset/reprovision physically exercised on Panlee V15 / 230208; Web Flasher catalogued |
| LVGL Orientation | **PHYSICAL PASS** | LVGL software rotation at 0/90/180/270, aligned touch hit-testing, logical resolution changes, QR/theme/backlight continuity and NVS orientation restore physically exercised on Panlee V15 / 230208; Web Flasher catalogued |

## RainbowTouch physical demonstration

`11_RainbowTouch` was compiled, uploaded, and physically observed on the reference Panlee specimen on 2026-08-18.

Observed behavior:

- touch input initialized successfully;
- finger motion produced a persistent trail;
- the trail followed touch position across the display;
- color varied continuously over screen coordinates using the simple `map()`-based RGB field;
- drawing used only the BSP application API (`touch().read()` + `display().fillRect()`), with no LovyanGFX dependency in the sketch.

This example is therefore promoted from source-only status to **PHYSICAL PASS** for the named specimen.

## LVGL Basic UI physical demonstration

`13_LVGL_BasicUI` was physically exercised on the reference Panlee specimen on 2026-08-20 after correcting the example initialization to call `board.touch().begin()` before LVGL pointer registration.

Observed behavior:

- LVGL UI rendered cleanly;
- touch input operated through the BSP-to-LVGL pointer bridge;
- repeated presses on `Tap me` advanced the live counter, including an observed 7 -> 8 transition;
- the backlight slider tracked touch across multiple positions;
- moving the slider produced visible brightness changes.

Video evidence:

- [YouTube Shorts — WT32-SC01-PLUS + LVGL 8 Basic UI](https://youtube.com/shorts/1nZqa2jilpw)

The first run's no-touch result is retained in the example README as failure history and was traced to the omitted touch initialization call rather than a hardware defect.

## LVGL Navigation Shell physical demonstration

`14_LVGL_NavigationShell` was physically exercised on the reference Panlee specimen on 2026-08-20.

Observed behavior:

- the four-page `HOME / REMOTE / SETTINGS / INFO` shell rendered correctly;
- the persistent bottom navigation bar remained visible and responsive;
- page transitions followed touch input correctly;
- active-page highlighting followed the selected page;
- repeated navigation remained visually stable;
- Remote command placeholders were operable;
- the Settings backlight slider remained interactive and changed the physical display brightness;
- the overall interface was visually suitable as the reusable base for subsequent LVGL HMI modules.

This promotes `14_LVGL_NavigationShell` to **PHYSICAL PASS** for the named Panlee specimen.

## LVGL Device Info physical demonstration

`15_LVGL_DeviceInfo` was physically exercised on the reference Panlee specimen on 2026-08-20.

Observed behavior:

- the validated four-page navigation shell remained operational;
- the INFO page rendered live runtime diagnostic cards correctly;
- ESP32-S3 device information was displayed from the running board;
- the expected 16 MiB Flash and 2 MiB PSRAM configuration was reported;
- live heap/PSRAM, sketch-space and uptime values were visible;
- touch-controller identity was displayed;
- repeated navigation to and from INFO remained stable;
- the Settings backlight control continued to operate.

Video evidence:

- [YouTube Shorts — WT32-SC01-PLUS + LVGL Device Info](https://youtube.com/shorts/vlxDE6bILbU)

This promotes `15_LVGL_DeviceInfo` to **PHYSICAL PASS** for the named Panlee specimen and makes it eligible for the Web Flasher catalog.

## LVGL Theme Switch physical demonstration

`16_LVGL_ThemeSwitch` was physically exercised on the reference Panlee specimen on 2026-08-20.

Observed behavior:

- the LVGL shell rendered correctly in the initial Dark theme;
- the Settings appearance control switched the running UI to Light theme;
- repeated Light/Dark switching worked correctly;
- `HOME / REMOTE / SETTINGS / INFO` remained usable after theme changes;
- active-page highlighting remained visible in both themes;
- live Device Info remained available after switching themes;
- touch input and the physical backlight slider continued to operate;
- no visible rendering corruption or instability was observed during the successful run.

Video evidence:

- [YouTube Shorts — WT32-SC01-PLUS + LVGL Light/Dark Theme Switch](https://youtube.com/shorts/e1_FdMlRdpw)

This promotes `16_LVGL_ThemeSwitch` to **PHYSICAL PASS** for the named Panlee specimen and includes it in the Web Flasher catalog. Theme persistence across reboot is not claimed by this example.

## LVGL Settings Persistence physical demonstration

`17_LVGL_SettingsPersistence` was physically exercised on the reference Panlee specimen on 2026-08-20.

Observed behavior:

- Light/Dark theme switching remained operational;
- a non-default brightness value could be selected and saved;
- brightness writes were delayed so slider movement did not write NVS on every step;
- the selected theme and brightness survived restart/power cycle;
- both values were restored automatically at boot;
- Settings controls reflected the restored values;
- repeated storage/restoration with a second settings combination worked normally;
- `HOME / REMOTE / SETTINGS / INFO` navigation and live Device Info remained operational after restoration.

This promotes `17_LVGL_SettingsPersistence` to **PHYSICAL PASS** for persistence of the declared theme and brightness settings on the named Panlee specimen and includes it in the Web Flasher catalog.

## LVGL QR Lifecycle physical demonstration

`18_LVGL_QR_Lifecycle` was physically exercised on the reference Panlee specimen on 2026-08-20 using the corrected Espressif **SoftAP** provisioning transport and the Panlee **2 MiB QSPI PSRAM** build profile.

Observed behavior:

- the firmware booted stably with 2 MiB QSPI PSRAM detected;
- the Espressif provisioning QR rendered on the physical LCD;
- the phone provisioning application accepted the QR identity and PoP;
- Security 1 provisioning completed;
- Wi-Fi credentials were delivered to the ESP32-S3;
- the station connected and obtained an IP address;
- the HOME QR transitioned from provisioning payload to the project GitHub URL;
- the GitHub information QR was decoded successfully during the demonstration;
- `HOME / REMOTE / SETTINGS / INFO` navigation remained operational;
- `RESET WIFI + REBOOT` erased saved Wi-Fi credentials and restarted onboarding;
- the subsequent provisioning cycle again accepted credentials and reconnected;
- the corrected SoftAP build showed no Guru Meditation or reboot loop.

Video evidence:

- [YouTube Shorts — WT32-SC01-PLUS Espressif QR provisioning + LVGL lifecycle](https://youtube.com/shorts/Fngs_ii1uKk)

This promotes `18_LVGL_QR_Lifecycle` to **PHYSICAL PASS** for the corrected Espressif SoftAP QR lifecycle on the named Panlee specimen and includes it in the Web Flasher catalog. The earlier BLE-controller startup failure remains documented in the example README as a separate failed transport experiment.

## LVGL Orientation physical demonstration

`19_LVGL_Orientation` was physically exercised on the reference Panlee specimen on 2026-08-20.

Observed behavior:

- runtime switching between `0 / 90 / 180 / 270` degrees worked;
- landscape orientations used `480x320` logical geometry and portrait orientations used `320x480`;
- the five `TL / TR / CENTER / BL / BR` touch targets remained correctly aligned with the rendered UI in every orientation;
- LVGL pointer rotation worked on top of the existing BSP landscape touch mapping without a second application-specific transform;
- `TEST / QR / SET / INFO` navigation remained operational after repeated rotation;
- QR geometry remained correct and usable;
- Light/Dark theme and the physical backlight slider remained operational;
- selected orientation was stored through `Preferences` / NVS and restored after restart;
- no panic, reset loop or visible rendering corruption was observed during the successful run.

Video evidence:

- [YouTube Shorts — WT32-SC01-PLUS + LVGL 0/90/180/270 orientation and touch](https://youtube.com/shorts/ttZOVsNHwy4)

This promotes `19_LVGL_Orientation` to **PHYSICAL PASS** for the named Panlee specimen and includes it in the Web Flasher catalog.

## Validated LCD mapping

| Signal | GPIO |
|---|---:|
| BL | 45 |
| RST | 4 |
| DC | 0 |
| WR | 47 |
| CS | tied / unused (-1) |
| TE | 48 |
| D0..D7 | 9, 46, 3, 8, 18, 17, 16, 15 |

Display: **480x320**, RGB565, 8-bit I80, **10 MHz**.

## Validated touch mapping

| Signal | GPIO |
|---|---:|
| SDA | 6 |
| SCL | 5 |
| INT | 7 |
| RST | 4 (shared with LCD reset) |

`Wire1`, address `0x38`, 400 kHz. Observed FT6336U-compatible identity: chip `0x02`, firmware `0x03`, FocalTech `0x11`.

Validated landscape mapping:

```text
LCD_X = raw_Y
LCD_Y = 319 - raw_X
```

## Validated SD path

| Signal | GPIO |
|---|---:|
| SCK | 39 |
| MOSI | 40 |
| MISO | 38 |
| CS | 41 |

### Read-only validation

The Arduino `03_StorageTest` physically passed on the reference specimen at **10 MHz**. It validated:

- SDHC initialization and Arduino `SD` FAT mount;
- raw sector 0 read;
- root directory enumeration;
- file readback without modifying the card.

The earlier card used for this run reported contradictory raw/FAT geometry. That warning is retained as media-specific evidence and is not treated as a board-path failure.

### Autonomous full-media qualification

The Arduino `04_SDDestructiveTest` subsequently completed a full destructive qualification on a separate 8 GB-class card.

The test runs autonomously on the WT32-SC01-PLUS using the LCD and touch interface. It uses 64-sector / 32 KiB multi-sector transfers at **10 MHz** and performs:

```text
1/7  full-card WRITE  0x00
2/7  full-card VERIFY 0x00
3/7  full-card WRITE  0xAA
4/7  full-card VERIFY 0xAA
5/7  full-card WRITE  0x55
6/7  full-card VERIFY 0x55
7/7  FAT restore + probe-file write/read/delete
```

Physical final screen:

```text
PASS
00 AA 55 VERIFIED
FAT RESTORED
CARD EMPTY AND READY
7680 MiB / 15728640 SECTORS
```

This promotes the named specimen's SD write path to **PHYSICAL PASS** for the tested 8 GB card at 10 MHz.

Evidence:

[`evidence/specimens/panlee-v15-230208-sample-a/03_storage_test/arduino-sd-destructive-full-pass-8gb.jpg`](../../evidence/specimens/panlee-v15-230208-sample-a/03_storage_test/arduino-sd-destructive-full-pass-8gb.jpg)

The test does not certify every SD-card model, maximum SDSPI clock, card-detect/write-protect behavior, or all WT32-SC01-PLUS OEM revisions.

## Validated audio mapping

| Signal | GPIO |
|---|---:|
| LRCK / WS | 35 |
| BCLK | 36 |
| DOUT | 37 |

`05_AudioTest` physically passed the 20–100% amplitude ramp, 15 s sustained 90% load, repeated 100% bursts, native USB Serial coexistence, and I2S deinit without observed reboot, panic, watchdog or brownout during the controlled audio qualification run.

## Hardware profile warning

The pin mapping is validated only for the Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen/profile. Do not assume every WT32-SC01-PLUS OEM revision is identical.

## Safety boundary

Normal BSP examples avoid factory fixture-only/destructive operations. `04_SDDestructiveTest` is intentionally separate because it overwrites the entire inserted card. It requires on-device operator confirmation before the destructive sequence begins.
