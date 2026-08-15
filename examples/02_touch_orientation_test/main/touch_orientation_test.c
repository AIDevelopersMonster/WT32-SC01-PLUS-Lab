#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7796.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "touch_orient";

#define LCD_H_RES               480
#define LCD_V_RES               320
#define LCD_BUS_WIDTH           8
#define LCD_PCLK_HZ             (10 * 1000 * 1000)
#define LCD_DRAW_LINES          20
#define LCD_DRAW_BUF_PIXELS     (LCD_H_RES * LCD_DRAW_LINES)
#define LCD_DRAW_BUF_BYTES      (LCD_DRAW_BUF_PIXELS * sizeof(uint16_t))

#define PIN_LCD_BL              45
#define PIN_LCD_RST             4
#define PIN_LCD_DC              0
#define PIN_LCD_WR              47
#define PIN_LCD_CS              (-1)
#define PIN_LCD_D0              9
#define PIN_LCD_D1              46
#define PIN_LCD_D2              3
#define PIN_LCD_D3              8
#define PIN_LCD_D4              18
#define PIN_LCD_D5              17
#define PIN_LCD_D6              16
#define PIN_LCD_D7              15

#define TOUCH_SDA_GPIO          GPIO_NUM_6
#define TOUCH_SCL_GPIO          GPIO_NUM_5
#define TOUCH_INT_GPIO          GPIO_NUM_7
#define TOUCH_I2C_PORT          I2C_NUM_1
#define TOUCH_I2C_FREQ_HZ       400000
#define TOUCH_I2C_ADDR          0x38
#define REG_TD_STATUS           0x02
#define REG_P1_XH               0x03
#define REG_ID_G_CIPHER_LOW     0xA0
#define FT6336U_CHIP_CODE       0x02

#define RAW_NATIVE_X_MAX        319.0
#define RAW_NATIVE_Y_MAX        479.0
#define TARGET_RADIUS           14
#define TARGET_CROSS_HALF       26
#define TARGET_TIMEOUT_MS       15000
#define RELEASE_STABLE_MS       120
#define SAMPLE_COUNT            5
#define SAMPLE_INTERVAL_MS      20

#define RGB565(r, g, b) \
    ((uint16_t)((((uint16_t)(r) & 0xF8U) << 8) | \
                (((uint16_t)(g) & 0xFCU) << 3) | \
                (((uint16_t)(b) & 0xF8U) >> 3)))

typedef struct {
    const char *name;
    int lcd_x;
    int lcd_y;
    uint16_t raw_x;
    uint16_t raw_y;
    bool captured;
} target_t;

typedef struct {
    bool swap_xy;
    bool mirror_x;
    bool mirror_y;
    double rms_error;
} transform_candidate_t;

static SemaphoreHandle_t s_tx_done_sem;
static esp_lcd_panel_handle_t s_panel;
static uint16_t *s_draw_buf;

