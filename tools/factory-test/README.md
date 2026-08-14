# Recovered factory test / FactoryCTL

This directory documents and reproduces the factory-test path recovered from the original firmware of the reference **Panlee ZX3D50CE08S-V15-USRC**, marking `230208`.

It is both a small safe JTAG tool and a worked example of practical firmware/hardware reverse engineering: starting from a flash image and a live ESP32-S3 target, we recovered the hidden production-test entry, reconstructed the test order, identified the interfaces used by the tests, and physically executed several of the original factory diagnostics without reflashing the board.

Detailed reverse-engineering evidence is kept in:

- [`../../evidence/specimens/panlee-v15-230208-sample-a/factory-mode-reverse-engineering.md`](../../evidence/specimens/panlee-v15-230208-sample-a/factory-mode-reverse-engineering.md)
- [`../../evidence/specimens/panlee-v15-230208-sample-a/uart-protocol-command-map.md`](../../evidence/specimens/panlee-v15-230208-sample-a/uart-protocol-command-map.md)

## Reference firmware

Known full-flash SHA-256:

```text
3772c1bf7d6d2b713973212ddf5c671e3c844a13a8464f675343d9aed4e7f044
```

The recovered application identifies itself as:

```text
project: get-start
version: 1
build date: Feb 14 2023
build time: 14:32:37
ESP-IDF: v4.4.4-dirty
```

All addresses and protocol claims in this document are scoped to this verified specimen/firmware unless stated otherwise.

## What was recovered

The original firmware contains a hidden production-test branch and a fixed local test runner. The runner order is:

```text
0x42007060  AUDIO launcher
0x42007063  DISPLAY / LCD interactive test
0x42007066  TOUCH 44-target test
0x42007069  IO external-fixture test
0x4200706C  SD-card test
0x4200706F  USB Con / USB Dis electrical-fixture test
0x42007072  final green OK function
0x42007075  runner return
```

Recovered stage functions:

```text
AUDIO launcher   0x42007140
AUDIO task       0x420070FC
DISPLAY          0x420069C0
TOUCH            0x42006AAC
IO               0x42006D14
SD               0x42006DF0
USB              0x42006E54
OK               0x42006FC0
```

### Physical validation reached so far

| Stage / mechanism | Status on reference specimen | Notes |
|---|---|---|
| Hidden factory entry by volatile JTAG state | PASS | No flash/eFuse writes |
| FactoryCTL `show-ok` | PASS | Green `OK` on black, return at `0x42007075` |
| DISPLAY | PASS | RGB/grayscale inspection and touch-advanced blue/green/red sequence |
| TOUCH | PASS | Initial touch reveals targets; all 44 targets completed and turned green |
| DISPLAY -> TOUCH boundary reuse | PASS | Reuses halted `0x42007066` factory session without reset |
| AUDIO | PASS | Embedded MP3 audibly played through onboard audio path |
| SD factory path | Recovered; pass path previously observed | Full write/rename/read cycle is not certified because the factory routine has weak post-mount error propagation |
| IO | Not physically reproduced | Needs six external one-hot GPIO states |
| USB Con / USB Dis | Not executed under built-in USB-JTAG | Test intentionally remuxes GPIO19/GPIO20 used by the debugger |
| Native external RS-485 factory entry | NOT YET VERIFIED | Planned when a USB-RS485 adapter/cable is available |

### Physical demo video

A YouTube Shorts recording shows the recovered factory diagnostics being exercised on the reference board, including the factory audio playback, display/graphics test, touch-target test and other demo-visible stages invoked through the JTAG/FactoryCTL workflow:

