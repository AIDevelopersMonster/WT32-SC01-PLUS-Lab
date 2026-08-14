#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_idf_version.h"

static const char *reset_reason_name(esp_reset_reason_t reason)
{
    switch (reason) {
    case ESP_RST_UNKNOWN:   return "UNKNOWN";
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXTERNAL_PIN";
    case ESP_RST_SW:        return "SOFTWARE";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "OTHER_WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    case ESP_RST_USB:       return "USB";
    case ESP_RST_JTAG:      return "JTAG";
    case ESP_RST_EFUSE:     return "EFUSE";
    case ESP_RST_PWR_GLITCH:return "POWER_GLITCH";
    case ESP_RST_CPU_LOCKUP:return "CPU_LOCKUP";
    default:                return "UNRECOGNIZED";
    }
}

static void print_mac(const uint8_t mac[6])
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void print_feature(const char *name, bool present)
{
    printf("  %-20s : %s\n", name, present ? "yes" : "no");
}

void app_main(void)
{
    esp_chip_info_t chip_info = {0};
    esp_chip_info(&chip_info);

    const esp_app_desc_t *app = esp_app_get_description();
    esp_reset_reason_t reset_reason = esp_reset_reason();

    uint8_t base_mac[6] = {0};
    esp_err_t mac_err = esp_read_mac(base_mac, ESP_MAC_BASE);

    uint32_t flash_id = 0;
    uint32_t flash_size = 0;
    esp_err_t flash_id_err = esp_flash_read_id(esp_flash_default_chip, &flash_id);
    esp_err_t flash_size_err = esp_flash_get_size(esp_flash_default_chip, &flash_size);

    size_t psram_size = esp_psram_get_size();
    size_t internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t spiram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    printf("\n");
    printf("============================================================\n");
    printf(" WT32-SC01-PLUS-Lab / 00_identity_probe\n");
    printf(" Passive runtime identity report - no external GPIO writes\n");
    printf("============================================================\n\n");

    printf("[SOFTWARE]\n");
    printf("  ESP-IDF              : %s\n", esp_get_idf_version());
    printf("  Project              : %s\n", app->project_name);
    printf("  App version          : %s\n", app->version);
    printf("  Build date/time      : %s %s\n", app->date, app->time);
    printf("  IDF target           : %s\n", CONFIG_IDF_TARGET);

    printf("\n[CHIP]\n");
    printf("  Model                : ESP32-S3\n");
    printf("  Revision             : v%u.%u (raw=%u)\n",
           chip_info.revision / 100,
           chip_info.revision % 100,
           chip_info.revision);
    printf("  CPU cores            : %u\n", chip_info.cores);
    printf("  Feature flags        : 0x%08" PRIX32 "\n", chip_info.features);
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
    printf("  Base MAC             : ");
    if (mac_err == ESP_OK) {
        print_mac(base_mac);
        printf("\n");
    } else {
        printf("ERROR (%s)\n", esp_err_to_name(mac_err));
    }

    printf("\n[RESET]\n");
    printf("  Last reset reason    : %s (%d)\n", reset_reason_name(reset_reason), (int)reset_reason);

    printf("\n[FLASH]\n");
    if (flash_id_err == ESP_OK) {
        printf("  JEDEC/device ID      : 0x%06" PRIX32 "\n", flash_id & 0xFFFFFFU);
        printf("  Manufacturer byte    : 0x%02" PRIX32 "\n", (flash_id >> 16) & 0xFFU);
        printf("  Device bytes         : 0x%04" PRIX32 "\n", flash_id & 0xFFFFU);
    } else {
        printf("  JEDEC/device ID      : ERROR (%s)\n", esp_err_to_name(flash_id_err));
    }
    if (flash_size_err == ESP_OK) {
        printf("  Runtime flash size   : %" PRIu32 " bytes (%" PRIu32 " MiB)\n",
               flash_size, flash_size / (1024U * 1024U));
    } else {
        printf("  Runtime flash size   : ERROR (%s)\n", esp_err_to_name(flash_size_err));
    }

    printf("\n[PSRAM]\n");
    printf("  esp_psram_get_size   : %u bytes (%.2f MiB)\n",
           (unsigned)psram_size,
           (double)psram_size / (1024.0 * 1024.0));
    printf("  Heap SPIRAM total    : %u bytes\n", (unsigned)spiram_total);
    printf("  Heap SPIRAM free     : %u bytes\n", (unsigned)spiram_free);

    printf("\n[INTERNAL HEAP]\n");
    printf("  Total 8-bit internal : %u bytes\n", (unsigned)internal_total);
    printf("  Free 8-bit internal  : %u bytes\n", (unsigned)internal_free);
    printf("  Largest free block   : %u bytes\n", (unsigned)internal_largest);
    printf("  Minimum free heap    : %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    printf("\n[SAFETY]\n");
    printf("  External GPIO writes : none\n");
    printf("  Display/touch init   : none\n");
    printf("  NVS/filesystem writes: none\n");
    printf("  Wi-Fi/BLE started    : no\n");

    printf("\n[EXPECTED REFERENCE SPECIMEN]\n");
    printf("  Board marking        : Panlee / ZX3D50CE08S-V15-USRC / 230208\n");
    printf("  Expected chip        : ESP32-S3 QFN56 revision v0.2\n");
    printf("  Expected flash       : 16 MiB, ID 0x5E4018\n");
    printf("  Expected PSRAM       : 2 MiB embedded Quad PSRAM\n");

    printf("\n============================================================\n");
    printf(" END 00_identity_probe - copy the complete report to evidence\n");
    printf("============================================================\n");
}
