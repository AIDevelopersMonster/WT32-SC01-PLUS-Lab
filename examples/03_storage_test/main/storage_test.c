#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sdmmc_cmd.h"

static const char *TAG = "storage_test";

/*
 * Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208
 *
 * Recovered factory SDSPI wiring:
 *   CLK  GPIO39
 *   MOSI GPIO40
 *   MISO GPIO38
 *   CS   GPIO41
 *
 * This test is intentionally READ-ONLY:
 *   - no FAT mount
 *   - no fopen()/rename()/unlink()
 *   - no format operation
 *   - no sdmmc_write_sectors()
 *
 * It initializes the card, prints card metadata, reads sector 0, and if an
 * MBR partition is present reads only the first sector of the first partition.
 * It also checks whether declared MBR partition extents fit inside the capacity
 * reported by the card CSD. A mismatch is classified as a media anomaly, not
 * as a failure of the WT32-SC01-PLUS SDSPI hardware path.
 */
#define PIN_SD_CLK              39
#define PIN_SD_MOSI             40
#define PIN_SD_MISO             38
#define PIN_SD_CS               41
#define SD_TEST_FREQ_KHZ        10000
#define SECTOR_SIZE             512

static uint8_t s_sector[SECTOR_SIZE] __attribute__((aligned(4)));

typedef struct {
    uint32_t first_partition_lba;
    unsigned nonempty_partitions;
    uint64_t max_partition_end_exclusive;
    bool geometry_consistent;
} mbr_inspection_t;

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static bool has_55aa_signature(const uint8_t *sector)
{
    return sector[510] == 0x55 && sector[511] == 0xAA;
}

static void print_boot_sector_hint(const uint8_t *sector, uint32_t lba)
{
    char oem[9] = {0};
    memcpy(oem, &sector[3], 8);

    printf("\n[BOOT-SECTOR HINT @ LBA %" PRIu32 "]\n", lba);
    printf("  Signature 0x55AA          : %s\n", has_55aa_signature(sector) ? "yes" : "no");
    printf("  OEM / system field       : %.8s\n", oem);

    const uint16_t bytes_per_sector = le16(&sector[11]);
    const uint8_t sectors_per_cluster = sector[13];
    const uint16_t reserved = le16(&sector[14]);
    const uint8_t fats = sector[16];

    if (bytes_per_sector != 0) {
        printf("  Bytes per sector         : %u\n", bytes_per_sector);
        printf("  Sectors per cluster      : %u\n", sectors_per_cluster);
        printf("  Reserved sectors         : %u\n", reserved);
        printf("  FAT count                : %u\n", fats);
    }

    if (memcmp(&sector[3], "EXFAT   ", 8) == 0) {
        printf("  Filesystem hint          : exFAT\n");
    } else if (memcmp(&sector[82], "FAT32   ", 8) == 0) {
        printf("  Filesystem hint          : FAT32\n");
    } else if (memcmp(&sector[54], "FAT16   ", 8) == 0) {
        printf("  Filesystem hint          : FAT16\n");
    } else if (memcmp(&sector[54], "FAT12   ", 8) == 0) {
        printf("  Filesystem hint          : FAT12\n");
    } else {
        printf("  Filesystem hint          : not identified from standard label fields\n");
    }
}

static mbr_inspection_t inspect_mbr(const uint8_t *sector, uint64_t card_sector_count)
{
    mbr_inspection_t result = {
        .first_partition_lba = 0,
        .nonempty_partitions = 0,
        .max_partition_end_exclusive = 0,
        .geometry_consistent = true,
    };

    printf("\n[SECTOR 0 / PARTITION TABLE]\n");
    printf("  Signature 0x55AA          : %s\n", has_55aa_signature(sector) ? "yes" : "no");

    for (unsigned i = 0; i < 4; ++i) {
        const uint8_t *entry = &sector[446 + i * 16];
        const uint8_t type = entry[4];
        const uint32_t lba = le32(&entry[8]);
        const uint32_t count = le32(&entry[12]);

        if (type == 0 || count == 0) {
            continue;
        }

        const uint64_t end_exclusive = (uint64_t)lba + (uint64_t)count;
        const bool in_range = end_exclusive <= card_sector_count;

        ++result.nonempty_partitions;
        printf("  Partition %u             : type=0x%02X start=%" PRIu32
               " sectors=%" PRIu32 " end(exclusive)=%" PRIu64 " [%s]\n",
               i + 1,
               type,
               lba,
               count,
               end_exclusive,
               in_range ? "IN RANGE" : "OUT OF RANGE");

        if (result.first_partition_lba == 0) {
            result.first_partition_lba = lba;
        }
        if (end_exclusive > result.max_partition_end_exclusive) {
            result.max_partition_end_exclusive = end_exclusive;
        }
        if (!in_range) {
            result.geometry_consistent = false;
        }
    }

    if (result.nonempty_partitions == 0) {
        printf("  MBR partitions           : none detected\n");
        printf("  Interpretation           : sector 0 may be a superfloppy/volume boot sector\n");
    }

    printf("\n[CAPACITY CONSISTENCY]\n");
    printf("  CSD addressable sectors  : %" PRIu64 "\n", card_sector_count);
    if (result.nonempty_partitions == 0) {
        printf("  MBR maximum end          : n/a\n");
        printf("  MBR vs CSD geometry      : NOT APPLICABLE\n");
    } else {
        printf("  MBR maximum end          : %" PRIu64 " (exclusive)\n",
               result.max_partition_end_exclusive);
        printf("  MBR vs CSD geometry      : %s\n",
               result.geometry_consistent
                   ? "PASS"
                   : "WARNING - PARTITION EXTENT EXCEEDS CSD CAPACITY");
    }

    return result;
}