- [Recovered WT32-SC01-PLUS factory tests — physical demo](https://www.youtube.com/shorts/ZQQNs8BSU0c)

The video is a **JTAG-assisted demonstration of recovered factory routines**, not yet proof of a complete native production-fixture run. Native entry through the reconstructed RS-485 service channel remains a separate future validation step.

This is the current stopping point of the reverse-engineering example. The next important experiment is to enter the same factory test through its **native RS-485 service channel**, with no JTAG selector injection.

## FactoryCTL v0.1.1

FactoryCTL is a conservative host-side runner for isolated recovered stages.

It contains:

- no flash erase/write operation;
- no eFuse write operation;
- no force override on firmware-signature mismatch;
- a bounded retry for the transient factory-entry breakpoint;
- OpenOCD Tcl RPC control on port `6666` by default;
- exact reuse of a halted stage boundary when the next test begins at the same address;
- refusal of tests that are unsafe or meaningless without the original fixture.

Start OpenOCD in a separate terminal using the ESP32-S3 built-in USB Serial/JTAG interface. The current lab configuration uses `board/esp32s3-builtin.cfg`.

Then, from the repository root:

```powershell
py tools/factory-test/factoryctl.py list
py tools/factory-test/factoryctl.py status
py tools/factory-test/factoryctl.py show-ok
py tools/factory-test/factoryctl.py run display
py tools/factory-test/factoryctl.py run touch
py tools/factory-test/factoryctl.py run audio
py tools/factory-test/factoryctl.py run sd
```

`run io`, `run usb`, and `run full` are intentionally refused in v0.1.

### Current operator notes

DISPLAY:

```text
combined RGB/grayscale pattern
  touch -> BLUE
  touch -> GREEN
  touch -> RED
  touch -> return
```

TOUCH:

```text
start stage
  first touch -> 44 target squares appear
  complete all target regions
  red targets -> green
  after final target -> return
```

AUDIO is asynchronous. FactoryCTL confirms the launcher return and lets the scheduler run for an observation window. Direct later calls in the recovered factory runner are skipped, but other application/UI tasks may still update parts of the LCD. Therefore the audio test certifies audible playback, **not** an invariant screen image.

## Reconstructed native factory-entry channel

The original hidden factory selector does not use USB Serial/JTAG. Static and live analysis reconstructs a separate service channel:

```text
UART1
115200 baud
8 data bits
no parity
1 stop bit
RS-485 half-duplex

TX  = GPIO42
RX  = GPIO1
RTS = GPIO2
```

The board firmware contains a custom `UART_Protocol`; it is not Modbus RTU even though its CRC calculation uses the reflected `0xA001` polynomial commonly associated with CRC-16/MODBUS.

### Frame format

```text
Offset  Size  Meaning
0       1     0xAA
1       1     0x55
2       2     total frame length, big-endian
4       2     command, big-endian
6       N     payload
6+N     2     CRC16, big-endian serialization
```

CRC parameters:

```text
poly   = 0xA001
init   = 0xFFFF
xorout = 0x0000
```

### Exact boot probe observed on the live board

A JTAG breakpoint immediately before the UART transmitter captured the exact 9-byte request generated by this firmware during the startup factory-selection window:

```text
AA 55 00 09 00 FF 00 63 5F
```

Decoded:

```text
sync       = AA 55
length     = 0x0009
command    = 0x00FF
payload    = 00
CRC        = 0x635F
```

This byte sequence is **directly observed evidence** for the board -> fixture direction.

### What is still unknown

The exact factory-fixture reply has not yet been captured.

The selector's receive path accepts a decoded result whose first selector byte is `0xFF`, so a valid response must ultimately satisfy that gate. However, the evidence does **not** yet prove that the fixture simply echoes the request above, and it does not prove the exact response payload.

Therefore the following is only a first experimental hypothesis, not a documented protocol fact:

```text
board -> fixture: AA 55 00 09 00 FF 00 63 5F   # exact observed probe
fixture -> board: valid AA55/CRC frame decoding to selector 0xFF
```

For the first USB-RS485 experiment, an exact echo of the observed 9-byte frame is a reasonable **test candidate**, but it must remain labelled experimental until a normal factory entry is observed or traffic from an original/compatible fixture is captured.

## Hypothesis for the complete stock factory test

Based on the recovered runner and physical execution, the likely factory workflow is:

```text
1. Power board with production firmware.
2. Board starts UART1/RS-485 service protocol at 115200 8N1.
3. During a bounded startup window the board repeatedly sends the 0x00FF probe.
4. Production fixture replies with a valid frame that satisfies the 0xFF selector gate.
5. Firmware prints/enters "Enter test mode" and starts the local runner.
6. AUDIO task is launched; audio plays asynchronously.
7. Operator performs DISPLAY visual test and advances color screens by touch.
8. Operator performs the 44-target TOUCH coverage test.
9. External fixture drives GPIO10/11/12/13/14/21 one-at-a-time so all six one-hot states are observed.
10. SD stage mounts the card and performs its factory file-system routine.
11. External fixture performs USB D+/D- "USB Con" and "USB Dis" electrical states.
12. Firmware displays green "OK" and returns from the runner.
```

This reconstruction is strongly supported for the **order and local stage behavior**, but the external production fixture itself has not been reproduced.

### Important consequence for the future USB-RS485 test

A USB-RS485 adapter alone should be enough to test whether we can enter factory mode **natively**, and should let us reproduce the early AUDIO/DISPLAY/TOUCH stages if the handshake is correct.

It is probably **not enough to complete the entire factory sequence**. The recovered runner blocks at the IO stage until all six one-hot external GPIO states have been seen, and later expects the USB electrical fixture states. A full native PASS will therefore require some combination of:

- USB-RS485 adapter for the startup handshake;
- a small GPIO fixture for GPIO10/11/12/13/14/21;
- an expendable SD card;
- a safe external arrangement for the USB Con/Dis line test.

That future fixture is intentionally left as the next phase of the project rather than guessed into existence now.

## Safety notes

- Do not generalize these addresses or pin assignments to every WT32-SC01-PLUS/OEM revision.
- Keep a verified full-flash backup before any invasive experiment.
- `run sd` may create/replace `/sdcard/hello.txt` and `/sdcard/foo.txt`; use an expendable card.
- Do not execute the recovered USB factory stage while using the ESP32-S3 built-in USB Serial/JTAG connection: the test deliberately remuxes GPIO19/GPIO20.
- The green `OK` function can be executed in isolation; seeing it that way is not proof that IO/USB passed.
- The proposed RS-485 reply is a hypothesis until verified by a native run/capture.

## Current milestone

```text
FACTORY_TEST_STRUCTURE          RECOVERED
JTAG_DIAGNOSTIC_RUNNER          PHYSICALLY VALIDATED FOR OK/DISPLAY/TOUCH/AUDIO
RS485_BOOT_PROBE                EXACT REQUEST CAPTURED
RS485_FIXTURE_REPLY             OPEN
FULL_NATIVE_FACTORY_ENTRY       OPEN
FULL_PRODUCTION_FIXTURE         OPEN
```

This is a deliberate stopping point. The reverse-engineering example already demonstrates that the original factory firmware can be turned into a diagnostic instrument and that the composition/order of the hidden production tests can be reconstructed. The next phase starts when USB-RS485 hardware is available.
