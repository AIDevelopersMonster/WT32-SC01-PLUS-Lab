---
name: Report your WT32-SC01-PLUS board
about: Submit photos and passive diagnostic evidence for a WT32-SC01-PLUS board or hardware variant
title: "[BOARD] "
labels: ""
assignees: ""
---

# WT32-SC01-PLUS specimen report

Thank you for helping map real WT32-SC01-PLUS hardware variants.

You do **not** need to know Git or create a pull request. Fill in what you can, drag photos and the generated audit `.txt` file into this issue, and leave unknown fields blank.

Before posting, please review files for information you do not want to publish. In particular, MAC addresses are unique hardware identifiers and are not required.

## 1. Board identity

- Manufacturer / brand marking:
- Product marking:
- PCB / revision marking:
- Date / batch marking:
- Other visible text:
- Seller / product page (optional):
- Approximate purchase date (optional):

## 2. Photos

Please drag and drop clear original photos here.

- [ ] Front of the complete board
- [ ] Back of the complete board
- [ ] Close-up of PCB/revision markings
- [ ] Close-up of ESP32 module/chip marking
- [ ] Close-up of display/touch controller markings, if readable
- [ ] Connector labels / unusual hardware, if relevant

### Photos

<!-- Drag images below this line. -->


## 3. Windows audit report

Run from a local clone or downloaded copy of this repository:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1
```

Then drag the generated file `wt32-sc01-plus-audit-YYYYMMDD-HHMMSS.txt` below.

### Audit file

<!-- Drag the generated .txt file below this line. -->


## 4. USB / serial identity

If the audit found a serial port, copy the relevant lines here.

- COM port:
- USB device name:
- USB VID/PID, if shown:
- Native ESP USB Serial/JTAG or external USB-UART bridge, if known:

```text
paste relevant SERIAL / PNP lines here
```

## 5. Passive chip query

If `python -m esptool` is available, run the audit again with your actual COM port, for example:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1 -Port COM7
```

This mode uses identification commands and does not erase or write flash.

- Chip model:
- Chip revision:
- Flash manufacturer/device:
- Detected flash size:

```text
paste chip-id / flash-id output here, or attach the second audit file
```

Do not use `-IncludeMac` unless you intentionally want to read the chip MAC. A MAC address is not needed for a public specimen report.

## 6. Display and touch observations

Fill only what you know from direct observation or a reproducible test.

- Display works: yes / no / not tested
- Display resolution observed:
- Display controller, if verified:
- Touch works: yes / no / not tested
- Touch controller, if verified:
- Touch type: capacitive / resistive / unknown
- Backlight behavior:

## 7. Other hardware observations

- microSD / TF:
- Audio / amplifier / speaker connector:
- USB connectors:
- Buttons:
- Expansion connectors:
- PSRAM result, if tested:
- Wi-Fi/BLE result, if tested:
- Anything that differs from photos/documentation already in this repository:

## 8. Evidence status

Please distinguish what you actually observed from information copied from a seller or another project.

### OBSERVED

<!-- Visible markings, physical features, direct measurements. -->

### VERIFIED

<!-- Results reproduced on this exact specimen. -->

### REPORTED ONLY

<!-- Vendor/community claims not yet reproduced on this exact specimen. -->

## 9. Permission to incorporate the evidence

- [ ] I understand that information posted in this public issue is public.
- [ ] I am willing for maintainers to summarize my submitted hardware observations in this repository with attribution to this issue.

For photos or third-party documents, only upload material you are allowed to share. Do not upload proprietary firmware, copyrighted manuals, or vendor files unless redistribution is permitted.
