# Factory mode reverse engineering — Panlee ZX3D50CE08S-V15-USRC 230208

Date: 2026-08-14

Reference specimen:

- Panlee
- `ZX3D50CE08S-V15-USRC`
- lot/date marking `230208`
- ESP32-S3 QFN56 rev v0.2
- 16 MiB SPI Flash
- 2 MiB embedded PSRAM

Verified factory-image SHA-256:

```text
3772C1BF7D6D2B713973212DDF5C671E3C844A13A8464F675343D9AED4E7F044
```

This document records the state of the factory-firmware reverse engineering for this exact physical specimen. It must not be generalized automatically to every WT32-SC01-PLUS/OEM revision.

## Evidence levels

- **Level A** — directly observed on the physical reference specimen.
- **Level B** — recovered directly from the verified factory firmware for this specimen, including live JTAG register/memory observations.
- **Level C** — exact/close external source for the board family.
- **Level D** — inference only.

No factory-flash erase or programming was performed during the experiments described here. JTAG manipulation was limited to CPU debug state, breakpoints, registers and volatile RAM.

## Factory application identity

The factory application begins at flash offset `0x10000` and contains an ESP application descriptor with:

```text
project: get-start
version: 1
build date: Feb 14 2023
build time: 14:32:37
ESP-IDF: v4.4.4-dirty
```

Relevant embedded strings include:

```text
Version v2.0\r
Enter test mode\r
Fine qmsd!\r
IO Test
SD Test
USB Con
USB Dis
OK
```

## Top-level factory runner

The recovered runner at `0x42007008` calls the production tests in a fixed local sequence:

```text
0x42007060  call 0x42007140   AUDIO launcher
0x42007063  call 0x420069C0   LCD interactive test
0x42007066  call 0x42006AAC   touch-panel test
0x42007069  call 0x42006D14   external IO test
0x4200706C  call 0x42006DF0   SD test
0x4200706F  call 0x42006E54   USB D+/D- fixture test
0x42007072  call 0x42006FC0   final OK screen
0x42007075  return
```

The progression between these stages is local control flow: a stage returns and the next function is called. Several stages block waiting for operator or fixture actions. Therefore seeing the FreeRTOS idle task while a factory run is in progress does **not** by itself prove that the factory sequence has completed; a factory task may be blocked while the scheduler runs idle.

## Audio test — Level A + B verified

The factory runner launches the audio task at `0x42007140`; the task entry is `0x420070FC`.

The embedded MP3 is addressed directly by the live task:

```text
DROM start: 0x3C089988
DROM end:   0x3C0D0EED
size:       0x47565 = 292197 bytes
```

Live JTAG at the task showed:

```text
a3  = 0x3C089988
a2  = 0x3C0D0EED
a10 = 0x42007078
```

Recovered media properties are approximately 44.1 kHz, mono, 128 kbit/s and 18.23 s. The physical specimen audibly played the embedded asset through the onboard audio path.

## LCD interactive test — Level A + B verified

Function:

```text
0x420069C0
```

Static disassembly contains RGB565 constants and an operator-wait helper. Isolated execution on the physical board produced the following sequence:

```text
initial: combined RGB + grayscale inspection pattern
 touch -> BLUE   (RGB565 0x001F)
 touch -> GREEN  (RGB565 0x07E0)
 touch -> RED    (RGB565 0xF800)
 touch -> return
```

After the final touch, a hardware breakpoint immediately after the function was hit at:

```text
PC = 0x42007066
```

This establishes that `0x420069C0` is an interactive LCD production test, not a passive final screen.

The factory display path also contains ST7796 driver strings and the physical board reports `MADCTL=28`, consistent with a 320x480 native panel used in 480x320 landscape logical orientation.

## Touch-panel test — Level A + B verified

Function:

```text
0x42006AAC
```

The function constructs 44 target regions and repeatedly obtains touch coordinates through helper `0x42007628`. Each target accepts a touch in an approximately 30x30 pixel region.

Physical behavior on the reference specimen:

- many small red target squares are displayed;
- touching a target changes that target from red to green;
- a small white point marks the current touch position;
- dragging across the panel produces white points connected by a blue trace;
- after all 44 target squares become green, the function stops reacting because it has completed and returned.

Completion was confirmed by the next hardware breakpoint:

```text
PC = 0x42007069
```

Thus the factory touch test verifies both broad panel coverage and continuous coordinate tracking.

## External IO test — Level B verified

Function:

```text
0x42006D14
```

The displayed label is directly embedded at `0x3C0733F8`:

```text
IO Test
```

The six GPIO numbers are stored directly in the firmware:

```text
10, 11, 12, 13, 14, 21
```

The test calls firmware functions identified by embedded diagnostics as:

```text
gpio_set_pull_mode
gpio_set_direction
```

and reads the six GPIO levels through a helper that directly extracts the appropriate bit from the ESP32-S3 GPIO input registers.

The production algorithm builds a six-bit input mask and marks a channel passed only when the complete mask equals one of the six one-hot values:

