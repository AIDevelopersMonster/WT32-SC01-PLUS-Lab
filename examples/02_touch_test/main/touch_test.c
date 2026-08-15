#include <inttypes.h>
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

#define TOUCH_I2C_FREQ_HZ       100000
#define TOUCH_EXPECTED_ADDR     0x38
#define TOUCH_POLL_MS           20
#define TOUCH_TEST_DURATION_MS  30000

#define REG_TOUCH_POINTS        0x02
#define REG_TOUCH1_XH           0x03
#define REG_CHIP_ID_HINT        0xA3
#define REG_FIRMWARE_ID         0xA6
#define REG_VENDOR_ID_HINT      0xA8

static esp_err_t read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, data, len, 100);
}

static void print_scan(i2c_master_bus_handle_t bus, bool *found_expected)
{
    unsigned found = 0;

    printf("\n[I2C SCAN]\n");
    for (uint16_t addr = 0x08; addr <= 0x77; ++addr) {
        esp_err_t err = i2c_master_probe(bus, addr, 20);
        if (err == ESP_OK) {
            printf("  ACK at 0x%02X\n", addr);
            ++found;
            if (addr == TOUCH_EXPECTED_ADDR) {
                *found_expected = true;
            }
        } else if (err == ESP_ERR_TIMEOUT) {
            printf("  Probe timeout at 0x%02X -- check bus/pull-ups\n", addr);
        }
    }

    if (found == 0) {
        printf("  No responding I2C addresses detected\n");
    }
}

void app_main(void)
{
    printf("\n");
    printf("================================================================\n");
    printf(" WT32-SC01-PLUS-Lab / 02_touch_test\n");
    printf(" READ-ONLY I2C touch discovery + raw coordinate validation\n");
    printf("================================================================\n");
    printf(" SDA / SCL / INT / shared RST : 6 / 5 / 7 / 4\n");
    printf(" I2C clock                    : %u Hz\n", TOUCH_I2C_FREQ_HZ);
    printf(" GPIO4 reset action           : NOT DRIVEN\n");
    printf(" Display                      : NOT INITIALIZED\n");
    printf(" Controller writes            : NONE\n");
    printf(" Expected FT5x06-family addr  : 0x38 (hypothesis to test)\n");
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
        .i2c_port = I2C_NUM_0,
        .sda_io_num = TOUCH_SDA_GPIO,
        .scl_io_num = TOUCH_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus = NULL;
    ESP_LOGI(TAG, "Initializing I2C master bus");
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        printf("RESULT: INVESTIGATE - I2C BUS INIT FAILED\n");
        return;
    }

    bool found_expected = false;
    print_scan(bus, &found_expected);

    if (!found_expected) {
        printf("\n[RESULT]\n");
        printf("  I2C bus scan              : completed\n");
        printf("  Address 0x38              : NOT DETECTED\n");
        printf("RESULT: INVESTIGATE - NO FT5x06-FAMILY ADDRESS AT 0x38\n");
        printf("NOTE: exact controller identity remains unresolved; no controller registers were written.\n");
        printf("END 02_touch_test\n");
        i2c_del_master_bus(bus);
        return;
    }

    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TOUCH_EXPECTED_ADDR,
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

    uint8_t chip_hint = 0;
    uint8_t fw_id = 0;
    uint8_t vendor_hint = 0;
    const esp_err_t chip_err = read_reg(dev, REG_CHIP_ID_HINT, &chip_hint, 1);
    const esp_err_t fw_err = read_reg(dev, REG_FIRMWARE_ID, &fw_id, 1);
    const esp_err_t vendor_err = read_reg(dev, REG_VENDOR_ID_HINT, &vendor_hint, 1);

    printf("\n[READ-ONLY REGISTER HINTS @ 0x38]\n");
    printf("  reg 0xA3                 : %s", chip_err == ESP_OK ? "0x" : "read failed");
    if (chip_err == ESP_OK) {
        printf("%02X", chip_hint);
    }
    printf("\n");
    printf("  reg 0xA6                 : %s", fw_err == ESP_OK ? "0x" : "read failed");
    if (fw_err == ESP_OK) {
        printf("%02X", fw_id);
    }
    printf("\n");
    printf("  reg 0xA8                 : %s", vendor_err == ESP_OK ? "0x" : "read failed");
    if (vendor_err == ESP_OK) {
        printf("%02X", vendor_hint);
    }
    printf("\n");
    printf("  Interpretation           : hints only; exact controller model is NOT claimed\n");

    printf("\n[RAW TOUCH OBSERVATION]\n");
    printf("Touch corners, center, and drag across the panel for about 30 seconds.\n");
    printf("Raw FT5x06-compatible fields are read only; no coordinate transform is applied.\n\n");

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
        err = read_reg(dev, REG_TOUCH_POINTS, &points_raw, 1);
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

        if (points > 0 && points <= 5) {
            uint8_t raw[6] = {0};
            err = read_reg(dev, REG_TOUCH1_XH, raw, sizeof(raw));
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
    printf("  I2C address 0x38         : ACK\n");
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
        printf("  I2C touch read path      : PASS CANDIDATE WITH READ ERRORS\n");
        printf("RESULT: INVESTIGATE - TOUCH DATA SEEN WITH I2C ERRORS\n");
    } else {
        printf("  I2C touch read path      : NO TOUCH SAMPLES OBSERVED\n");
        printf("RESULT: INVESTIGATE - ADDRESS RESPONDS BUT NO TOUCH DATA OBSERVED\n");
    }
    printf("Exact touch-controller model and 480x320 orientation remain outside this test's claim.\n");
    printf("END 02_touch_test\n");

    i2c_master_bus_rm_device(dev);
    i2c_del_master_bus(bus);
}
