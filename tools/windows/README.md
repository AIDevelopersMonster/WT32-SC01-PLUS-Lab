# Windows tools

This directory contains Windows-side helper tools for the WT32-SC01-PLUS Lab.

The main tool is:

- [`wt32-sc01-plus-audit.ps1`](wt32-sc01-plus-audit.ps1) — a read-only host/environment audit with an optional passive ESP chip/flash identification step.

The script is intended for both maintainers and visitors who want to report a real WT32-SC01-PLUS board or hardware variant.

## What the script does

By default, the script inspects the Windows host and records information that can help reproduce a hardware/software environment:

- Windows version/build and architecture;
- WinGet availability;
- Git;
- VS Code;
- Python and pip;
- CMake, Ninja and Make;
- ESP-IDF command availability;
- Espressif EIM CLI availability;
- `esptool` availability/version;
- OpenOCD;
- PlatformIO and Arduino CLI;
- relevant VS Code extensions;
- WSL version/status/distributions;
- serial ports;
- likely ESP/JTAG/USB-UART devices such as native Espressif USB Serial/JTAG, CP210x, CH340/CH343 or CH910 bridges.

It writes the collected information to a timestamped text file such as:

```text
wt32-sc01-plus-audit-20260812-154500.txt
```

The script does **not** erase or write flash.

## Requirements

For the basic Windows audit:

- Windows PowerShell or PowerShell;
- no ESP-IDF installation is required;
- no administrator privileges should normally be required.

For the optional board query:

- Python must be available;
- `esptool` must be installed so that this works:

```powershell
python -m esptool version
```

The script reports missing tools instead of requiring every tool to be installed.

## Recommended way: clone the repository

Open PowerShell and run:

```powershell
git clone https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab.git
cd WT32-SC01-PLUS-Lab
```

Then run the audit from the repository root:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1
```

A report file will be created in the current directory.

## If PowerShell blocks the script

Do not permanently weaken the machine-wide execution policy just for this tool.

For the current PowerShell process only, you can use:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
```

Then run:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1
```

Closing that PowerShell window removes the process-scoped override.

## Basic host audit

Run:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1
```

This is the safest first step. It does not require you to know the board COM port.

The report will include sections such as:

```text
=== WINDOWS ===
=== PACKAGE MANAGER ===
=== CORE TOOLS ===
=== ESPRESSIF ===
=== PLATFORMIO / ARDUINO ===
=== VS CODE EXTENSIONS ===
=== WSL ===
=== USB / SERIAL CANDIDATES ===
=== OPTIONAL BOARD QUERY ===
=== PUBLIC REPORTING NOTES ===
```

## Find the board COM port

Connect the WT32-SC01-PLUS by USB before running the script.

In the `USB / SERIAL CANDIDATES` section, look for entries such as:

```text
SERIAL: COM7 | USB Serial Device ...
PNP: USB JTAG/serial debug unit | ... | USB\VID_303A&PID_1001...
```

or external USB-UART bridge names such as:

```text
CP210x
CH340
CH343
CH910
```

Do not assume every WT32-SC01-PLUS revision uses the same USB interface.

## Passive board query

After identifying the correct COM port, run the script again with `-Port`:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1 -Port COM7
```

Replace `COM7` with the actual port on your computer.

The script then asks `esptool` for:

- chip identity;
- chip revision information provided by `esptool`;
- flash manufacturer/device information;
- detected flash size when available.

The board may reset or enter the ROM bootloader during communication. The script does not issue flash erase or flash write commands.

## Optional WSL tool probe

The standard audit records WSL version/status but does not launch Linux commands inside WSL.

To also inspect the default WSL Linux distribution:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1 -ProbeWslLinux
```

This checks items such as Linux distribution information, Git, Python, CMake, Ninja and GCC.

## Optional MAC query

A chip MAC address is a unique hardware identifier and is **not needed** for a normal public board report.

If you intentionally want to read it:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1 -Port COM7 -IncludeMac
```

Review and redact the resulting MAC address before uploading the report publicly if you do not want to disclose it.

## Choose a report filename

You can provide your own output path:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1 -OutFile .\my-board-audit.txt
```