static bool lcd_color_trans_done(esp_lcd_panel_io_handle_t panel_io,
                                 esp_lcd_panel_io_event_data_t *edata,
                                 void *user_ctx)
{
    (void)panel_io;
    (void)edata;
    (void)user_ctx;
    BaseType_t high_task_wakeup = pdFALSE;
    xSemaphoreGiveFromISR(s_tx_done_sem, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

static void wait_for_color_transfer(void)
{
    if (xSemaphoreTake(s_tx_done_sem, pdMS_TO_TICKS(2000)) != pdTRUE) {
        ESP_LOGE(TAG, "Timed out waiting for LCD DMA transfer");
        abort();
    }
}

static uint16_t target_pixel(int x, int y, int tx, int ty)
{
    const int dx = x - tx;
    const int dy = y - ty;
    const uint16_t black = RGB565(0, 0, 0);
    const uint16_t white = RGB565(255, 255, 255);
    const uint16_t red = RGB565(255, 0, 0);
    const uint16_t gray = RGB565(70, 70, 70);

    if (x < 3 || x >= LCD_H_RES - 3 || y < 3 || y >= LCD_V_RES - 3) {
        return gray;
    }
    if ((dx * dx + dy * dy) <= (TARGET_RADIUS * TARGET_RADIUS)) {
        return red;
    }
    if ((abs(dx) <= 1 && abs(dy) <= TARGET_CROSS_HALF) ||
        (abs(dy) <= 1 && abs(dx) <= TARGET_CROSS_HALF)) {
        return white;
    }
    return black;
}

static void show_target(int tx, int ty)
{
    for (int y0 = 0; y0 < LCD_V_RES; y0 += LCD_DRAW_LINES) {
        const int lines = (y0 + LCD_DRAW_LINES <= LCD_V_RES)
                              ? LCD_DRAW_LINES
                              : (LCD_V_RES - y0);
        for (int row = 0; row < lines; ++row) {
            const int y = y0 + row;
            for (int x = 0; x < LCD_H_RES; ++x) {
                s_draw_buf[row * LCD_H_RES + x] = target_pixel(x, y, tx, ty);
            }
        }
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(s_panel,
                                                  0,
                                                  y0,
                                                  LCD_H_RES,
                                                  y0 + lines,
                                                  s_draw_buf));
        wait_for_color_transfer();
    }
}

static void init_display(void)
{
    const gpio_config_t bl_cfg = {
        .pin_bit_mask = 1ULL << PIN_LCD_BL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&bl_cfg));
    ESP_ERROR_CHECK(gpio_set_level(PIN_LCD_BL, 0));

    s_tx_done_sem = xSemaphoreCreateBinary();
    if (s_tx_done_sem == NULL) {
        abort();
    }

    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    const esp_lcd_i80_bus_config_t bus_cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .dc_gpio_num = PIN_LCD_DC,
        .wr_gpio_num = PIN_LCD_WR,
        .data_gpio_nums = {
            PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3,
            PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7,
        },
        .bus_width = LCD_BUS_WIDTH,
        .max_transfer_bytes = LCD_DRAW_BUF_BYTES,
        .dma_burst_size = 64,
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_cfg, &i80_bus));

    esp_lcd_panel_io_handle_t io = NULL;
    const esp_lcd_panel_io_i80_config_t io_cfg = {
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = LCD_PCLK_HZ,
        .trans_queue_depth = 4,
        .on_color_trans_done = lcd_color_trans_done,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags = {
            .swap_color_bytes = 1,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_cfg, &io));

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io, &panel_cfg, &s_panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    s_draw_buf = (uint16_t *)esp_lcd_i80_alloc_draw_buffer(
        io, LCD_DRAW_BUF_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (s_draw_buf == NULL) {
        abort();
    }

    ESP_ERROR_CHECK(gpio_set_level(PIN_LCD_BL, 1));

    /* The LCD reset pin is shared with touch reset. Allow touch to boot. */
    vTaskDelay(pdMS_TO_TICKS(250));
}

static esp_err_t read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, data, len, 100);
}

static bool read_touch_point(i2c_master_dev_handle_t dev, uint16_t *x, uint16_t *y)
{
    uint8_t points_raw = 0;
    if (read_reg(dev, REG_TD_STATUS, &points_raw, 1) != ESP_OK) {
        return false;
    }
    const uint8_t points = points_raw & 0x0F;
    if (points == 0 || points > 2) {
        return false;
    }

    uint8_t raw[4] = {0};
    if (read_reg(dev, REG_P1_XH, raw, sizeof(raw)) != ESP_OK) {
        return false;
    }

    *x = ((uint16_t)(raw[0] & 0x0F) << 8) | raw[1];
    *y = ((uint16_t)(raw[2] & 0x0F) << 8) | raw[3];
    return true;
}

