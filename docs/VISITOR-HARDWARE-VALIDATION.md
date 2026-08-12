# Visitor Hardware Validation Guide

This guide explains how any owner of a WT32-SC01-PLUS compatible board can contribute reproducible evidence to this repository without needing to know Git.

## Why this matters

WT32-SC01-PLUS boards may differ by manufacturer, OEM source, PCB revision, display controller, touch controller, memory configuration, USB interface, connector population, and firmware history.

This repository therefore separates three evidence levels:

- **OBSERVED** — visible marking, physical feature, or direct measurement;
- **REPORTED** — vendor/community/documentation statement not yet reproduced on the named specimen;
- **VERIFIED** — a result reproduced on the named physical specimen.

Do not assume that another WT32-SC01-PLUS is electrically identical to the reference Panlee specimen.

---

# Easiest contribution path: no Git required

You only need a GitHub account and a Windows PC connected to your board.

1. Open this repository.
2. Download or clone the repository.
3. Connect your WT32-SC01-PLUS board by USB.
4. Run the supplied Windows audit script.
5. Photograph the board.
6. Open **Issues -> New issue**.
7. Choose **Report your WT32-SC01-PLUS board**.
8. Drag the generated `.txt` report and your photos directly into the issue.
9. Fill in whatever board markings you can read.
10. Submit the issue.

You do **not** need write access to this repository. You do **not** need to create a branch or pull request.

---

# 1. Get the repository

## Option A — Git clone

If Git is installed:

```powershell
git clone https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab.git
cd WT32-SC01-PLUS-Lab
```

## Option B — Download ZIP

On GitHub use:

**Code -> Download ZIP**

Extract the ZIP and open PowerShell in the extracted directory.

For running the audit script, a ZIP download is sufficient.

---

# 2. Connect the board

Connect the board to the PC by USB.

If the board has multiple USB connectors and you are unsure which one is used for programming/debugging, report which connector you tried and what Windows detected.

Do not connect external power or unknown GPIO wiring just for this first identity step.

---

# 3. Run the host audit

From the repository root run:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1
```

The script collects development-environment and USB/serial information and saves a report such as:

```text
wt32-sc01-plus-audit-20260812-153000.txt
```

The default audit does not write or erase the board's flash.

The script checks, when available:

- Windows information;
- WinGet;
- Git;
- VS Code;
- Python and pip;
- CMake / Ninja / Make;
- ESP-IDF;
- Espressif EIM CLI;
- esptool;
- OpenOCD;
- PlatformIO;
- Arduino CLI;
- relevant VS Code extensions;
- WSL state;
- serial ports;
- likely ESP / JTAG / USB-UART devices.

Missing tools are not automatically an error. The report is also useful for telling us what a visitor still needs to install.

---

# 4. Identify the board USB connection

In the report look for entries containing names such as:

- `USB JTAG/serial debug unit`;
- `ESP`;
- `CP210x`;
- `CH340`;
- `CH343`;
- `CH910`;
- `Silicon Labs`;
- `WCH`.

Also look for a COM port such as:

```text
COM7
```

and a USB identifier such as:

```text
VID_xxxx&PID_xxxx
```

These details help distinguish native ESP USB Serial/JTAG from boards using an external USB-UART bridge.

---

# 5. Optional passive board query

If Python and esptool are already available, run the audit again with the real serial port:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1 -Port COM7
```

Replace `COM7` with the actual port found on your PC.

This optional mode invokes identification commands such as chip and flash identification. It does not use flash erase or flash write commands.

The result can help record:

- MCU family;
- chip revision;
- flash manufacturer/device ID;
- detected flash size;
- serial communication path.

## MAC-address privacy

The normal public-report workflow does **not** need a MAC address.

The script only reads the MAC if explicitly requested with:

```powershell
-IncludeMac
```

A MAC address is a unique hardware identifier. Do not publish it unless you intentionally want to.

---

# 6. Optional WSL/Linux environment probe

If you also develop under WSL2, you may run:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1 -ProbeWslLinux
```

This records basic Linux tool availability in the default WSL distribution.

It does not configure USB passthrough and does not install anything.

---

# 7. Photograph the board

For a useful specimen report, clear photographs are often more important than seller descriptions.

Please photograph when possible:

- complete front side;
- complete back side;
- manufacturer/brand marking;
- PCB revision string;
- ESP32 module or chip marking;
- display flex/label markings;
- touch-controller area;
- USB connectors;
- expansion connectors;
- audio components;
- unusual or unpopulated footprints.

Do not remove shields, displays, flex cables, or glued parts just to make a report unless you already intended to disassemble the unit and know the risks.

---

# 8. Submit the report through GitHub Issues

Open:

**Issues -> New issue -> Report your WT32-SC01-PLUS board**

The template asks for:

- manufacturer / brand;
- product and PCB revision markings;
- board photos;
- generated audit `.txt` file;
- USB/serial identity;
- optional chip/flash identification;
- display/touch observations;
- differences from already documented boards;
- separation of OBSERVED, VERIFIED, and REPORTED information.

Files and images can normally be attached by dragging them into the GitHub issue editor.

This is the preferred contribution method for visitors who only want to report their hardware.

---

# 9. Privacy check before posting

Before publishing an issue, inspect the report and attachments.

Remove or redact anything you do not want public, especially:

- MAC addresses;
- device serial numbers, if any;
- personal directory names not already redacted;
- proxy credentials or private proxy URLs;
- access tokens;
- Wi-Fi credentials;
- private repository URLs;
- unrelated machine-identifying data.

The audit script attempts to replace the local Windows user profile path with `<USERPROFILE>`, but contributors should still review the file themselves before upload.

---

# 10. What happens after submission

A submitted issue is evidence, not an automatic change to the canonical board profile.

Maintainers may:

1. compare the report with known specimens;
2. ask for one or two additional non-destructive tests;
3. classify the board as a known or new variant;
4. assign an internal specimen identifier;
5. summarize the evidence under `docs/board-passports/`, `docs/board-variants/`, or `evidence/specimens/`;
6. keep uncertain information explicitly marked as unknown or reported-only.

A single visitor report is not enough to declare that every board with the same marketing name is identical.

---

# Advanced contribution path: fork + pull request

Experienced GitHub users may contribute a fully structured specimen record.

Typical workflow:

```text
Fork repository
    -> create branch
    -> add specimen evidence
    -> add/update board passport
    -> open Pull Request
```

Suggested evidence location:

```text
evidence/specimens/<specimen-id>/
```

Possible related documentation locations:

```text
docs/board-passports/
docs/board-variants/
config/board_profiles/
```

Do not overwrite or silently generalize the reference Panlee data when submitting a different revision.

---

# Safety boundary

The visitor audit workflow is intentionally conservative.

It is designed for:

- host-environment inspection;
- USB/serial identification;
- passive ESP chip/flash identification;
- evidence collection.

It is **not** an authorization to:

- erase flash;
- overwrite factory firmware;
- change eFuses;
- modify boot configuration;
- drive unknown GPIOs;
- probe unknown voltage domains;
- short pins or connectors;
- assume pin compatibility with another WT32-SC01-PLUS revision.

Destructive or electrically active tests belong to later documented acceptance stages and must state their assumptions explicitly.
