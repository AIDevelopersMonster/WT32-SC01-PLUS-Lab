# Panlee ESP32-TUX baseline build procedure

This procedure is for the Project 4 reference specimen only:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC
230208
16 MiB Flash / 2 MiB embedded PSRAM
```

It intentionally separates **build** from **flash**. Do not flash merely because compilation succeeds.

## 1. Clone the recorded upstream revision

From a working directory outside `WT32-SC01-PLUS-Lab`:

```powershell
git clone --recursive https://github.com/sukesh-ak/ESP32-TUX.git ESP32-TUX-Panlee
git -C ESP32-TUX-Panlee checkout 47639648a37ffc9ef9c2a748eeb9761894b9238a
git -C ESP32-TUX-Panlee submodule update --init --recursive
```

Verify:

```powershell
git -C ESP32-TUX-Panlee rev-parse HEAD
git -C ESP32-TUX-Panlee status --short
```

Expected revision:

```text
47639648a37ffc9ef9c2a748eeb9761894b9238a
```

## 2. Start with the upstream toolchain expectation

The recorded upstream `dependencies.lock` identifies:

```text
target: esp32s3
idf: 5.1.2
```

If the lab uses ESP-IDF 6.0.2, first try a build and record compatibility failures. Do not silently rewrite the application before preserving that evidence.

Activate the installed ESP-IDF environment in the normal way for the host, then:

```powershell
cd ESP32-TUX-Panlee
idf.py set-target esp32s3
```

## 3. Apply the Panlee configuration delta

The lab overlay is stored at:

```text
third-party/esp-idf/esp32-tux/our-version/sdkconfig.panlee-v15.defaults
```

For a reproducible experiment, copy the upstream `sdkconfig.defaults` to a Panlee-specific defaults file and change only these baseline selections:

```text
CONFIG_TUX_DEVICE_WT32_SC01_PLUS=y
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="16MB"
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions/partition-16MB.csv"
CONFIG_PARTITION_TABLE_FILENAME="partitions/partition-16MB.csv"
CONFIG_PROV_TRANSPORT_SOFTAP=y
CONFIG_PROV_TRANSPORT=2
CONFIG_TIMEZONE_STRING="MSK-3"
```

Do not commit Wi-Fi credentials or API keys.

### Why `MSK-3`

ESP-IDF uses POSIX `TZ` strings via `setenv("TZ", ...)` and `tzset()`. In POSIX TZ notation the numeric sign is opposite the ordinary UTC-offset notation, so fixed Moscow UTC+3 with no DST is represented here as `MSK-3`.

## 4. Keep the baseline partition geometry

At the recorded upstream revision, `partition-8MB.csv` and `partition-16MB.csv` contain the same partition sizes. For the first Panlee run, this is desirable: it leaves the extra physical Flash unused and avoids combining a hardware-profile test with a storage-layout redesign.

Do not enlarge OTA or SPIFFS partitions until the baseline is physically stable.

## 5. Build only

```powershell
idf.py fullclean
idf.py build
```

Record:

```powershell
idf.py --version
git rev-parse HEAD
```

Keep the complete build log if ESP-IDF 6.x reports API compatibility errors.

## 6. Inspect before flash

A successful build is not permission to flash yet. Check:

- the selected target is `esp32s3`;
- the configured flash size is 16 MB;
- the generated partition table uses the expected offsets;
- bootloader, partition-table and application binaries all fit their assigned regions;
- no command proposes an erase of the full 16 MB device unless deliberately requested.

Useful inspection commands depend on the active ESP-IDF version. The normal `idf.py build` output should show the application-size/partition fit result. Preserve that output in the Project 4 evidence record.

## 7. First flash gate

Only after the build/partition review passes, flash through the normal development port:

```powershell
idf.py -p COM10 flash monitor
```

Replace `COM10` if Windows assigns another port.

For the first run, collect serial output from reset through UI startup. Stop and preserve the log if the firmware repeatedly resets, reports partition errors, or fails before display initialization.

## 8. Physical validation order

Use this order so a later subsystem cannot hide an earlier failure:

1. boot stability;
2. LCD/LVGL rendering;
3. touch coordinates and UI interaction;
4. brightness;
5. portrait/landscape rotation;
6. light/dark themes;
7. SoftAP Wi-Fi provisioning;
8. reconnect after reboot;
9. SPIFFS;
10. SD card;
11. OTA only as a separate deliberate test.

Update `../evidence/README.md` only with observations actually seen on the physical specimen.
