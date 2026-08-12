# Evidence

This directory contains reproducible evidence tied to physical specimens: serial dumps, eFuse/chip summaries, firmware hashes, measurements, acceptance JSON, calibration values and test logs.

Recommended layout:

```text
evidence/specimens/<specimen-id>/
├── hw00-identity/
├── hw01-chip/
├── hw02-display/
├── hw03-touch/
└── ...
```

Do not commit secrets, Wi-Fi credentials, private MAC inventories or vendor firmware unless redistribution is allowed.
