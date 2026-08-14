#pragma once

#include <stdint.h>
#include "esp_chip_info.h"

/*
 * Runtime-comparable snapshot of facts already established for the physical
 * reference specimen by passive chip/flash tooling and factory-firmware RE.
 *
 * Specimen:
 *   Panlee / ZX3D50CE08S-V15-USRC / 230208
 *
 * Canonical evidence (do not replace these with this header):
 *   evidence/specimens/panlee-v15-230208-sample-a/hw01-chip/factory-flash-analysis.md
 *   evidence/specimens/panlee-v15-230208-sample-a/factory-mode-reverse-engineering.md
 *   tools/factory-test/README.md
 *
 * This header intentionally contains only values that 00_identity_probe can
 * independently re-measure at runtime. Package type, factory application
 * version, peripheral pinout, display/touch/audio behavior and factory-test
 * addresses remain documented in their canonical evidence files.
 */

#define REF_SPECIMEN_ID              "panlee-v15-230208-sample-a"
#define REF_BOARD_MARKING            "Panlee / ZX3D50CE08S-V15-USRC / 230208"

#define REF_CHIP_MODEL               CHIP_ESP32S3
#define REF_CHIP_REVISION_RAW        2U       /* MXX encoding: v0.2 */

#define REF_FLASH_ID                 0x5E4018U
#define REF_FLASH_SIZE_BYTES         16777216U /* 16 MiB */

#define REF_PSRAM_SIZE_BYTES         2097152U  /* 2 MiB */
#define REF_PSRAM_INTERFACE          "Quad / AP_3v3 (factory-tool evidence)"

#define REF_FACTORY_PROJECT          "get-start"
#define REF_FACTORY_APP_VERSION      "1"
#define REF_FACTORY_IDF_VERSION      "v4.4.4-dirty"
#define REF_FACTORY_BUILD_DATE       "Feb 14 2023"
#define REF_FACTORY_BUILD_TIME       "14:32:37"
