# UART_Protocol command map / RS-485 service protocol

Date: 2026-08-14

Reference specimen: Panlee `ZX3D50CE08S-V15-USRC`, marking `230208`.

Factory image SHA-256:

```text
3772C1BF7D6D2B713973212DDF5C671E3C844A13A8464F675343D9AED4E7F044
```

Status:

```text
FIVE_SEGMENT_STATIC_AUDIT_COMPLETE
APPLICATION_COMMAND_MAP: 0x00FF ONLY FOUND
PHYSICAL_FIXTURE_REPLY: NOT YET CAPTURED
```

This note records the cross-reference and raw-reference audit of the recovered QMSD `UART_Protocol` implementation in the verified factory application for this exact specimen. It must not be generalized automatically to other WT32-SC01-PLUS/OEM firmware revisions.

## Evidence basis

The factory application's five loaded segments are:

```text
DROM   0x3C070020 .. 0x3C1E2D54   size 0x172D34
DRAM   0x3FC95780 .. 0x3FC98994   size 0x003214
IRAM1  0x40374000 .. 0x4037E0A0   size 0x00A0A0
IROM   0x42000020 .. 0x42069D70   size 0x069D50
IRAM2  0x4037E0A0 .. 0x40385780   size 0x0076E0
```

The executable regions were dumped from the live target through JTAG without writing flash. The IROM dump was disassembled with `xtensa-esp32s3-elf-objdump`; both IRAM regions were also disassembled. A second audit scanned all five binary segments for raw little-endian 32-bit values corresponding to the UART protocol API entry points and globals.

## Recovered protocol API

```text
0x4200FB4C  framed UART_Protocol transmit
0x4200FBAC  decoded-message receive/dequeue
0x4200FBDC  parser-to-application message enqueue helper
0x4200FC3C  uart_queue_task / receive parser
```

Protocol state globals:

```text
0x3FC9914C  decoded-message/application queue handle
0x3FC99150  UART event queue handle / receive-event state
0x3FC99154  stored UART port number/state byte
```

The semantic labels for the globals are based on their observed initialization and use. The first is strongly identified as the queue shared by parser enqueue and application dequeue; the latter two are transport-layer state associated with the UART driver and `uart_queue_task`.

## Direct application cross-references

### TX: `0x4200FB4C`

Across the complete dumped IROM disassembly, there is one direct application call:

```text
0x420068F9  movi  a10, 255
0x420068FC  call8 0x4200FB4C
```

This is the factory-mode selector.

Live JTAG capture at `PC = 0x4200FB93`, immediately before the first UART write, established the exact transmitted request:

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

No second direct call to `0x4200FB4C` was found in the complete IROM disassembly.

### RX: `0x4200FBAC`

There is one direct application call:

```text
0x42006907  movi  a11, 0
0x42006909  mov   a10, a1
0x4200690B  call8 0x4200FBAC
```

This is also in the factory-mode selector. The caller rejects an empty receive result and enters the hidden factory branch only when the low byte of the decoded command stored in its output buffer is `0xFF`.

No second direct call to `0x4200FBAC` was found in the complete IROM disassembly.

### Parser delivery helper: `0x4200FBDC`

The only direct call found is internal to `uart_queue_task` after framing and CRC validation:

```text
0x4200FDFD  load command bytes from accepted frame
...
0x4200FE12  derive payload length from frame length
0x4200FE15  derive payload pointer from frame + 6
0x4200FE21  call8 0x4200FBDC
```

This function is therefore a generic parser-to-application queue helper, not a command-specific dispatcher.

## Executable-segment search result

A combined text search over the disassembled IROM, IRAM1 and IRAM2 regions for:

```text
4200FB4C
4200FBAC
4200FBDC
3FC9914C
3FC99150
3FC99154
```

returned protocol references only from the known IROM implementation. No matching protocol API/global references appeared in either IRAM disassembly.

Observed IROM use of the globals is confined to the known transport implementation:

```text
0x3FC9914C
  -> initialized around 0x4200FACC
  -> consumed by 0x4200FBAC
  -> used by 0x4200FBDC

0x3FC99150
  -> initialized by the protocol setup
  -> consumed by uart_queue_task / UART event handling

0x3FC99154
  -> initialized with the UART port
  -> used by TX and uart_queue_task UART operations
```

No second application consumer of the decoded-message queue was found.

## Raw function-pointer/reference audit

A binary scanner then searched all five loaded segments for exact little-endian 32-bit representations of:

```text
0x4200FB4C  UART_Protocol TX
0x4200FBAC  UART_Protocol RX/dequeue
0x4200FBDC  parser enqueue helper
0x3FC9914C  decoded-message queue global
0x3FC99150  UART event queue global
0x3FC99154  UART port/state global
```