```text
000001
000010
000100
001000
010000
100000
```

Each channel is remembered independently. The function returns only after all six one-hot states have been observed.

For the ESP-IDF enum values used by this firmware, the call arguments are consistent with the six pins being configured as inputs with internal pulldown. The exact external production fixture has not yet been reproduced physically.

## SD test — Level A + B verified for the factory pass path

Function:

```text
0x42006DF0
```

The physical specimen displayed red `SD Test` text on a black background. The function then returned and hit the breakpoint immediately before the USB test:

```text
PC = 0x4200706F
```

This is Level A evidence that the factory firmware considered the SD stage complete on that run.

### Recovered SDSPI wiring

The helper `0x4200715C` configures the SD interface in SPI mode:

```text
CLK  = GPIO39
MOSI = GPIO40
MISO = GPIO38
CS   = GPIO41
```

Mount point:

```text
/sdcard
```

### Intended file-system test

The firmware contains and executes an Espressif-style file-system cycle using:

```text
/sdcard/hello.txt
/sdcard/foo.txt
```

with embedded modes and log messages showing this intended sequence:

```text
mount filesystem
open /sdcard/hello.txt with "w"
write "Hello %s!\n"
close
remove old /sdcard/foo.txt if present
rename hello.txt -> foo.txt
open foo.txt with "r"
read a line into a 64-byte buffer
remove trailing newline
log the read string
unmount
```

### Factory-test weakness discovered

The outer SD stage repeats `0x4200715C` only while its return value is nonzero. Static control-flow tracing shows that SPI/bus initialization and filesystem-mount failures propagate as failure, but several later file-operation error paths can return with a zero status after mount has succeeded.

Therefore the actual factory pass criterion is weaker than the apparent intended test:

- SPI/bus initialization failure -> retry/fail;
- filesystem/card mount failure -> retry/fail;
- after successful mount, some write/open/rename/read-open failures can still return success to the outer factory stage.

Accordingly, a factory `SD Test` PASS must **not** be documented as proof that the complete write/rename/read cycle succeeded. It proves the factory code reached its own success return; the later file operations require separate validation if they are to be certified independently.

The test may delete/replace `/sdcard/foo.txt`, so only an expendable card should be used for future physical validation.

## USB Con / USB Dis fixture test — Level B verified

Function:

```text
0x42006E54
```

Embedded display labels:

```text
USB Con
USB Dis
```

The routine repurposes the ESP32-S3 native USB pins:

```text
GPIO20 = USB D+
GPIO19 = USB D-
```

Recovered behavior is consistent with an electrical production-fixture continuity test:

1. GPIO20 is configured as an output.
2. GPIO19 is configured as an input with pull-up behavior.
3. In the `USB Con` phase, GPIO20 is driven low/high and GPIO19 is expected to follow the same state, consistent with an external fixture connecting the two lines.
4. In the `USB Dis` phase, the fixture connection is expected to be removed; GPIO19 then returns high via its pull-up while GPIO20 is low.

This is not evidence of USB enumeration or USB packet-level testing. It is a physical-line factory test.

The test has **not** been executed physically during JTAG work because the debugger itself is connected through the ESP32-S3 built-in USB Serial/JTAG interface on GPIO19/GPIO20. Running the test could intentionally disable the active debug transport.

## Final OK function — Level A + B verified

Function:

```text
0x42006FC0
```

Static firmware evidence:

```text
text  = "OK"
color = RGB565 0x07E0
```

When invoked in isolation after skipping the USB test, the physical specimen displayed green `OK` text and returned to the next breakpoint:

```text
PC = 0x42007075
```

This verifies the final display function itself. It does **not** mean that IO and USB were passed during that isolated run, because those stages were deliberately skipped.

## Factory-entry UART / RS-485 service channel

The factory-selection routine begins at:

```text
0x420068B4
```

Recovered startup configuration uses a separate UART path from the Type-C USB Serial/JTAG connection:

```text
UART1
115200 8N1
TX  = GPIO42
RX  = GPIO1
RTS = GPIO2
```

The configured UART mode corresponds to the ESP-IDF RS-485 half-duplex mode used with RTS-controlled external transceiver direction.

The firmware creates a task named:

```text
uart_queue_task
```

which parses the custom framed `UART_Protocol` and passes decoded messages to the application.

### Frame format

The transmitter at `0x4200FB4C` and parser at `0x4200FC3C` establish this wire format:

```text
Offset  Size  Meaning
0       1     0xAA
1       1     0x55
2       2     total frame length, big-endian
4       2     command, big-endian
6       N     payload
6+N     2     CRC16, big-endian serialization
```

Constraints recovered from the parser:

```text
minimum frame length = 8 bytes
maximum frame length = 0x100 bytes
```

The parser searches the byte stream for `AA 55`, waits for a complete declared frame, calculates CRC over all bytes except the final two CRC bytes, compares the received CRC, extracts the 16-bit command and forwards the payload.

Relevant error strings include:

