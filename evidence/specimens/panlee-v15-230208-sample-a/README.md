# Evidence — panlee-v15-230208-sample-a

Reference markings:

- Panlee
- ZX3D50CE08S-V15-USRC
- 230208

## Acceptance status

| Stage | Status | Evidence |
|---|---|---|
| HW-00 | IN PROGRESS | identity/photos being collected |
| HW-01 | **PASS** | [`hw01-chip/factory-flash-analysis.md`](hw01-chip/factory-flash-analysis.md) |
| HW-02 | TODO | display hardware validation |
| HW-03 | TODO | touch hardware validation |

## HW-01 verified facts

For this physical specimen:

- ESP32-S3 QFN56, revision v0.2;
- 40 MHz crystal;
- 16 MiB SPI Flash detected;
- Flash manufacturer/device ID: `0x5E / 0x4018`;
- Quad Flash interface, 3.3 V;
- embedded PSRAM: 2 MiB (`AP_3v3`);
- Secure Boot disabled in captured eFuse state;
- Flash Encryption disabled in captured eFuse state;
- USB Serial/JTAG and download mode available;
- complete factory Flash backup acquired twice;
- both 16 MiB reads match bit-for-bit by SHA-256.

Verified factory-image SHA-256:

```text
3772C1BF7D6D2B713973212DDF5C671E3C844A13A8464F675343D9AED4E7F044
```

The vendor firmware binary itself is kept outside the public Git repository until redistribution rights are established.
