#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_mac.h"
#include "esp_psram.h"
#include "esp_system.h"

#include "reference_baseline.h"

static const char *reset_reason_name(esp_reset_reason_t reason)
{
    switch (reason) {
    case ESP_RST_UNKNOWN:    return "UNKNOWN";
    case ESP_RST_POWERON:    return "POWERON";
    case ESP_RST_EXT:        return "EXTERNAL_PIN";
    case ESP_RST_SW:         return "SOFTWARE";
    case ESP_RST_PANIC:      return "PANIC";
    case ESP_RST_INT_WDT:    return "INT_WDT";
    case ESP_RST_TASK_WDT:   return "TASK_WDT";
    case ESP_RST_WDT:        return "OTHER_WDT";
    case ESP_RST_DEEPSLEEP:  return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:   return "BROWNOUT";
    case ESP_RST_SDIO:       return "SDIO";
    case ESP_RST_USB:        return "USB";
    case ESP_RST_JTAG:       return "JTAG";
    case ESP_RST_EFUSE:      return "EFUSE";
    case ESP_RST_PWR_GLITCH: return "POWER_GLITCH";
    case ESP_RST_CPU_LOCKUP: return "CPU_LOCKUP";
    default:                 return "UNRECOGNIZED";
    }
}

static void print_mac(const uint8_t mac[6])
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void print_feature(const char *name, bool present)
{
    printf("  %-24s : %s\n", name, present ? "yes" : "no");
}

static const char *match_word(bool ok)
{
    return ok ? "MATCH" : "MISMATCH";
}

