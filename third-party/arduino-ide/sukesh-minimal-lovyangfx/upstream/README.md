# Upstream reference

Canonical source:

https://gist.github.com/sukesh-ak/610508bc84779a26efdcf969bf51a2d1

Author: Sukesh Akhilesh (`sukesh-ak`)

Checked: 2026-08-18

Observed upstream characteristics:

- Arduino IDE
- Board: `ESP32S3 Dev Module`
- LovyanGFX
- ST7796 display controller
- 8-bit parallel MCU8080 bus
- FT5x06-family touch driver
- Simple touch-drawing demonstration
- Last active: 2026-06-10
- 16 stars / 2 forks at the checked snapshot

## Physical validation on our specimen

The original sketch was compiled and uploaded on 2026-08-18 to:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC / 230208
```

Result:

- Arduino compile: **PASS**
- upload: **PASS**
- ST7796 display: **PHYSICAL PASS**
- touch coordinates: **PHYSICAL PASS**
- touch drawing: **PHYSICAL PASS**
- LovyanGFX dependency: **PHYSICALLY VALIDATED with this sketch/specimen**

Video evidence:

https://youtube.com/shorts/5CkP_Jh4ofo

Detailed record: [`../evidence/README.md`](../evidence/README.md)

## Redistribution note

No explicit license was identified on the Gist page during the 2026-08-18 review. The upstream `.ino` file is therefore intentionally not mirrored in this repository.

Use the canonical link above to inspect or download the original directly from its author.

This directory may later hold a revision manifest, checksums, compatibility notes, or an attributed source snapshot only if redistribution rights are made explicit.
