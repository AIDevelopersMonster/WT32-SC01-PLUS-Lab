# Arduino libraries: installation, structure, and writing your own

This guide explains what an Arduino library is, how Arduino IDE finds libraries, how to install them, and how to write a small reusable library yourself. The repository's real `WT32_SC01_PLUS` library is used as the main example.

## 1. What an Arduino library is

An Arduino library is a reusable package of C/C++ source code, metadata, and examples. A sketch usually sees only the public API:

```cpp
#include <WT32_SC01_PLUS.h>

WT32_SC01_PLUS board;

void setup() {
  Serial.begin(115200);
  board.begin();
}
```

The sketch does not need to know how the LCD bus, PWM backlight, I2S audio, or other low-level code is implemented internally.

A library is different from a board definition. A library provides code and APIs. The selected Arduino board/core determines the compiler, MCU family, upload tools, and board-specific build options.

For this project we deliberately use Espressif's existing generic ESP32-S3 board target instead of inventing a new Arduino board package.

## 2. Where Arduino libraries are stored

The most important location for user-installed libraries is the Arduino sketchbook `libraries` directory.

Typical Windows location:

```text
C:\Users\<USER>\Documents\Arduino\libraries\
```

A library installed there might look like:

```text
C:\Users\<USER>\Documents\Arduino\libraries\WT32_SC01_PLUS\
```

Arduino IDE also has libraries bundled with installed platforms/cores and with the IDE itself. If several libraries provide the same header name, Arduino's library resolution rules decide which candidate is used, so duplicate copies should be avoided.

## 3. Common installation methods

### Method A — Library Manager

For libraries published to the Arduino Library Manager index:

1. Open Arduino IDE.
2. Open **Library Manager**.
3. Search for the library name.
4. Select the required version.
5. Click **Install**.

This is the simplest method for normal public libraries.

### Method B — Add a ZIP library

If a library is distributed as a ZIP archive:

1. Download the ZIP.
2. In Arduino IDE choose **Sketch -> Include Library -> Add .ZIP Library...**.
3. Select the ZIP file.

Arduino copies the library into the sketchbook library area.

### Method C — Copy the folder manually

Copy the library directory directly into:

```text
<Sketchbook>\libraries\
```

Then restart Arduino IDE if the examples do not appear immediately.

### Method D — Development junction on Windows

During active development, copying the library after every change is inconvenient. This repository uses a better method: a Windows directory junction.

Example:

```powershell
New-Item -ItemType Junction `
  -Path C:\Users\CHUWI\Documents\Arduino\libraries\WT32_SC01_PLUS `
  -Target C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Arduino\libraries\WT32_SC01_PLUS
```

After that, Arduino IDE sees the Git working copy directly. Editing the repository changes the library used by Arduino IDE immediately.

Verify the link:

```powershell
Get-Item C:\Users\CHUWI\Documents\Arduino\libraries\WT32_SC01_PLUS
```

This is especially useful when the library is developed in Git and tested repeatedly on real hardware.

## 4. Typical Arduino library layout

A modern library normally uses a structure like this:

```text
MyLibrary/
├── library.properties
├── README.md
├── keywords.txt              # optional
├── src/
│   ├── MyLibrary.h
│   └── MyLibrary.cpp
└── examples/
    └── BasicExample/
        ├── BasicExample.ino
        └── sketch.yaml        # optional project metadata
```

Our project currently follows the same pattern:

```text
libraries/WT32_SC01_PLUS/
├── library.properties
├── README.md
├── src/
│   ├── WT32_SC01_PLUS.h
│   ├── WT32_SC01_PLUS.cpp
│   ├── WT32_SC01_PLUS_Display.cpp
│   └── WT32_SC01_PLUS_Pins.h
└── examples/
    └── 01_DisplayTest/
        ├── 01_DisplayTest.ino
        └── sketch.yaml
```

Additional validated modules are added incrementally rather than all at once.

## 5. `library.properties`

