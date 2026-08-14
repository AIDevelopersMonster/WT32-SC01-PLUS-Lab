# Evidence — panlee-v15-230208-sample-a

Reference markings:

- Panlee
- ZX3D50CE08S-V15-USRC
- 230208

## Acceptance status

| Stage | Status | Evidence |
|---|---|---|
| HW-00 | IN PROGRESS | identity/photos being collected |
| HW-01 | **PASS** | [`hw01-chip/factory-flash-analysis.md`](hw01-chip/factory-flash-analysis.md), [`00_identity_probe/README.md`](00_identity_probe/README.md) |
| HW-02 | **PASS** | [`01_display_test/README.md`](01_display_test/README.md) |
| HW-03 | TODO | touch hardware validation |
| HW-04 | **PASS — read path; media anomaly observed** | [`03_storage_test/README.md`](03_storage_test/README.md) |

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
- both 16 MiB reads match bit-for-bit by SHA-256;
- independent `00_identity_probe` runtime comparison passed on the physical board.

Verified factory-image SHA-256:

```text
3772C1BF7D6D2B713973212DDF5C671E3C844A13A8464F675343D9AED4E7F044
```

## HW-02 verified display facts

`01_display_test` independently initialized and drove the physical LCD from ESP-IDF 6.0.2 using the Espressif ST7796 component. The operator visually confirmed correct repeated diagnostic screens.

Validated for this specimen at the test settings:

- 480x320 landscape operation;
- ST7796-compatible initialization;
- ESP32-S3 8-bit I80 interface;
- 10 MHz I80 pixel clock;
- RGB565 transfers;
- active-high backlight on GPIO45;
- reset GPIO4;
- DC/RS GPIO0;
- WR GPIO47;
- data bus D0..D7 = GPIO9,46,3,8,18,17,16,15;
- repeated solid-color, color-bar, grayscale and orientation/geometry output.

TE GPIO48 was not exercised and remains outside the HW-02 PASS claim.

## HW-04 verified storage facts

`03_storage_test` independently initialized and read a real SDHC card through SDSPI without mounting a filesystem or writing card data.

Validated for this specimen at the test settings:

- SD CLK GPIO39;
- SD MOSI GPIO40;
- SD MISO GPIO38;
- SD CS GPIO41;
- SDSPI operation at 10 MHz;
- SDHC initialization;
- sector 0 read;
- MBR parsing;
- first partition boot-sector read;
- normal completion without panic/watchdog/read timeout.

The inserted media reported `106496000` CSD-addressable sectors while its MBR partition extends to sector `125829120` exclusive. The board-side read path therefore remains PASS, while the SD card / partition geometry is recorded separately as a media anomaly. See the canonical storage evidence protocol for details.

The vendor firmware binary itself is kept outside the public Git repository until redistribution rights are established.
