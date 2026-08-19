# ESP32-TUX physical evidence

Target specimen:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
```

## Evidence status

No ESP32-TUX firmware has yet been physically executed on this specimen in Project 4.

The source-level pin comparison is documented in the parent README, but it is not hardware evidence for the ESP32-TUX application.

## Evidence to record

For the first physical run, capture:

- exact upstream/adaptation commit or source revision;
- ESP-IDF version;
- full build command and result;
- generated flash size and partition table;
- upload command and offsets;
- boot/serial log;
- LCD/LVGL result;
- touch result;
- brightness/theme/orientation result;
- Wi-Fi provisioning result;
- SPIFFS result;
- SD result;
- OTA result only if deliberately tested;
- video URL once published.

Use `PASS`, `FAIL`, `PARTIAL`, or `NOT TESTED` per subsystem. Do not infer one subsystem from another.