`library.properties` is metadata used by Arduino tooling.

Our current file contains fields such as:

```ini
name=WT32_SC01_PLUS
version=0.1.0
author=WT32-SC01-PLUS-Lab contributors
maintainer=WT32-SC01-PLUS-Lab contributors
category=Device Control
architectures=esp32
includes=WT32_SC01_PLUS.h
```

Important fields:

- `name` — library name shown to users;
- `version` — semantic version of the library;
- `author` / `maintainer` — project authorship/contact metadata;
- `sentence` / `paragraph` — short descriptions;
- `category` — Library Manager category;
- `url` — project homepage/repository;
- `architectures` — supported Arduino architectures;
- `includes` — headers Arduino may suggest when the library is included.

For our library `architectures=esp32` means the code is intended for the Arduino-ESP32 platform rather than AVR, SAMD, etc.

## 6. The `src` directory

The `src` directory contains the code compiled with the library.

### Public header

The main public header is:

```text
src/WT32_SC01_PLUS.h
```

A sketch includes it with:

```cpp
#include <WT32_SC01_PLUS.h>
```

This header declares the API visible to the application.

For example, our library exposes objects such as:

```cpp
WT32_SC01_PLUS board;

board.begin();
board.display();
board.backlight();
```

The goal is to make application code read in terms of board functions rather than GPIO register operations.

### Implementation files

Implementation can be split across several `.cpp` files:

```text
WT32_SC01_PLUS.cpp
WT32_SC01_PLUS_Display.cpp
WT32_SC01_PLUS_Audio.cpp
...
```

This keeps each subsystem isolated and easier to validate.

### Hardware profile header

This project also uses:

```text
WT32_SC01_PLUS_Pins.h
```

for specimen-specific GPIO mappings and related constants.

This is important because boards sold under the same commercial name may have different OEM revisions. A pin mapping should not be promoted into the validated profile until it has been physically checked on the target specimen.

## 7. The `examples` directory

Examples are normal Arduino sketches distributed with the library.

A valid example lives in its own directory and the `.ino` filename matches the directory name:

```text
examples/
└── 01_DisplayTest/
    └── 01_DisplayTest.ino
```

Installed examples become accessible from Arduino IDE through the library examples menu.

A good example should demonstrate one concept clearly. In this project we prefer subsystem bring-up examples such as:

```text
01_DisplayTest
02_TouchTest
03_BacklightTest
04_SDCardTest
05_AudioTest
06_RS485Test
99_SelfTest
```

The progression is intentional: test one subsystem at a time, physically validate it, then integrate it into larger tests.

## 8. `sketch.yaml` — project metadata beside an example

Arduino sketches can have a `sketch.yaml` file in the sketch directory.

For our examples the basic form is:

```yaml
default_fqbn: esp32:esp32:esp32s3
default_port_config:
  baudrate: 115200
```

This does **not** define a new board. It tells Arduino tooling that the sketch is intended to use Espressif's generic ESP32-S3 target and that the serial monitor should normally use 115200 baud.

We deliberately do not store a host-specific value such as:

```text
COM10
```

because the COM number can change between computers or USB connections.

A more complete FQBN can also encode board-menu choices once those settings have been physically validated.

## 9. Minimal example: writing a library yourself

Suppose we want a tiny library called `BlinkDevice`.

Directory:

```text
BlinkDevice/
├── library.properties
├── src/
│   ├── BlinkDevice.h
│   └── BlinkDevice.cpp
└── examples/
    └── Basic/
        └── Basic.ino
```

### `library.properties`

```ini
name=BlinkDevice
version=0.1.0
author=Example Author
sentence=Simple LED wrapper.
category=Device Control
architectures=*
includes=BlinkDevice.h
```

### `src/BlinkDevice.h`

```cpp
#pragma once

#include <Arduino.h>

class BlinkDevice {
public:
  explicit BlinkDevice(int pin);
  void begin();
  void set(bool on);

private:
  int pin_;
};
```

