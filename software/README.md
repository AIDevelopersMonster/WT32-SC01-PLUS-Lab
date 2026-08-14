# Software assets

This directory contains **machine-consumable software configuration assets** used by WT32-SC01-PLUS-Lab.

It is intentionally separate from [`docs/software`](../docs/software/), which is the human-readable catalog of tools, installation notes and lab policy.

Current structure:

```text
software/
└─ espressif/
   ├─ README.md
   └─ config/
      ├─ README.md
      └─ panlee-v15-230208-sample-a.idf6.0.2.sdkconfig.defaults
```

Configuration profiles in this directory are tied to the specimen/revision and tool version named in the file or README. Do not silently generalize them to every board sold as WT32-SC01-PLUS.
