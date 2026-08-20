# Project 4 transfer plan — reusable ESP32-TUX ideas for WT32-SC01-PLUS-Lab

Project 4 is complete as a third-party validation workload. The goal from this point is **not** to turn ESP32-TUX into the repository's permanent application framework. Instead, the lab will selectively transfer the best reusable patterns into our own WT32_SC01_PLUS ecosystem while keeping hardware support, application behavior, external services and physical evidence clearly separated.

## Transfer principles

1. Keep the existing `WT32_SC01_PLUS` BSP as the hardware boundary.
2. Reuse **patterns and architecture**, not unnecessary upstream coupling.
3. Prefer small independent examples/modules that can be physically validated one feature at a time.
4. Do not promote source presence to `PHYSICAL PASS` without a real-board run.
5. External services such as OTA hosting and weather APIs remain separate integration dependencies.
6. Keep the existing ESP32-TUX Panlee port as a reference workload and regression target.

## Capability / transfer matrix

| Feature / idea from Project 4 | Can ChatGPT implement in this repository? | Needs physical/external validation? | Recommended destination | Decision |
|---|---|---|---|---|
| LVGL application shell with page navigation | **YES** | Physical display/touch run | New reusable HMI layer/examples above BSP | TRANSFER |
| Settings page skeleton | **YES** | Physical interaction | HMI/settings module | TRANSFER |
| Brightness control | **YES — already demonstrated in `13_LVGL_BasicUI`** | Already physically demonstrated for basic path | BSP + LVGL settings UI | TRANSFER / IN PROGRESS |
| Light/Dark theme switching | **YES** | Physical UI run | HMI theme module | TRANSFER |
| Runtime orientation switching | **YES** | Physical touch/display coordinate validation | HMI display policy | TRANSFER, but validate carefully |
| Task-safe LVGL access / event-message boundary | **YES** | Stress/concurrency validation | Common HMI event layer | TRANSFER |
| HOME / REMOTE / SETTINGS / DEVICE INFO page pattern | **YES** | Physical navigation | Reusable page framework | TRANSFER conceptually |
| Programmable Remote button grid | **YES** | Physical UI + target action tests | Separate example/module; later map to GPIO/RS485/Modbus/etc. | TRANSFER as generic command surface |
| Runtime Device Info screen | **YES** | Physical display check | Diagnostics/HMI page | HIGH PRIORITY TRANSFER |
| Wi-Fi status/IP/reset controls | **YES** | Network + physical UI run | Settings/network module | TRANSFER |
| Secure SoftAP provisioning using ESP-IDF provisioning protocol | **YES in ESP-IDF applications** | Real phone/client + network run | ESP-IDF network module | OPTIONAL TRANSFER |
| Browser captive-portal provisioning | **YES, but this is not an ESP32-TUX feature** | Real browser/network run | Reuse/evolve `12_RetroClock` approach | PREFER FOR SIMPLE USER PRODUCTS |
| Persistent Wi-Fi credentials | **YES** | Reboot/reconnect test | Network settings layer | TRANSFER |
| Dual-purpose QR lifecycle | **YES** | Physical QR readability/client test | Common onboarding/info widget | HIGH PRIORITY TRANSFER |
| Stable redirect URL encoded in post-provision QR | **YES (firmware side)** | Redirect service itself requires server/DNS control | QR/info page | TRANSFER; external redirect service required |
| SPIFFS assets exposed to LVGL | **YES** | Physical mount/read/render | Asset/filesystem layer | TRANSFER |
| SD assets exposed to LVGL | **YES** | SD card run | Asset/filesystem layer | TRANSFER |
| Unified logical asset paths such as internal vs SD drives | **YES** | Physical filesystem tests | Asset abstraction | TRANSFER |
| OTA UI and event pipeline | **YES** | Physical button/event test | Update module | TRANSFER |
| Full HTTPS OTA download/write/reboot/rollback | **YES to implement** | **Requires reachable controlled OTA endpoint and real-board update cycle** | Dedicated OTA qualification project/example | NOT CLOSED BY CODE ALONE |
| Weather UI | **YES** | Physical render | Optional application widget | LOW PRIORITY |
| Live weather data | **YES to integrate** | **Requires provider URL/API key/network** | Optional application service | EXTERNAL DEPENDENCY |
| NTP/SNTP time synchronization | **YES** | Network + clock comparison | Common time service | TRANSFER |
| Moscow UTC+3 / DST=0 preset | **YES — already validated in project work** | Already physically checked in ESP32-TUX and RetroClock paths | Common timezone presets | KEEP |
| ESP-IDF 6 migration knowledge | **YES** | Build/CI plus selective hardware regression | Documentation/tooling guidance | KEEP AS REFERENCE |
| ESP32-TUX source tree as our permanent product framework | Technically possible | Large maintenance burden | — | **DO NOT ADOPT AS CORE** |

## What ChatGPT can do without the board connected

ChatGPT can implement and review source-level work for:

- reusable LVGL page/navigation scaffolding;
- settings/device-info/QR widgets;
- theme/orientation logic;
- task-safe event/message architecture;
- generic Remote button grids and callback interfaces;
- Wi-Fi configuration state machines;
- filesystem/asset abstractions;
- OTA client code and test harnesses;
- weather-service adapters;
- NTP/timezone modules;
- CI, documentation, examples and evidence templates.

These activities can establish **SOURCE / BUILD / CI** status, but not a physical PASS.

## What ChatGPT cannot honestly close alone

The following require the user and real hardware or an external service:

- proving touch/display behavior after orientation changes;
- proving QR readability with a real phone/client;
- provisioning and reconnect behavior on a real network;
- validating GPIO/RS485/Modbus actions behind Remote buttons;
- SD-card filesystem behavior on the inserted media;
- full OTA image download, slot switch, reboot and rollback;
- live weather-provider behavior using valid credentials;
- long-duration UI/network/PSRAM stress qualification.

For these items ChatGPT can prepare firmware, commands, checklists and evidence tables, but the final hardware status must come from an observed run.

## Recommended extraction order

The extraction should remain incremental rather than starting another monolithic application:

1. **LVGL core bridge** — already started by `13_LVGL_BasicUI`.
2. **Reusable navigation + Settings + Device Info**.
3. **Theme + orientation + controlled LVGL event/message access**.
4. **QR lifecycle widget** with provisioning/info modes and redirect-ready URL handling.
5. **Network configuration layer** — choose ESP-IDF provisioning for secure native clients or the existing browser/captive-portal model for simple consumer onboarding.
6. **Asset/filesystem abstraction** for internal flash and SD.
7. **Generic Remote command surface** with pluggable callbacks; later connect to GPIO/RS485/Modbus/application actions.
8. **OTA qualification** as a separate controlled test with a known reachable endpoint.
9. Optional application widgets such as weather.

## Relationship to `13_LVGL_BasicUI`

`13_LVGL_BasicUI` is the first small extraction step from the broader lessons of Project 4:

```text
Arduino application
    -> LVGL
        -> WT32_SC01_PLUS BSP
            -> Panlee hardware
```

It already demonstrates display rendering, touch input, LVGL button events, a live counter and backlight control without importing the full ESP32-TUX architecture. Future transfers should follow the same principle: small, understandable, BSP-centered modules rather than a wholesale framework copy.

## Project 4 closure state

`PROJECT_4_REFERENCE_WORKLOAD_COMPLETE`

The validated ESP32-TUX Panlee port remains useful for regression testing and architectural reference. New reusable functionality belongs in the lab's own BSP/examples/modules and should be tracked separately from the historical Project 4 port.