```text
UART_Protocol: Deal recv frame too slow
UART_Protocol: frame length > %d
UART_Protocol: Frame crc error
```

### CRC algorithm

The CRC helpers at `0x4201E18C` and `0x4201E1D0` use this 16-entry nibble table:

```text
0000 CC01 D801 1400 F001 3C00 2800 E401
A001 6C00 7800 B401 5000 9C01 8801 4400
```

This is the reflected CRC-16 algorithm with:

```text
polynomial = 0xA001
init       = 0xFFFF
xorout     = 0x0000
```

It is the CRC calculation commonly called CRC-16/MODBUS, but the surrounding QMSD UART protocol is **not Modbus RTU**. In this protocol the 16-bit CRC is serialized high byte first.

Equivalent pseudocode:

```text
crc = 0xFFFF
for each byte:
    crc = table[(crc ^ byte) & 0x0F] ^ (crc >> 4)
    crc = table[(crc ^ (byte >> 4)) & 0x0F] ^ (crc >> 4)
```

### Live-captured factory boot request

A hardware breakpoint was placed at `0x4200FB93`, immediately before the first UART write after frame construction. The physical board was reset and halted there.

Live registers/memory showed:

```text
PC = 0x4200FB93
payload length a4 = 1
payload pointer a3 = 0x3FCF3B1C
payload[0] = 0x00
```

The locally prepared header/CRC buffer was:

```text
AA 55 00 09 00 FF 63 5F
```

Because the payload is sent separately between the six-byte header and two-byte CRC, the exact nine-byte wire request is:

```text
AA 55 00 09 00 FF 00 63 5F
```

Decoded:

```text
sync    = AA 55
length  = 0x0009
command = 0x00FF
payload = 00
CRC     = 0x635F
```

This request is therefore directly recovered from the live factory firmware, not guessed from documentation.

### Entry handshake behavior

The factory selector initializes a zero payload byte, then repeatedly sends command `0x00FF` with a one-byte `00` payload during a bounded startup window. Each iteration includes approximately a 20 ms delay before polling the decoded receive queue.

The control-flow gate is:

```text
send factory request
wait
poll decoded receive queue
if no decoded message -> remain on normal path / continue polling
if decoded command low byte != 0xFF -> reject
if decoded command low byte == 0xFF -> print "Enter test mode" and run 0x42007008
```

The current selector checks the low byte of the decoded command. The exact original reply emitted by the vendor production fixture has **not yet been captured**. A syntactically valid frame whose decoded command has low byte `0xFF` satisfies the recovered gate, but that does not prove the original fixture echoes the board request.

### Why there is no separate RS-485 test in the runner

The strongest current interpretation is that RS-485 is the service/control channel used to discover the production fixture and enter factory mode. A successful bidirectional handshake would already exercise UART1 TX/RX, RTS direction control, the external RS-485 transceiver path, framing and CRC parsing.

Within the recovered top-level runner, there is no per-stage RS-485 `next test` command. Stage progression is local: LCD/touch wait for operator interaction, IO/USB wait for fixture electrical conditions, SD runs locally, and a function return advances to the next stage.

This explains why the runner contains explicit tests for LCD, touch, IO, SD and USB but no separate named `RS485 Test` stage.

## Current factory-test map

| Function | Stage | Evidence | Physical status |
|---|---|---|---|
| `0x42007140` | Audio launcher | A+B | audible playback verified |
| `0x420069C0` | LCD interactive RGB/grayscale test | A+B | complete interactive sequence verified |
| `0x42006AAC` | 44-point touch test + trajectory | A+B | all targets completed; return verified |
| `0x42006D14` | IO Test, GPIO 10/11/12/13/14/21 | B | fixture not reproduced |
| `0x42006DF0` | SD Test | A+B | factory success return observed; file-cycle caveat applies |
| `0x42006E54` | USB Con / USB Dis electrical test | B | intentionally not run over active USB-JTAG |
| `0x42006FC0` | final green `OK` | A+B | isolated function and return verified |

## Remaining open items

1. Capture or reconstruct the **original fixture reply** to command `0x00FF` and physically verify factory entry through a USB-RS485 adapter or equivalent fixture.
2. Enumerate other consumers/commands of the generic `UART_Protocol`; the protocol clearly supports commands beyond the single factory-entry use case.
3. Reproduce the six-line IO fixture safely and promote the IO stage to Level A.
4. Validate SD write/rename/read independently instead of relying on the factory stage's weak error propagation.
5. Devise an alternate debug/observation method before physically running `USB Con / USB Dis`, because that test reconfigures the same pins used by built-in USB Serial/JTAG.
6. Implement a host-side `factoryctl` with firmware-signature checks, no flash-write command, and explicit safety classes for tests that affect debug transport or require external fixtures.

## Safety / claim boundary

All addresses and protocol bytes in this document are specific to the verified 2023 factory image identified by the SHA-256 above. A future tool must refuse to use hard-coded execution addresses until the connected firmware has been positively matched to this image/version or independently mapped.
