# 06_RS485Test

Arduino physical-validation candidate for the Panlee `ZX3D50CE08S-V15-USRC / 230208` specimen.

## Evidence status

Factory-firmware reverse engineering recovered this UART/RS485 configuration:

```text
UART1
115200 8N1
TX  = GPIO42
RX  = GPIO1
RTS = GPIO2
mode = RS485 half duplex
```

These pins are therefore **factory-recovered evidence**, but are not promoted to Arduino `PHYSICAL PASS` until this test completes on the real board.

## What this test proves

A successful round trip exercises the complete path:

```text
ESP32-S3 UART1 TX
 -> GPIO42
 -> onboard RS485 transceiver
 -> external A/B pair
 -> external RS485 peer
 -> external A/B pair
 -> onboard RS485 transceiver
 -> GPIO1 RX
 -> ESP32-S3 UART1
```

GPIO2 is used as UART RTS for automatic half-duplex direction control.

The test deliberately does not initialize LCD, touch, SD, audio, Wi-Fi or LVGL.

## External peer

Use a USB-RS485 adapter, a second RS485 board, PLC/MCU test fixture, or another device capable of returning ASCII lines at **115200 8N1**.

Connect the differential bus according to the labels on the physical board/adapter:

```text
A <-> A
B <-> B
GND <-> GND   (recommended when the devices are separately powered)
```

If no data is received, one diagnostic step is to swap A/B because A/B naming conventions are unfortunately not universal between vendors. Do not change the recovered GPIO mapping for this check.

## Protocol

The board automatically transmits:

```text
WT32-RS485 PING 1
WT32-RS485 PING 2
...
```

The peer returns the same sequence number as:

```text
WT32-RS485 PONG 1
WT32-RS485 PONG 2
...
```

Example PASS output:

```text
[TX] WT32-RS485 PING 17
[RX] WT32-RS485 PONG 17
[PASS] RS485 round-trip sequence=17 RTT=23 ms
[PHYSICAL PASS CANDIDATE] TX + RTS direction + A/B + RX path completed.
```

A matching PONG is required. Receiving unrelated characters is useful diagnostic evidence but is not enough for PASS.

## Serial Monitor commands

The native USB Serial Monitor remains available at 115200 baud:

```text
ping
status
help
```

`status` prints sent/PASS/timeout/RX counters.

## Acceptance recommendation

For final physical acceptance, capture at least 20 consecutive matching round trips without timeout or corruption. Prefer an additional longer run (for example 1000 frames) before calling the interface stress-tested.

Claim ceiling after the first successful test: the named specimen's onboard RS485 path works at 115200 8N1 using UART1 TX=42, RX=1 and RTS=2. This does not prove maximum baud rate, long-cable performance, EMC margin, termination correctness for arbitrary networks, or compatibility with every WT32-SC01-PLUS OEM revision.