Options can be combined:

```powershell
.\tools\windows\wt32-sc01-plus-audit.ps1 -Port COM7 -ProbeWslLinux -OutFile .\my-board-audit.txt
```

## How to read the result

The audit is evidence, not an automatic declaration that every installed component is correct.

Useful interpretations include:

### `NOT FOUND`

Example:

```text
NOT FOUND: idf.py
```

This means that command is not available in the PowerShell environment used for the audit. It does not necessarily prove that the software is absent from the computer; for example, some tools are available only inside a dedicated shell or private virtual environment.

### Serial/PNP lines

These help identify how the physical board is connected to Windows. Preserve the USB VID/PID and device name when reporting a hardware variant because these can distinguish native ESP USB from an external USB-UART bridge.

### `chip-id` and `flash-id`

These are direct observations from the connected specimen and are therefore useful evidence for the repository's `VERIFIED` category, provided the correct board/COM port was selected.

### Host tool versions

These belong to the test environment. They are useful for reproducing successful or failed tests but do not define the hardware revision itself.

## What to do with the generated report

For a new board or hardware revision, use GitHub Issues rather than trying to write directly to the repository.

Open the repository's **Issues** page, choose **New issue**, then choose:

**Report your WT32-SC01-PLUS board**

Attach:

- the generated `wt32-sc01-plus-audit-*.txt` file;
- a clear photo of the complete front side;
- a clear photo of the complete back side;
- close-ups of manufacturer, PCB, revision and date/batch markings;
- close-ups of important IC markings if readable;
- observations about display, touch, storage, audio and connectors when known.

Full visitor workflow:

- [`../../docs/VISITOR-HARDWARE-VALIDATION.md`](../../docs/VISITOR-HARDWARE-VALIDATION.md)

Issue template:

- [`../../.github/ISSUE_TEMPLATE/board_specimen_report.md`](../../.github/ISSUE_TEMPLATE/board_specimen_report.md)

## Privacy before public upload

The script attempts to replace the current Windows user profile/name in captured text, but it cannot guarantee that every possible personal identifier is removed.

Before attaching a report publicly, review it for:

- MAC addresses;
- serial numbers or unique device identifiers;
- proxy configuration/details;
- local file paths;
- usernames not caught by automatic replacement;
- company/private network names;
- anything else you do not want to publish.

Do not upload passwords, tokens, private keys, Wi-Fi credentials or proprietary firmware.

## Evidence rules for this repository

The project distinguishes:

- **OBSERVED** — markings, physical features and direct observations;
- **VERIFIED** — a result reproduced on the named specimen;
- **REPORTED** — seller/vendor/community information that has not been reproduced on the named specimen.

A report from one board must not silently redefine every WT32-SC01-PLUS board. Different OEMs, PCB revisions, displays, touch controllers, memory configurations and USB interfaces may exist.

## What this script intentionally does not do

It does not:

- erase flash;
- write firmware;
- modify eFuses;
- probe unknown GPIO electrically;
- assume a display/touch pinout;
- prove PSRAM capacity solely from a seller description;
- claim that two visually similar boards are electrically identical.

More invasive acceptance tests belong to later HW stages after the specimen has been identified.

## Typical visitor workflow

```text
Connect board by USB
        |
        v
Run basic Windows audit
        |
        v
Find likely COM / USB interface
        |
        v
Run audit again with -Port COMx
        |
        v
Review the generated .txt for privacy
        |
        v
Take clear front/back/marking photos
        |
        v
GitHub Issues -> Report your WT32-SC01-PLUS board
        |
        v
Maintainers compare the specimen with known variants
        |
        v
Useful evidence may be incorporated into board-passports,
board-variants and evidence/specimens with provenance to the issue
```

## Problems and bug reports

If the script itself fails, open a normal **Hardware/software bug** issue and include:

- the PowerShell error;
- Windows version/build;
- the command you used;
- whether you ran from a Git clone or downloaded files;
- the generated partial audit file, if one was created.