Result:

```text
=== IROM ===
0x3FC9914C -> literal at 0x42000D6C
0x3FC99150 -> literal at 0x42000D74
0x3FC99154 -> literal at 0x42000D70

No absolute function-address literals for:
0x4200FB4C
0x4200FBAC
0x4200FBDC

=== IRAM1 ===
no target literals found

=== IRAM2 ===
no target literals found

=== DROM ===
no target literals found

=== DRAM ===
no target literals found
```

The absence of raw function-address values is meaningful because it finds no obvious function-pointer table or data-stored pointer that could invoke these protocol functions through an indirect `callx8` path. It does **not** conflict with the known direct `call8` instructions: Xtensa direct calls encode a relative displacement rather than storing the full target address as a 32-bit data literal.

## Current application command map

| Direction | Command | Payload | Application site | Recovered purpose | Status |
|---|---:|---|---|---|---|
| board -> fixture | `0x00FF` | `00` | `0x420068FC` | production-fixture discovery / factory-entry probe | Level B + live JTAG |
| fixture -> board | decoded command low byte must equal `0xFF` | not checked by factory gate | `0x4200690B` | authorize hidden factory-mode entry | Level B |

**No additional application-level UART_Protocol command was found in the five-segment static audit.**

## Architecture supported by the audit

```text
boot
  |
  +--> initialize UART1 / RS-485 half duplex
  |
  +--> send AA55 frame, CMD 0x00FF, payload 00
  |
  +--> uart_queue_task
  |      |
  |      +--> UART event queue
  |      +--> stream synchronization on AA 55
  |      +--> frame-length handling
  |      +--> CRC validation
  |      +--> command/payload extraction
  |      +--> decoded-message queue
  |
  +--> factory selector polls decoded-message queue
         |
         +--> no message / wrong low command byte -> do not enter factory mode
         |
         +--> command low byte == 0xFF
                |
                +--> "Enter test mode"
                +--> run local factory sequence
                     Audio -> LCD -> Touch -> IO -> SD -> USB -> OK
```

The evidence therefore argues strongly against a hidden RS-485 `NEXT TEST` command controlling the recovered top-level test sequence. The individual factory stages advance because each local test function eventually returns after its operator/fixture condition is satisfied.

## What the negative result does and does not prove

The completed audit gives strong binary-level evidence for the following statement:

> In the five loaded segments of this verified factory image, the only discovered application-level use of the recovered QMSD `UART_Protocol` API is the startup production-fixture discovery/authorization path using command `0x00FF`. No additional direct application consumers, no second users of the decoded-message queue, and no static absolute function-pointer references to the TX/RX protocol API were found.

The audit does **not** constitute a mathematical proof that no other UART/RS-485 behavior can exist. Residual theoretical possibilities include:

- a target address constructed arithmetically at runtime rather than stored as a literal;
- a wrapper or independently implemented UART protocol path that never references these exact API functions/globals;
- dynamically produced code/data behavior not represented by the loaded static segments.

No evidence for any of those alternatives has been found in this firmware.

## Interpretation

For this application image, `UART_Protocol` appears to be a reusable/general transport component whose application-level use is narrow: detect/authorize the production fixture at startup. The protocol's generic framing, 16-bit command field and payload support do not imply that this particular firmware implements a broad service command set.

This also explains why there is no named `RS485 Test` in the recovered factory runner. A successful request/reply handshake can already exercise the board-side UART1 TX/RX path, RTS-controlled half-duplex direction, framing and CRC parser before factory mode is entered.

The exact reply emitted by the original vendor fixture remains unknown. The factory selector accepts a valid decoded message whose command low byte is `0xFF`, but that condition alone does not establish what the original fixture actually transmits.

## Conclusion

For the verified Panlee `ZX3D50CE08S-V15-USRC` factory image:

```text
UART_PROTOCOL_COMMAND_MAP

0x00FF  FACTORY_FIXTURE_DISCOVERY / FACTORY_ENTRY

OTHER APPLICATION COMMANDS
NOT FOUND IN FIVE-SEGMENT STATIC AUDIT
```

This branch can be considered statically closed unless new evidence appears, such as an original production fixture capture, an alternate firmware build, or a newly identified independent UART service path.

## Remaining dynamic work

1. Capture the original RS-485 fixture reply when suitable hardware becomes available.
2. Physically verify that a correctly framed response satisfying the recovered gate enters factory mode without JTAG state manipulation.
3. Preserve the factory image unchanged and keep any host-side test tooling firmware-identity guarded.