### `src/BlinkDevice.cpp`

```cpp
#include "BlinkDevice.h"

BlinkDevice::BlinkDevice(int pin) : pin_(pin) {}

void BlinkDevice::begin() {
  pinMode(pin_, OUTPUT);
  set(false);
}

void BlinkDevice::set(bool on) {
  digitalWrite(pin_, on ? HIGH : LOW);
}
```

### `examples/Basic/Basic.ino`

```cpp
#include <BlinkDevice.h>

BlinkDevice led(LED_BUILTIN);

void setup() {
  led.begin();
}

void loop() {
  led.set(true);
  delay(500);
  led.set(false);
  delay(500);
}
```

That is already a usable Arduino library.

## 10. How the same idea scales to `WT32_SC01_PLUS`

The board library is conceptually the same as the tiny `BlinkDevice` example, just divided into hardware subsystems.

Instead of writing application code like:

```cpp
pinMode(45, OUTPUT);
// configure I80 bus
// configure ST7796 registers
// allocate DMA buffers
// drive PWM
```

the sketch can use a board-level API:

```cpp
#include <WT32_SC01_PLUS.h>

WT32_SC01_PLUS board;

void setup() {
  board.begin();
  board.display().drawTestPattern();
  board.backlight().set(100);
}
```

The library becomes the hardware abstraction and validation layer.

Our development rule is:

```text
reverse-engineering evidence
        -> candidate mapping/driver
        -> isolated Arduino example
        -> compile validation
        -> physical validation
        -> documented PASS
        -> reusable library API
```

That approach prevents guessed pinouts or untested subsystem code from silently becoming part of the public BSP.

## 11. Library versus sketch

Use a **sketch** for application-specific behavior:

```text
weather station
internet radio
machine controller
sensor UI
```

Use a **library** for reusable functionality:

```text
LCD driver
board pin profile
touch interface
SD support
audio output
RS-485 interface
```

A useful rule is: if several sketches need the same code, that code probably belongs in a library.

## 12. Updating a library under Git

With a junction-based development setup, the normal workflow is:

```text
edit files in Git worktree
        -> Arduino IDE sees the same files
        -> Verify
        -> Upload
        -> physical test
        -> commit only the validated increment
```

This repository additionally uses CI compilation so a library example has both local and GitHub-side compile checks before or alongside physical validation.

## 13. Debugging library discovery

If Arduino IDE cannot find a header such as:

```text
WT32_SC01_PLUS.h: No such file or directory
```

check:

1. the library directory really exists under the sketchbook `libraries` directory;
2. `src/WT32_SC01_PLUS.h` exists;
3. there are no extra directory levels such as `WT32_SC01_PLUS/WT32_SC01_PLUS/src`;
4. Arduino IDE has been restarted or rescanned;
5. there is not another stale copy of the same library taking precedence;
6. the junction target still exists if a Windows junction is being used.

## 14. Recommended policy for this repository

For `WT32_SC01_PLUS` development:

- keep the library source in Git under `libraries/WT32_SC01_PLUS`;
- expose it to Arduino IDE through a junction rather than manual copying;
- place each hardware test under `examples/<TestName>`;
- place `sketch.yaml` beside each `.ino` when project metadata is useful;
- never hard-code a developer's COM port into shared metadata;
- keep OEM-specific pins explicitly tied to a validated board profile;
- do not merge an experimental peripheral into the validated baseline merely because it compiles;
- record physical PASS/FAIL separately from compile PASS/FAIL.

## Official Arduino references

- Arduino library specification: <https://docs.arduino.cc/arduino-cli/library-specification/>
- Arduino sketch specification: <https://docs.arduino.cc/arduino-cli/sketch-specification/>
- Installing libraries: <https://support.arduino.cc/hc/en-us/articles/5145457742236-Add-libraries-to-Arduino-IDE>
- Arduino sketch project file (`sketch.yaml`): <https://docs.arduino.cc/arduino-cli/sketch-project-file/>
