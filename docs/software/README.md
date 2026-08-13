# Software tools

This section collects the software used to identify, back up, flash, develop, test and characterize **WT32-SC01-PLUS / ESP32-S3** boards.

The lab prefers official Espressif tools for chip-level operations and reproducible command-line tools for evidence collection. Third-party frameworks are listed separately because they serve a different purpose.

> **Safety rule:** identify the board and make a verified flash backup before erasing or replacing the factory firmware. Reading chip information, flash ID and eFuse values is normally part of passive characterization; writing eFuses, enabling Secure Boot, enabling Flash Encryption, or erasing flash is a different class of operation and should not be done during initial acceptance.

## Quick selection

| Tool | What we use it for | Platform | Priority | Official download / docs |
|---|---|---|---|---|
| **esptool** | Chip identification, MAC/flash information, flash read/write/erase, image inspection | Windows / Linux / macOS | **Essential** | [Espressif esptool documentation](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/index.html) |
| **ESP Flash Download Tool** | Official GUI for flashing, reading flash/eFuse information and factory-style operations | Windows | **Recommended** | [Espressif Other Tools](https://www.espressif.com/en/support/download/other-tools) |
| **ESP-IDF** | Native Espressif SDK, compiler, debugger, serial monitor and low-level ESP32-S3 development | Windows / Linux / macOS | **Essential for low-level work** | [ESP-IDF Get Started for ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/index.html) |
| **PlatformIO** | Reproducible project builds, dependency management and convenient multi-framework development | Windows / Linux / macOS | Recommended | [PlatformIO](https://platformio.org/) |
| **Arduino-ESP32** | Fast experiments and compatibility testing with Arduino libraries | Windows / Linux / macOS | Optional / useful | [Arduino-ESP32 documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/) |
| **LVGL** | GUI framework for display/touch experiments after the panel and pinout are verified | Cross-platform library | Later stage | [LVGL](https://lvgl.io/) |
| **ESP RF Test Tool** | Wi-Fi/BLE RF characterization and certification-oriented tests | See Espressif package | Advanced | [Espressif Other Tools](https://www.espressif.com/en/support/download/other-tools) |
| **WFA Certification and Test Guide** | Wi-Fi Alliance certification workflow and QuickTrack guidance | Documentation / test workflow | Product certification only | [Espressif Other Tools](https://www.espressif.com/en/support/download/other-tools) |

## 1. esptool

**esptool** is the primary command-line utility for low-level ESP bootloader and SPI flash work.

### Why it is useful in this lab

- identify the connected ESP32-S3;
- read chip and flash information;
- inspect the bootloader connection;
- read the complete factory flash into a binary file;
- write a known firmware image;
- erase flash when an experiment explicitly requires it;
- automate evidence collection in scripts and CI-friendly workflows.

### Download / installation

Official documentation:

- https://docs.espressif.com/projects/esptool/en/latest/esp32s3/index.html

Typical Python installation:

```bash
python -m pip install esptool
```

Verify installation:

```bash
python -m esptool version
```

For this repository, **esptool is preferred for reproducible hardware audits** because commands and results can be recorded exactly.

## 2. ESP Flash Download Tool

**ESP Flash Download Tool** is Espressif's official graphical flashing utility.

### Why it is useful in this lab

- convenient Windows GUI for connecting to the ROM bootloader;
- chip information checks;
- flash operations without constructing command lines manually;
- reading flash regions for backup/reverse-engineering work;
- reading eFuse information;
- comparing GUI results with `esptool` output;
- later factory/programming experiments.

### Download

Espressif download page:

- https://www.espressif.com/en/support/download/other-tools

On that page, open **Flash Download Tools** and download the current release.

### Lab policy

During initial board acceptance:

- **OK:** identify chip;
- **OK:** read flash information;
- **OK:** read eFuse information;
- **OK:** make a full flash backup;
- **DO NOT:** erase flash before a verified backup exists;
- **DO NOT:** write eFuses during passive characterization;
- **DO NOT:** enable Secure Boot or Flash Encryption just to test the tool.

## 3. ESP-IDF

**ESP-IDF** is Espressif's native development framework and the reference environment for ESP32-S3.

### Why it is useful in this lab

- authoritative ESP32-S3 peripheral support;
- access to low-level GPIO, SPI, I2C, UART, USB, Wi-Fi and BLE features;
- controlled experiments when Arduino abstractions hide hardware details;
- serial monitoring and flashing;
- future display/touch/audio/storage diagnostics;
- debugging and production-grade firmware development.

### Download / installation

Official ESP32-S3 getting-started guide:

- https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/index.html

Use the Espressif installer/instructions for your operating system. This lab currently targets the **ESP32-S3** toolchain explicitly.

## 4. PlatformIO

**PlatformIO** provides a convenient reproducible project environment and can build ESP-IDF or Arduino-based projects.

### Why it is useful in this lab

- dependency management;
- repeatable builds;
- easy project configuration through `platformio.ini`;
- convenient serial monitor and upload workflow;
- useful for small hardware experiments and examples.

### Download

- https://platformio.org/

The repository contains a conservative `platformio.ini`; do not promote an unverified display controller, touch controller or pin map into the default configuration.

## 5. Arduino-ESP32

**Arduino-ESP32** is useful for quick experiments and for testing existing Arduino libraries against the WT32-SC01-PLUS hardware.

### Typical uses

- simple GPIO tests;
- I2C/SPI peripheral probes;
- display/touch library experiments;
- rapid proof-of-concept applications;
- comparison with ESP-IDF implementations.

### Download / documentation

- https://docs.espressif.com/projects/arduino-esp32/en/latest/

Arduino-based examples are useful, but results still need to be tied to a specific board specimen and hardware revision.

## 6. LVGL

**LVGL** is the GUI framework we expect to use for later integrated display/touch tests.

### Use it after

1. the display controller is confirmed;
2. display bus pins are verified;
3. touch controller and coordinates are verified;
4. PSRAM/memory configuration is known;
5. a minimal display test passes reliably.

### Download / documentation

- https://lvgl.io/
- https://docs.lvgl.io/

LVGL is **not** an identification tool and should not be used to guess the hardware configuration.

## 7. ESP RF Test Tool and Test Guide

Espressif publishes an **ESP RF Test Tool and Test Guide** package for RF performance and certification-related work. The current Espressif tools page explicitly includes **ESP32-S3** among the supported product families.

### Possible future uses

- Wi-Fi RF characterization;
- BLE RF characterization;
- transmitter/receiver test modes;
- antenna comparison between WT32-SC01-PLUS variants;
- advanced product validation with suitable RF equipment.

### Download

- https://www.espressif.com/en/support/download/other-tools

This is an **advanced characterization tool**, not required for normal firmware development.

## 8. ESP Series Chips WFA Certification and Test Guide

This Espressif package describes the Wi-Fi Alliance certification path and related test workflow.

### When it matters

Use it only if a WT32-SC01-PLUS-based design is moving toward formal Wi-Fi product certification or certification-oriented validation.

### Download

- https://www.espressif.com/en/support/download/other-tools

It is not needed for HW-00/HW-01 board identification.

## Recommended lab workflow

```text
Physical board
    |
    +--> USB / USB-UART connection
    |
    +--> esptool
    |      +--> identify ESP32-S3
    |      +--> read flash ID
    |      +--> collect basic chip information
    |      +--> make factory flash backup
    |
    +--> ESP Flash Download Tool
    |      +--> independent GUI cross-check
    |      +--> read flash / eFuse information
    |
    +--> ESP-IDF / PlatformIO
    |      +--> controlled diagnostic firmware
    |
    +--> Arduino-ESP32
    |      +--> library compatibility experiments
    |
    +--> LVGL
           +--> integrated GUI tests after hardware verification
```

## Tool categories

### Identification and recovery

- esptool
- ESP Flash Download Tool

### Firmware development

- ESP-IDF
- PlatformIO
- Arduino-ESP32

### Display/UI development

- LVGL

### Advanced RF / certification

- ESP RF Test Tool and Test Guide
- WFA Certification and Test Guide

## Evidence rule

Whenever a tool is used to establish a hardware fact, record at least:

- board passport / specimen identifier;
- tool name;
- tool version;
- connection method and serial port;
- exact command or important GUI settings;
- raw output or screenshot when useful;
- date of the observation;
- hash of binary dumps when firmware/flash is read.

Tool output is evidence about the **tested specimen**. It must not automatically be generalized to every board sold as WT32-SC01-PLUS.