static bool wait_for_release(i2c_master_dev_handle_t dev)
{
    TickType_t stable_start = 0;
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(TARGET_TIMEOUT_MS);

    while ((int32_t)(deadline - xTaskGetTickCount()) > 0) {
        uint8_t points_raw = 0;
        if (read_reg(dev, REG_TD_STATUS, &points_raw, 1) == ESP_OK &&
            (points_raw & 0x0F) == 0) {
            if (stable_start == 0) {
                stable_start = xTaskGetTickCount();
            }
            if ((xTaskGetTickCount() - stable_start) >= pdMS_TO_TICKS(RELEASE_STABLE_MS)) {
                return true;
            }
        } else {
            stable_start = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return false;
}

static int cmp_u16(const void *a, const void *b)
{
    const uint16_t av = *(const uint16_t *)a;
    const uint16_t bv = *(const uint16_t *)b;
    return (av > bv) - (av < bv);
}

static bool capture_target(i2c_master_dev_handle_t dev, target_t *target)
{
    printf("\n[TARGET] %s lcd=(%d,%d)\n", target->name, target->lcd_x, target->lcd_y);
    printf("Touch the red target and hold briefly. Release before the next target.\n");

    show_target(target->lcd_x, target->lcd_y);

    if (!wait_for_release(dev)) {
        printf("  ERROR: release was not observed before target capture\n");
        return false;
    }

    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(TARGET_TIMEOUT_MS);
    uint16_t xs[SAMPLE_COUNT] = {0};
    uint16_t ys[SAMPLE_COUNT] = {0};
    unsigned count = 0;

    while ((int32_t)(deadline - xTaskGetTickCount()) > 0) {
        uint16_t x = 0;
        uint16_t y = 0;
        if (read_touch_point(dev, &x, &y)) {
            xs[count] = x;
            ys[count] = y;
            ++count;
            if (count >= SAMPLE_COUNT) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (count < SAMPLE_COUNT) {
        printf("  ERROR: captured only %u/%u samples\n", count, SAMPLE_COUNT);
        return false;
    }

    qsort(xs, SAMPLE_COUNT, sizeof(xs[0]), cmp_u16);
    qsort(ys, SAMPLE_COUNT, sizeof(ys[0]), cmp_u16);
    target->raw_x = xs[SAMPLE_COUNT / 2];
    target->raw_y = ys[SAMPLE_COUNT / 2];
    target->captured = true;

    printf("  CAPTURED raw=(%u,%u), INT=%d\n",
           target->raw_x, target->raw_y, gpio_get_level(TOUCH_INT_GPIO));
    return true;
}

static void map_candidate(const transform_candidate_t *c,
                          uint16_t raw_x,
                          uint16_t raw_y,
                          double *lcd_x,
                          double *lcd_y)
{
    double nx;
    double ny;

    if (c->swap_xy) {
        nx = (double)raw_y / RAW_NATIVE_Y_MAX;
        ny = (double)raw_x / RAW_NATIVE_X_MAX;
    } else {
        nx = (double)raw_x / RAW_NATIVE_X_MAX;
        ny = (double)raw_y / RAW_NATIVE_Y_MAX;
    }

    if (c->mirror_x) nx = 1.0 - nx;
    if (c->mirror_y) ny = 1.0 - ny;

    *lcd_x = nx * (LCD_H_RES - 1);
    *lcd_y = ny * (LCD_V_RES - 1);
}

static const char *bool_word(bool v)
{
    return v ? "true" : "false";
}

static void evaluate_transforms(const target_t *targets, size_t count)
{
    transform_candidate_t candidates[8];
    size_t ci = 0;

    for (int swap = 0; swap <= 1; ++swap) {
        for (int mx = 0; mx <= 1; ++mx) {
            for (int my = 0; my <= 1; ++my) {
                candidates[ci++] = (transform_candidate_t) {
                    .swap_xy = swap != 0,
                    .mirror_x = mx != 0,
                    .mirror_y = my != 0,
                    .rms_error = 0.0,
                };
            }
        }
    }

    for (size_t c = 0; c < 8; ++c) {
        double sum_sq = 0.0;
        for (size_t i = 0; i < count; ++i) {
            double x = 0.0;
            double y = 0.0;
            map_candidate(&candidates[c], targets[i].raw_x, targets[i].raw_y, &x, &y);
            const double dx = x - targets[i].lcd_x;
            const double dy = y - targets[i].lcd_y;
            sum_sq += dx * dx + dy * dy;
        }
        candidates[c].rms_error = sqrt(sum_sq / (double)count);
    }

    for (size_t i = 0; i < 7; ++i) {
        for (size_t j = i + 1; j < 8; ++j) {
            if (candidates[j].rms_error < candidates[i].rms_error) {
                const transform_candidate_t t = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = t;
            }
        }
    }

    printf("\n[TRANSFORM CANDIDATES]\n");
    for (size_t i = 0; i < 8; ++i) {
        printf("  #%u swap_xy=%s mirror_x=%s mirror_y=%s RMS=%.2f px\n",
               (unsigned)(i + 1),
               bool_word(candidates[i].swap_xy),
               bool_word(candidates[i].mirror_x),
               bool_word(candidates[i].mirror_y),
               candidates[i].rms_error);
    }

    const transform_candidate_t *best = &candidates[0];
    printf("\n[BEST TRANSFORM]\n");
    printf("  swap_xy  : %s\n", bool_word(best->swap_xy));
    printf("  mirror_x : %s\n", bool_word(best->mirror_x));
    printf("  mirror_y : %s\n", bool_word(best->mirror_y));
    printf("  RMS error: %.2f px\n", best->rms_error);

    printf("\n[POINT CHECK]\n");
    for (size_t i = 0; i < count; ++i) {
        double x = 0.0;
        double y = 0.0;
        map_candidate(best, targets[i].raw_x, targets[i].raw_y, &x, &y);
        printf("  %-12s raw=(%3u,%3u) -> mapped=(%6.1f,%6.1f), target=(%3d,%3d)\n",
               targets[i].name,
               targets[i].raw_x,
               targets[i].raw_y,
               x,
               y,
               targets[i].lcd_x,
               targets[i].lcd_y);
    }

    printf("\n[RESULT]\n");
    if (best->rms_error <= 35.0 &&
        (candidates[1].rms_error - best->rms_error) >= 25.0) {
        printf("RESULT: ORIENTATION TRANSFORM PASS CANDIDATE\n");
    } else {
        printf("RESULT: ORIENTATION TRANSFORM NEEDS REVIEW\n");
    }
    printf("Claim remains candidate until operator confirms each displayed target was touched correctly.\n");
}

void app_main(void)
{
    printf("\n");
    printf("================================================================\n");
    printf(" WT32-SC01-PLUS-Lab / 02_touch_orientation_test\n");
    printf(" Display-assisted five-point raw -> 480x320 orientation test\n");
    printf("================================================================\n");
    printf(" LCD logical resolution        : 480 x 320 landscape\n");
    printf(" LCD controller                : ST7796-compatible / I80 10 MHz\n");
    printf(" Touch controller path         : FT6336U-compatible @ 0x38\n");
    printf(" Touch I2C                     : I2C1, SDA6/SCL5, 400 kHz\n");
    printf(" Shared LCD/TP reset           : GPIO4\n");
    printf(" Calibration persistence       : NONE\n");
    printf(" Touch-controller writes       : NONE\n");
    printf("================================================================\n\n");

    const gpio_config_t int_cfg = {
        .pin_bit_mask = 1ULL << TOUCH_INT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&int_cfg));

    ESP_LOGI(TAG, "Initializing display; LCD reset also releases touch controller");
    init_display();

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
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TOUCH_I2C_ADDR,
        .scl_speed_hz = TOUCH_I2C_FREQ_HZ,
        .scl_wait_us = 0,
        .flags.disable_ack_check = false,
    };

    i2c_master_dev_handle_t dev = NULL;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &dev));

    uint8_t chip_code = 0;
    const esp_err_t id_err = read_reg(dev, REG_ID_G_CIPHER_LOW, &chip_code, 1);
    printf("[TOUCH STARTUP]\n");
    printf("  direct read 0xA0 : %s", id_err == ESP_OK ? "0x" : "FAILED");
    if (id_err == ESP_OK) printf("%02X", chip_code);
    printf("\n");

    if (id_err != ESP_OK || chip_code != FT6336U_CHIP_CODE) {
        printf("RESULT: ABORT - TOUCH SIGNATURE NOT CONFIRMED AFTER DISPLAY RESET\n");
        printf("END 02_touch_orientation_test\n");
        return;
    }

    target_t targets[] = {
        { .name = "TOP-LEFT",     .lcd_x = 40,  .lcd_y = 40 },
        { .name = "TOP-RIGHT",    .lcd_x = 439, .lcd_y = 40 },
        { .name = "CENTER",       .lcd_x = 240, .lcd_y = 160 },
        { .name = "BOTTOM-LEFT",  .lcd_x = 40,  .lcd_y = 279 },
        { .name = "BOTTOM-RIGHT", .lcd_x = 439, .lcd_y = 279 },
    };

    printf("\nTouch each red target in sequence. Hold briefly until CAPTURED appears.\n");
    printf("The program requires a release between targets.\n");

    bool all_ok = true;
    for (size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
        if (!capture_target(dev, &targets[i])) {
            all_ok = false;
            break;
        }
    }

    if (all_ok) {
        printf("\n[FIVE-POINT CAPTURE]\n");
        for (size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
            printf("  %-12s lcd=(%3d,%3d) raw=(%3u,%3u)\n",
                   targets[i].name,
                   targets[i].lcd_x,
                   targets[i].lcd_y,
                   targets[i].raw_x,
                   targets[i].raw_y);
        }
        evaluate_transforms(targets, sizeof(targets) / sizeof(targets[0]));
    } else {
        printf("\n[RESULT]\n");
        printf("RESULT: INCOMPLETE - FIVE-POINT CAPTURE FAILED OR TIMED OUT\n");
    }

    printf("END 02_touch_orientation_test\n");

    i2c_master_bus_rm_device(dev);
    i2c_del_master_bus(bus);
}
