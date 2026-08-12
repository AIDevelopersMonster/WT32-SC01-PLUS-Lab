# Visitor Hardware Validation Guide

This guide explains how contributors can test their own WT32-SC01-PLUS compatible board and provide reproducible evidence.

## Principle

Do not assume that all boards are identical. The repository tracks:

- OBSERVED: visible markings and measured facts;
- REPORTED: vendor/community information;
- VERIFIED: reproduced on a named specimen.

## 1. Collect host information

Run:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1
```

This collects the development environment and USB information without modifying the board.

## 2. Identify the board USB connection

Look for entries containing:

- ESP
- USB JTAG/serial debug unit
- CP210x
- CH340/CH343
- CH910

Record the COM port if available.

## 3. Run the passive chip probe

After identifying the port:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1 -Port COMx
```

Replace COMx with your real port.

The probe uses read-only identification commands. It does not erase or write flash.

## 4. Submit evidence

Please provide:

- front and back PCB photos;
- all visible markings;
- audit report;
- display and touch observations;
- any successful or failed examples.

Remove personal paths, proxy information and MAC addresses before public upload if desired.

## 5. Add a specimen record

Use:

```
evidence/specimens/<your-board-id>/
```

Never replace the reference Panlee specimen data with another unknown revision.