void app_main(void)
{
    esp_chip_info_t chip_info = {0};
    esp_chip_info(&chip_info);

    const esp_app_desc_t *app = esp_app_get_description();
    const esp_reset_reason_t reset_reason = esp_reset_reason();

    uint8_t base_mac[6] = {0};
    const esp_err_t mac_err = esp_read_mac(base_mac, ESP_MAC_BASE);

    uint32_t flash_id = 0;
    uint32_t flash_size = 0;
    const esp_err_t flash_id_err = esp_flash_read_id(esp_flash_default_chip, &flash_id);
    const esp_err_t flash_size_err = esp_flash_get_size(esp_flash_default_chip, &flash_size);

    const size_t psram_size = esp_psram_get_size();
    const size_t internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const size_t internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const size_t spiram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    const size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    const bool chip_model_match = chip_info.model == REF_CHIP_MODEL;
    const bool chip_revision_match = chip_info.revision == REF_CHIP_REVISION_RAW;
    const bool flash_id_match = flash_id_err == ESP_OK && ((flash_id & 0xFFFFFFU) == REF_FLASH_ID);
    const bool flash_size_match = flash_size_err == ESP_OK && flash_size == REF_FLASH_SIZE_BYTES;
    const bool psram_size_match = psram_size == REF_PSRAM_SIZE_BYTES;
    const bool identity_match = chip_model_match && chip_revision_match && flash_id_match && flash_size_match && psram_size_match;

    printf("\n");
    printf("================================================================\n");
    printf(" WT32-SC01-PLUS-Lab / 00_identity_probe\n");
    printf(" Runtime identity cross-check - no external GPIO writes\n");
    printf("================================================================\n\n");

    printf("[REFERENCE]\n");
    printf("  Specimen ID              : %s\n", REF_SPECIMEN_ID);
    printf("  Board marking            : %s\n", REF_BOARD_MARKING);
    printf("  Factory app              : %s v%s\n", REF_FACTORY_PROJECT, REF_FACTORY_APP_VERSION);
    printf("  Factory ESP-IDF          : %s\n", REF_FACTORY_IDF_VERSION);
    printf("  Factory build            : %s %s\n", REF_FACTORY_BUILD_DATE, REF_FACTORY_BUILD_TIME);
    printf("  PSRAM evidence           : %s\n", REF_PSRAM_INTERFACE);

    printf("\n[SOFTWARE - THIS TEST]\n");
    printf("  ESP-IDF                  : %s\n", esp_get_idf_version());
    printf("  Project                  : %s\n", app->project_name);
    printf("  App version              : %s\n", app->version);
    printf("  Build date/time          : %s %s\n", app->date, app->time);
    printf("  IDF target               : %s\n", CONFIG_IDF_TARGET);

    printf("\n[CHIP]\n");
    printf("  Model enum               : %d\n", (int)chip_info.model);
    printf("  Revision                 : v%u.%u (raw=%u)\n",
           chip_info.revision / 100,
           chip_info.revision % 100,
           chip_info.revision);
    printf("  CPU cores                : %u\n", chip_info.cores);
    printf("  Feature flags            : 0x%08" PRIX32 "\n", chip_info.features);
#ifdef CHIP_FEATURE_WIFI_BGN
    print_feature("Wi-Fi 2.4 GHz", (chip_info.features & CHIP_FEATURE_WIFI_BGN) != 0);
#endif
#ifdef CHIP_FEATURE_BLE
    print_feature("Bluetooth LE", (chip_info.features & CHIP_FEATURE_BLE) != 0);
#endif
#ifdef CHIP_FEATURE_BT
    print_feature("Bluetooth Classic", (chip_info.features & CHIP_FEATURE_BT) != 0);
#endif
#ifdef CHIP_FEATURE_EMB_FLASH
    print_feature("Embedded flash", (chip_info.features & CHIP_FEATURE_EMB_FLASH) != 0);
#endif
#ifdef CHIP_FEATURE_EMB_PSRAM
    print_feature("Embedded PSRAM", (chip_info.features & CHIP_FEATURE_EMB_PSRAM) != 0);
#endif

    printf("\n[IDENTIFIERS]\n");
    printf("  Base MAC                 : ");
    if (mac_err == ESP_OK) {
        print_mac(base_mac);
        printf("\n");
    } else {
        printf("ERROR (%s)\n", esp_err_to_name(mac_err));
    }

    printf("\n[RESET]\n");
    printf("  Last reset reason        : %s (%d)\n", reset_reason_name(reset_reason), (int)reset_reason);

    printf("\n[FLASH]\n");
    if (flash_id_err == ESP_OK) {
        printf("  JEDEC/device ID          : 0x%06" PRIX32 "\n", flash_id & 0xFFFFFFU);
        printf("  Manufacturer byte        : 0x%02" PRIX32 "\n", (flash_id >> 16) & 0xFFU);
        printf("  Device bytes             : 0x%04" PRIX32 "\n", flash_id & 0xFFFFU);
    } else {
        printf("  JEDEC/device ID          : ERROR (%s)\n", esp_err_to_name(flash_id_err));
    }
    if (flash_size_err == ESP_OK) {
        printf("  Runtime flash size       : %" PRIu32 " bytes (%" PRIu32 " MiB)\n",
               flash_size, flash_size / (1024U * 1024U));
    } else {
        printf("  Runtime flash size       : ERROR (%s)\n", esp_err_to_name(flash_size_err));
    }

    printf("\n[PSRAM]\n");
    printf("  esp_psram_get_size       : %u bytes (%.2f MiB)\n",
           (unsigned)psram_size,
           (double)psram_size / (1024.0 * 1024.0));
    printf("  Heap SPIRAM total        : %u bytes\n", (unsigned)spiram_total);
    printf("  Heap SPIRAM free         : %u bytes\n", (unsigned)spiram_free);

    printf("\n[INTERNAL HEAP]\n");
    printf("  Total 8-bit internal     : %u bytes\n", (unsigned)internal_total);
    printf("  Free 8-bit internal      : %u bytes\n", (unsigned)internal_free);
    printf("  Largest free block       : %u bytes\n", (unsigned)internal_largest);
    printf("  Minimum free heap        : %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    printf("\n[REFERENCE COMPARISON]\n");
    printf("  Chip model ESP32-S3      : %s\n", match_word(chip_model_match));
    printf("  Chip revision v0.2       : %s\n", match_word(chip_revision_match));
    printf("  Flash ID 0x%06X          : %s\n", REF_FLASH_ID, match_word(flash_id_match));
    printf("  Flash size 16 MiB        : %s\n", match_word(flash_size_match));
    printf("  PSRAM size 2 MiB         : %s\n", match_word(psram_size_match));
    printf("  Identity baseline        : %s\n", identity_match ? "MATCH" : "MISMATCH - INVESTIGATE");

    printf("\n[SAFETY]\n");
    printf("  External GPIO writes     : none by application\n");
    printf("  Display/touch init       : none\n");
    printf("  NVS/filesystem writes    : none\n");
    printf("  Wi-Fi/BLE started        : no\n");
    printf("  NOTE                     : flashing this app replaces factory firmware regions\n");

    printf("\n================================================================\n");
    printf(" END 00_identity_probe - save the complete serial log as evidence\n");
    printf("================================================================\n");
}
