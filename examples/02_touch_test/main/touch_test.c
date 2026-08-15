#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "touch_test";

#define TOUCH_SDA_GPIO          GPIO_NUM_6
#define TOUCH_SCL_GPIO          GPIO_NUM_5
#define TOUCH_INT_GPIO          GPIO_NUM_7
#define TOUCH_RST_GPIO          GPIO_NUM_4
#define TOUCH_I2C_PORT          I2C_NUM_1
#define TOUCH_I2C_FREQ_HZ       400000
#define TOUCH_I2C_ADDR          0x38

#define TOUCH_POLL_MS           20
#define TOUCH_TEST_DURATION_MS  30000
#define TOUCH_RESET_LOW_MS      20
#define TOUCH_RESET_BOOT_MS     200

/* FT6336U-compatible register map used by the external reference project. */
#define REG_MODE_SWITCH         0x00
#define REG_TD_STATUS           0x02
#define REG_P1_XH               0x03
#define REG_ID_G_CIPHER_LOW     0xA0
#define REG_ID_G_CIPHER_HIGH    0xA3
#define REG_ID_G_FIRMID         0xA6
#define REG_ID_G_FOCALTECH_ID   0xA8
#define FT6336U_CHIP_CODE        0x02

static esp_err_t read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, data, len, 100);
}

static unsigned scan_bus(i2c_master_bus_handle_t bus, const char *label, bool *saw_0x38)
{
    unsigned found = 0;

    printf("\n[I2C SCAN - %s]\n", label);
    for (uint16_t addr = 0x08; addr <= 0x77; ++addr) {
        const esp_err_t err = i2c_master_probe(bus, addr, 20);
        if (err == ESP_OK) {
            printf("  ACK at 0x%02X\n", addr);
            ++found;
            if (addr == TOUCH_I2C_ADDR) {
                *saw_0x38 = true;
            }
        } else if (err == ESP_ERR_TIMEOUT) {
            printf("  Probe timeout at 0x%02X -- check bus/pull-ups\n", addr);
        }
    }

    if (found == 0) {
        printf("  No responding addresses detected by address-only probe\n");
    }
    return found;
}

static esp_err_t pulse_shared_reset(void)
{
    printf("\n[SHARED RESET RECOVERY]\n");
    printf("  GPIO4 is shared by touch reset and LCD reset.\n");
    printf("  Display is not initialized in this test.\n");
    printf("  Sequence: HIGH -> LOW %u ms -> HIGH -> wait %u ms\n",
           TOUCH_RESET_LOW_MS, TOUCH_RESET_BOOT_MS);

    const gpio_config_t rst_cfg = {
        .pin_bit_mask = 1ULL << TOUCH_RST_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&rst_cfg);
    if (err != ESP_OK) return err;

    err = gpio_set_level(TOUCH_RST_GPIO, 1);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(10));

    err = gpio_set_level(TOUCH_RST_GPIO, 0);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(TOUCH_RESET_LOW_MS));

    err = gpio_set_level(TOUCH_RST_GPIO, 1);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(TOUCH_RESET_BOOT_MS));

    return ESP_OK;
}

static bool read_identity(i2c_master_dev_handle_t dev,
                          uint8_t *cipher_low,
                          uint8_t *cipher_high,
                          uint8_t *firm_id,
                          uint8_t *focaltech_id)
{
    const esp_err_t e0 = read_reg(dev, REG_ID_G_CIPHER_LOW, cipher_low, 1);
    const esp_err_t e1 = read_reg(dev, REG_ID_G_CIPHER_HIGH, cipher_high, 1);
    const esp_err_t e2 = read_reg(dev, REG_ID_G_FIRMID, firm_id, 1);
    const esp_err_t e3 = read_reg(dev, REG_ID_G_FOCALTECH_ID, focaltech_id, 1);

    printf("\n[DIRECT REGISTER READ @ 0x38]\n");
    printf("  0xA0 CIPHER_LOW / chip code : %s", e0 == ESP_OK ? "0x" : "read failed");
    if (e0 == ESP_OK) printf("%02X", *cipher_low);
    printf("\n");
    printf("  0xA3 CIPHER_HIGH             : %s", e1 == ESP_OK ? "0x" : "read failed");
    if (e1 == ESP_OK) printf("%02X", *cipher_high);
    printf("\n");
    printf("  0xA6 firmware ID             : %s", e2 == ESP_OK ? "0x" : "read failed");
    if (e2 == ESP_OK) printf("%02X", *firm_id);
    printf("\n");
    printf("  0xA8 FocalTech ID            : %s", e3 == ESP_OK ? "0x" : "read failed");
    if (e3 == ESP_OK) printf("%02X", *focaltech_id);
    printf("\n");

    return e0 == ESP_OK;
}

