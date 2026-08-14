# UART_Protocol command map / RS-485 service protocol

Date: 2026-08-14

Reference specimen: Panlee `ZX3D50CE08S-V15-USRC`, marking `230208`.

Factory image SHA-256:

```text
3772C1BF7D6D2B713973212DDF5C671E3C844A13A8464F675343D9AED4E7F044
```

This note records the first whole-IROM cross-reference audit of the recovered QMSD `UART_Protocol` implementation.

## Scope

The live executable IROM range was dumped through JTAG without writing flash:

```text
0x42000020 .. 0x42069D70
size = 0x69D50 = 433488 bytes
```

The binary was disassembled with `xtensa-esp32s3-elf-objdump`, producing a ~14 MB text disassembly. The complete disassembly was searched for references to the three protocol functions:

```text
0x4200FB4C  framed UART_Protocol transmit
0x4200FBAC  decoded-message receive/dequeue
0x4200FBDC  parser-to-application message enqueue helper
```

## Direct cross-reference result

Within the complete dumped `0x42000020..0x42069D70` IROM segment, only the following direct application calls were found.

### TX: `0x4200FB4C`

Single direct caller:

```text
0x420068F9  movi  a10, 255
0x420068FC  call8 0x4200FB4C
```

This is the factory-mode selector. Live capture already established the complete transmitted frame:

```text
AA 55 00 09 00 FF 00 63 5F
```

Decoded:

```text
command = 0x00FF
payload = 00
```

No second direct IROM call to `0x4200FB4C` was found.

### RX: `0x4200FBAC`

Single direct caller:

```text
0x42006907  movi  a11, 0
0x42006909  mov   a10, a1
0x4200690B  call8 0x4200FBAC
```

This is also in the factory-mode selector. The caller rejects an empty receive result and accepts the hidden factory branch only when the low byte of the decoded command stored at the output buffer is `0xFF`.

No second direct IROM call to `0x4200FBAC` was found.

### Parser delivery helper: `0x4200FBDC`

The only direct call found is internal to `uart_queue_task` after frame synchronization, length checking and CRC validation:

```text
0x4200FDFD  load frame command bytes
...
0x4200FE12  payload length = frame_length - 6
0x4200FE15  payload pointer = frame + 6
0x4200FE21  call8 0x4200FBDC
```

`0x4200FBDC` is therefore a generic parser-to-application queue helper, not a command-specific dispatcher. It packages the decoded command and payload for later retrieval by `0x4200FBAC`.

## Current command map

| Direction | Command | Payload | Direct application site | Recovered purpose | Status |
|---|---:|---|---|---|---|
| TX board -> fixture | `0x00FF` | `00` | `0x420068FC` | factory-fixture discovery / entry probe | Level B + live JTAG |
| RX fixture -> board | low byte must equal `0xFF` | not checked by factory gate | `0x4200690B` | authorize hidden factory-mode entry | Level B |

At this stage, **no additional directly called UART_Protocol application commands have been found in the dumped IROM segment**.

## Interpretation

The evidence currently favors this architecture:

```text
boot
  -> initialize UART1 / RS-485 half duplex
  -> repeatedly send command 0x00FF, payload 00
  -> uart_queue_task parses AA55/length/CRC frames
  -> decoded messages are placed in one application queue
  -> factory selector polls that queue
  -> decoded command low byte == FF => Enter test mode
  -> factory runner then advances locally through Audio/LCD/Touch/IO/SD/USB/OK
```

This argues against an RS-485 `NEXT TEST` command driving the recovered top-level factory sequence. The protocol layer is generic, but this particular application image currently shows only one direct protocol use: startup discovery/authorization of the production fixture.

## Important claim boundary

The cross-reference result is strong but deliberately scoped.

It proves that no other **direct references visible in the dumped IROM disassembly** were found for these exact functions. It does not yet exclude every possible use elsewhere because:

- a function could theoretically be invoked indirectly through a function pointer / `callx8`;
- executable IRAM segments at `0x40374000...` and `0x4037E0A0...` were not part of this IROM-only xref scan;
- a separate wrapper or alternate protocol implementation could exist without referencing these exact entry addresses.

Therefore the correct current statement is:

> In the complete factory IROM segment, the only direct application TX/RX users of the recovered `UART_Protocol` are the factory-entry selector paths for command `0x00FF`. No evidence of a direct RS-485 per-test progression command has been found.

## Next checks

1. Search both executable IRAM segments for indirect/literal references to `0x4200FB4C`, `0x4200FBAC` and protocol queue globals.
2. Search IROM/IRAM for references to the protocol globals around `0x3FC9914C`, `0x3FC99150`, `0x3FC99154` to identify alternate access paths.
3. Inspect initialization around `0x4200FACC` and the queue/task creation to recover the exact message struct and queue topology.
4. If a USB-RS485 adapter becomes available, capture the original fixture reply or test a syntactically valid reply under controlled conditions.
5. Keep the original factory flash untouched; all current work remains static/read-only or volatile JTAG instrumentation.
