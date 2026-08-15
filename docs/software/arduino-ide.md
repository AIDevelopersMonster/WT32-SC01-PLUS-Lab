# Arduino IDE for WT32-SC01-PLUS development

This page documents the Arduino IDE setup used for the Arduino BSP and hardware-test examples in this repository.

## Purpose

Arduino IDE is used here for fast, reproducible hardware bring-up of the Panlee WT32-SC01-PLUS / ESP32-S3 specimen. It is not a replacement for the repository's ESP-IDF research path; both workflows are kept intentionally separate.

Use Arduino IDE when you want to:

- open and run the `WT32_SC01_PLUS` library examples;
- compile small isolated hardware tests;
- upload to the ESP32-S3;
- use Serial Monitor during diagnostics;
- validate the Arduino-facing BSP API.

## Official download

Download Arduino IDE only from the official Arduino software page:

- <https://www.arduino.cc/en/software>

Official Arduino IDE 2 documentation:

- <https://docs.arduino.cc/software/ide-v2/>

Do not commit the Arduino IDE installer into this repository. The installer is large, platform-specific, and becomes obsolete; this repository stores the reproducible installation/configuration instructions instead.

## Install Arduino IDE

On Windows:

1. Download the current Arduino IDE 2.x installer from the official Arduino website.
2. Install it normally.
3. Start Arduino IDE once so that its data directories are created.

Typical executable locations include:

```text
C:\Program Files\Arduino IDE\Arduino IDE.exe
```

or, for a per-user installation:

```text
%LOCALAPPDATA%\Programs\Arduino IDE\Arduino IDE.exe
```

The exact path depends on the installation method.

## Install ESP32 support

The board uses an ESP32-S3, so Arduino IDE needs Espressif's Arduino-ESP32 platform.

Official Espressif installation documentation:

- <https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html>

In Arduino IDE, install the Espressif ESP32 platform through Boards Manager. The repository's CI currently pins Arduino-ESP32 3.3.8 for reproducible compile checks, while local development may use another explicitly tested version.

For repository validation, always record the Arduino-ESP32 version used.

## Correct board family

For the validated Panlee specimen use:

```text
ESP32S3 Dev Module
```

Do **not** select:

```text
ESP32 Dev Module
```

The latter targets the original ESP32 family and causes `esptool` to reject the physical ESP32-S3 with a wrong-chip error.

The Arduino examples in this repository can include a `sketch.yaml` file such as:

```yaml
default_fqbn: esp32:esp32:esp32s3
default_port_config:
  baudrate: 115200
```

This is project metadata, not a custom board definition.

## Native USB on the reference specimen

The tested board exposes the ESP32-S3 native USB interface. On the current Windows workstation it enumerated as a device with Espressif VID `303A` and PID `1001`.

For Arduino serial diagnostics over the native USB path, the important board-menu setting is:

```text
USB CDC On Boot -> Enabled
```

When native USB CDC is enabled, normal Arduino `Serial` output can be routed to the USB CDC port. Without it, `Serial` may instead refer to UART0 depending on the selected Arduino-ESP32 options.

For tests that depend on Serial output, verify the USB menu settings before treating a silent Serial Monitor as a firmware failure.

## Serial Monitor

The repository's diagnostic examples normally use:

```text
115200 baud
```

The actual COM number is machine-specific and should not be committed into shared project metadata.

For example, a development machine may show:

```text
COM10
```

but another computer may use `COM3`, `COM7`, etc.

## Install the local WT32_SC01_PLUS library

The library source is kept in Git:

```text
libraries/WT32_SC01_PLUS/
```

For active development on Windows, use a directory junction so Arduino IDE sees the Git worktree directly:

```powershell
New-Item -ItemType Directory -Force `
  C:\Users\CHUWI\Documents\Arduino\libraries

New-Item -ItemType Junction `
  -Path C:\Users\CHUWI\Documents\Arduino\libraries\WT32_SC01_PLUS `
  -Target C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Arduino\libraries\WT32_SC01_PLUS
```

Verify:

```powershell
Get-Item C:\Users\CHUWI\Documents\Arduino\libraries\WT32_SC01_PLUS
```

This avoids copying the library after every source change.

For a general explanation of Arduino libraries, see:

- [`../arduino-libraries.md`](../arduino-libraries.md)

## Open a repository example

Example path:

```text
C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Arduino\libraries\WT32_SC01_PLUS\examples\01_DisplayTest\01_DisplayTest.ino
```

or:

```text
...\examples\05_AudioTest\05_AudioTest.ino
```

You can open the `.ino` from File Explorer or launch Arduino IDE from PowerShell.

Example:

```powershell
Start-Process "C:\Program Files\Arduino IDE\Arduino IDE.exe" `
  -ArgumentList '"C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Arduino\libraries\WT32_SC01_PLUS\examples\01_DisplayTest\01_DisplayTest.ino"'
```

If `.ino` files are associated with Arduino IDE, this also works:

```powershell
Start-Process "C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Arduino\libraries\WT32_SC01_PLUS\examples\01_DisplayTest\01_DisplayTest.ino"
```

## Normal verification workflow

For a hardware example:

```text
open sketch
    -> confirm ESP32S3 Dev Module
    -> confirm USB/Serial options if needed
    -> Verify
    -> Upload
    -> open Serial Monitor when the test uses Serial
    -> observe physical hardware
    -> record PASS/FAIL
```

Compile success alone is not a physical hardware PASS.

## Current validated Arduino results

For the Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen:

- Arduino display test: physical PASS;
- backlight PWM test: physical PASS;
- high-power I2S audio playback: physical PASS without controller reboot;
- concurrent USB Serial during the audio test requires the native USB CDC settings to be enabled and is tracked separately from the audio-output PASS.

These results apply to the tested specimen/profile, not automatically to every OEM board sold as WT32-SC01-PLUS.

## Useful official references

- Arduino IDE download: <https://www.arduino.cc/en/software>
- Arduino IDE 2 documentation: <https://docs.arduino.cc/software/ide-v2/>
- Installing Arduino libraries: <https://support.arduino.cc/hc/en-us/articles/5145457742236-Add-libraries-to-Arduino-IDE>
- Arduino-ESP32 installation: <https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html>
- Arduino-ESP32 Tools menu: <https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/tools_menu.html>