void app_main(void)
{
    printf("\n");
    printf("================================================================\n");
    printf(" WT32-SC01-PLUS-Lab / 03_storage_test\n");
    printf(" READ-ONLY SD / SDSPI hardware validation\n");
    printf("================================================================\n");
    printf(" CLK / MOSI / MISO / CS    : 39 / 40 / 38 / 41\n");
    printf(" SDSPI test clock          : %d kHz\n", SD_TEST_FREQ_KHZ);
    printf(" Filesystem mount          : NO\n");
    printf(" Sector writes             : NO\n");
    printf(" Format / create / rename  : NO\n");
    printf(" Read scope                : card metadata + sector 0 + first partition boot sector\n");
    printf(" Capacity check            : MBR partition extents vs card CSD\n");
    printf("================================================================\n\n");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SD_TEST_FREQ_KHZ;

    const spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_SD_MOSI,
        .miso_io_num = PIN_SD_MISO,
        .sclk_io_num = PIN_SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SECTOR_SIZE,
    };

    ESP_LOGI(TAG, "Initializing SPI bus");
    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        printf("\nRESULT: INVESTIGATE - SPI BUS INIT FAILED\n");
        return;
    }

    ESP_LOGI(TAG, "Initializing SDSPI host");
    ret = sdspi_host_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sdspi_host_init failed: %s", esp_err_to_name(ret));
        spi_bus_free(host.slot);
        printf("\nRESULT: INVESTIGATE - SDSPI HOST INIT FAILED\n");
        return;
    }

    sdspi_device_config_t dev_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    dev_cfg.host_id = host.slot;
    dev_cfg.gpio_cs = PIN_SD_CS;
    dev_cfg.gpio_cd = SDSPI_SLOT_NO_CD;
    dev_cfg.gpio_wp = SDSPI_SLOT_NO_WP;

    sdspi_dev_handle_t device = -1;
    ret = sdspi_host_init_device(&dev_cfg, &device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sdspi_host_init_device failed: %s", esp_err_to_name(ret));
        sdspi_host_deinit();
        spi_bus_free(host.slot);
        printf("\nRESULT: INVESTIGATE - SDSPI DEVICE INIT FAILED\n");
        return;
    }

    host.slot = device;

    sdmmc_card_t card;
    memset(&card, 0, sizeof(card));

    ESP_LOGI(TAG, "Probing and initializing SD card");
    ret = sdmmc_card_init(&host, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sdmmc_card_init failed: %s", esp_err_to_name(ret));
        sdspi_host_remove_device(device);
        sdspi_host_deinit();
        spi_bus_free(dev_cfg.host_id);
        printf("\nRESULT: INVESTIGATE - CARD NOT INITIALIZED\n");
        printf("Check card insertion, contact quality, card compatibility and pull-ups.\n");
        return;
    }

    printf("\n[CARD INFORMATION]\n");
    sdmmc_card_print_info(stdout, &card);

    const uint64_t card_sector_count = (uint64_t)card.csd.capacity;

    int real_freq_khz = 0;
    if (sdspi_host_get_real_freq(device, &real_freq_khz) == ESP_OK) {
        printf("  SDSPI actual clock       : %d kHz\n", real_freq_khz);
    }

    ESP_LOGI(TAG, "Reading sector 0 (read-only)");
    ret = sdmmc_read_sectors(&card, s_sector, 0, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sector 0 read failed: %s", esp_err_to_name(ret));
        sdspi_host_remove_device(device);
        sdspi_host_deinit();
        spi_bus_free(dev_cfg.host_id);
        printf("\nRESULT: INVESTIGATE - CARD INIT OK, DATA READ FAILED\n");
        return;
    }

    const mbr_inspection_t mbr = inspect_mbr(s_sector, card_sector_count);

    if (mbr.first_partition_lba != 0) {
        ESP_LOGI(TAG, "Reading first partition boot sector at LBA %" PRIu32 " (read-only)",
                 mbr.first_partition_lba);
        ret = sdmmc_read_sectors(&card, s_sector, mbr.first_partition_lba, 1);
        if (ret == ESP_OK) {
            print_boot_sector_hint(s_sector, mbr.first_partition_lba);
        } else {
            ESP_LOGW(TAG, "partition boot sector read failed: %s", esp_err_to_name(ret));
        }
    } else {
        print_boot_sector_hint(s_sector, 0);
    }

    printf("\n[SAFETY AUDIT]\n");
    printf("  FAT filesystem mounted   : no\n");
    printf("  Files opened             : no\n");
    printf("  Files created/renamed    : no\n");
    printf("  Sectors written          : no\n");
    printf("  Card formatted           : no\n");

    printf("\n[RESULT]\n");
    printf("  SDSPI read path          : PASS\n");
    if (mbr.nonempty_partitions == 0) {
        printf("  Card capacity consistency: NOT APPLICABLE (no MBR partitions)\n");
        printf("RESULT: STORAGE READ PATH PASS CANDIDATE\n");
    } else if (mbr.geometry_consistent) {
        printf("  Card capacity consistency: PASS\n");
        printf("RESULT: STORAGE READ PATH PASS CANDIDATE\n");
    } else {
        printf("  Card capacity consistency: WARNING - MBR EXCEEDS CSD CAPACITY\n");
        printf("RESULT: PASS WITH MEDIA ANOMALY\n");
        printf("NOTE: the WT32-SC01-PLUS SDSPI read path passed; investigate this SD card separately.\n");
    }
    printf("END 03_storage_test\n");

    sdspi_host_remove_device(device);
    sdspi_host_deinit();
    spi_bus_free(dev_cfg.host_id);
}