void app_main(void)
{
    printf("\n");
    printf("================================================================\n");
    printf(" WT32-SC01-PLUS-Lab / 02_touch_test\n");
    printf(" FT6336U-oriented I2C discovery + raw coordinate validation\n");
    printf("================================================================\n");
    printf(" SDA / SCL / INT / shared RST : 6 / 5 / 7 / 4\n");
    printf(" I2C controller               : I2C1\n");
    printf(" I2C clock                    : %u Hz\n", TOUCH_I2C_FREQ_HZ);
    printf(" Expected address             : 0x38\n");
    printf(" Reference chip code          : reg 0xA0 == 0x02 for FT6336U\n");
    printf(" Initial reset action         : NONE\n");
    printf(" Recovery reset               : GPIO4 only if direct read fails\n");
    printf(" Display                      : NOT INITIALIZED\n");
    printf(" Controller register writes   : NONE\n");
    printf(" Raw observation window       : %u ms\n", TOUCH_TEST_DURATION_MS);
    printf("================================================================\n\n");

    const gpio_config_t int_cfg = {
        .pin_bit_mask = 1ULL << TOUCH_INT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&int_cfg));

    const i2c_master_bus_config_t bus_cfg = {
        .i2c_port = TOUCH_I2C_PORT,
        .sda_io_num = TOUCH_SDA_GPIO,
        .scl_io_num = TOUCH_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus = NULL;
    ESP_LOGI(TAG, "Initializing I2C1 on SDA6/SCL5 at 400 kHz");
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        printf("RESULT: INVESTIGATE - I2C BUS INIT FAILED\n");
        return;
    }

    bool scan_saw_0x38 = false;
    const unsigned scan_count_before = scan_bus(bus, "BEFORE RESET", &scan_saw_0x38);

    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TOUCH_I2C_ADDR,
        .scl_speed_hz = TOUCH_I2C_FREQ_HZ,
        .scl_wait_us = 0,
        .flags.disable_ack_check = false,
    };

    i2c_master_dev_handle_t dev = NULL;
    err = i2c_master_bus_add_device(bus, &dev_cfg, &dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
        printf("RESULT: INVESTIGATE - DEVICE HANDLE CREATION FAILED\n");
        i2c_del_master_bus(bus);
        return;
    }

    uint8_t cipher_low = 0;
    uint8_t cipher_high = 0;
    uint8_t firm_id = 0;
    uint8_t focaltech_id = 0;

    bool direct_read_ok = read_identity(dev, &cipher_low, &cipher_high, &firm_id, &focaltech_id);
    bool reset_attempted = false;
    unsigned scan_count_after = 0;

    if (!direct_read_ok) {
        reset_attempted = true;
        err = pulse_shared_reset();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Shared reset pulse failed: %s", esp_err_to_name(err));
            printf("RESULT: INVESTIGATE - SHARED RESET GPIO FAILED\n");
            printf("END 02_touch_test\n");
            i2c_master_bus_rm_device(dev);
            i2c_del_master_bus(bus);
            return;
        }

        bool scan_after_saw_0x38 = false;
        scan_count_after = scan_bus(bus, "AFTER GPIO4 RESET", &scan_after_saw_0x38);
        scan_saw_0x38 = scan_saw_0x38 || scan_after_saw_0x38;
        direct_read_ok = read_identity(dev, &cipher_low, &cipher_high, &firm_id, &focaltech_id);
    }

    printf("\n[DISCOVERY SUMMARY]\n");
    printf("  Address-only scan count before reset : %u\n", scan_count_before);
    printf("  Address-only scan saw 0x38            : %s\n", scan_saw_0x38 ? "yes" : "no");
    printf("  Shared reset attempted                : %s\n", reset_attempted ? "yes" : "no");
    if (reset_attempted) {
        printf("  Address-only scan count after reset  : %u\n", scan_count_after);
    }
    printf("  Direct register read at 0x38          : %s\n", direct_read_ok ? "PASS" : "FAILED");
    printf("  INT level                             : %d\n", gpio_get_level(TOUCH_INT_GPIO));

    if (!direct_read_ok) {
        printf("\n[RESULT]\n");
        printf("RESULT: INVESTIGATE - NO DIRECT FT6336U-COMPATIBLE RESPONSE AT 0x38\n");
        printf("NOTE: address-only scan is diagnostic only; the decisive test here is direct register access.\n");
        printf("NOTE: next step is physical SDA/SCL/RST/INT/power verification against the factory image.\n");
        printf("END 02_touch_test\n");
        i2c_master_bus_rm_device(dev);
        i2c_del_master_bus(bus);
        return;
    }

    printf("\n[IDENTITY INTERPRETATION]\n");
    if (cipher_low == FT6336U_CHIP_CODE) {
        printf("  reg 0xA0 == 0x02          : MATCHES FT6336U reference driver\n");
    } else {
        printf("  reg 0xA0                  : 0x%02X (does NOT match reference FT6336U code 0x02)\n", cipher_low);
    }
    printf("  Claim                     : FT6336U-compatible signature only until independently confirmed\n");

    printf("\n[RAW TOUCH OBSERVATION]\n");
    printf("Touch corners, center, and drag across the panel for about 30 seconds.\n");
    printf("Registers are read only; no coordinate transform is applied.\n\n");

    const TickType_t start = xTaskGetTickCount();
    const TickType_t duration = pdMS_TO_TICKS(TOUCH_TEST_DURATION_MS);
    unsigned samples_with_touch = 0;
    unsigned read_errors = 0;
    uint16_t min_x = UINT16_MAX;
    uint16_t max_x = 0;
    uint16_t min_y = UINT16_MAX;
    uint16_t max_y = 0;
    uint8_t last_points = 0xFF;

    while ((xTaskGetTickCount() - start) < duration) {
        uint8_t points_raw = 0;
        err = read_reg(dev, REG_TD_STATUS, &points_raw, 1);
        if (err != ESP_OK) {
            ++read_errors;
            vTaskDelay(pdMS_TO_TICKS(TOUCH_POLL_MS));
            continue;
        }

        const uint8_t points = points_raw & 0x0F;
        if (points != last_points) {
            printf("points=%u INT=%d\n", points, gpio_get_level(TOUCH_INT_GPIO));
            last_points = points;
        }

        if (points > 0 && points <= 2) {
            uint8_t raw[6] = {0};
            err = read_reg(dev, REG_P1_XH, raw, sizeof(raw));
            if (err == ESP_OK) {
                const uint8_t event = (raw[0] >> 6) & 0x03;
                const uint16_t x = ((uint16_t)(raw[0] & 0x0F) << 8) | raw[1];
                const uint8_t track_id = (raw[2] >> 4) & 0x0F;
                const uint16_t y = ((uint16_t)(raw[2] & 0x0F) << 8) | raw[3];

                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
                ++samples_with_touch;

                printf("touch raw: x=%u y=%u event=%u track=%u INT=%d\n",
                       x, y, event, track_id, gpio_get_level(TOUCH_INT_GPIO));
            } else {
                ++read_errors;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(TOUCH_POLL_MS));
    }

    printf("\n[SUMMARY]\n");
    printf("  Direct register path     : PASS\n");
    printf("  FT6336U reference code   : %s\n", cipher_low == FT6336U_CHIP_CODE ? "MATCH" : "NO MATCH");
    printf("  Samples with touch      : %u\n", samples_with_touch);
    printf("  I2C read errors          : %u\n", read_errors);
    if (samples_with_touch > 0) {
        printf("  Observed raw X range     : %u .. %u\n", min_x, max_x);
        printf("  Observed raw Y range     : %u .. %u\n", min_y, max_y);
    } else {
        printf("  Observed raw ranges      : none\n");
    }

    printf("\n[RESULT]\n");
    if (samples_with_touch > 0 && read_errors == 0) {
        printf("  I2C touch read path      : PASS CANDIDATE\n");
        printf("RESULT: TOUCH RAW READ PATH PASS CANDIDATE\n");
    } else if (samples_with_touch > 0) {
        printf("RESULT: INVESTIGATE - TOUCH DATA SEEN WITH I2C READ ERRORS\n");
    } else {
        printf("RESULT: INVESTIGATE - CONTROLLER RESPONDS BUT NO TOUCH SAMPLES OBSERVED\n");
    }

    printf("Exact physical controller marking and 480x320 coordinate orientation remain outside this test's claim.\n");
    printf("END 02_touch_test\n");

    i2c_master_bus_rm_device(dev);
    i2c_del_master_bus(bus);
}
