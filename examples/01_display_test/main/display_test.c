#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lcd_st7796.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "display_test";

/*
 * Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208
 *
 * Display path evidence:
 *   - factory firmware: ST7796 + ESP32-S3 8080 path
 *   - physical factory LCD test: passed
 *   - independent WT32-SC01-PLUS family pinout: matches the mapping below
 *
 * This mapping is being promoted to direct physical evidence only after this
 * test is visually confirmed on the reference specimen.
 */
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
#define PIN_LCD_TE              48 /* documented, intentionally unused here */

#define PIN_LCD_D0              9
#define PIN_LCD_D1              46
#define PIN_LCD_D2              3
#define PIN_LCD_D3              8
#define PIN_LCD_D4              18
#define PIN_LCD_D5              17
#define PIN_LCD_D6              16
#define PIN_LCD_D7              15

#define RGB565(r, g, b) \
    ((uint16_t)((((uint16_t)(r) & 0xF8U) << 8) | \
                (((uint16_t)(g) & 0xFCU) << 3) | \
                (((uint16_t)(b) & 0xF8U) >> 3)))

static SemaphoreHandle_t s_tx_done_sem;

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

static uint16_t color_bars_pixel(int x)
{
    static const uint16_t bars[] = {
        RGB565(255, 255, 255),
        RGB565(255, 255, 0),
        RGB565(0, 255, 255),
        RGB565(0, 255, 0),
        RGB565(255, 0, 255),
        RGB565(255, 0, 0),
        RGB565(0, 0, 255),
        RGB565(0, 0, 0),
    };
    int index = (x * 8) / LCD_H_RES;
    if (index > 7) {
        index = 7;
    }
    return bars[index];
}

static uint16_t geometry_pixel(int x, int y)
{
    const uint16_t black = RGB565(0, 0, 0);
    const uint16_t cyan = RGB565(0, 255, 255);
    const uint16_t yellow = RGB565(255, 255, 0);

    if (x < 4 || x >= LCD_H_RES - 4 || y < 4 || y >= LCD_V_RES - 4) {
        return cyan;
    }

    /* Unique corners make rotation/mirroring immediately visible. */
    if (x < 64 && y < 64) {
        return RGB565(255, 0, 0);       /* expected top-left: red */
    }
    if (x >= LCD_H_RES - 64 && y < 64) {
        return RGB565(0, 255, 0);       /* expected top-right: green */
    }
    if (x < 64 && y >= LCD_V_RES - 64) {
        return RGB565(0, 0, 255);       /* expected bottom-left: blue */
    }
    if (x >= LCD_H_RES - 64 && y >= LCD_V_RES - 64) {
        return RGB565(255, 255, 255);   /* expected bottom-right: white */
    }

    if ((x >= LCD_H_RES / 2 - 3 && x <= LCD_H_RES / 2 + 3) ||
        (y >= LCD_V_RES / 2 - 3 && y <= LCD_V_RES / 2 + 3)) {
        return yellow;
    }

    /* Fine grid exposes missing/stuck data lines and gross timing artifacts. */
    if ((x % 40) == 0 || (y % 40) == 0) {
        return RGB565(80, 80, 80);
    }

    return black;
}

static void render_strip(uint16_t *buf, int y0, int lines, int pattern)
{
    for (int row = 0; row < lines; ++row) {
        const int y = y0 + row;
        for (int x = 0; x < LCD_H_RES; ++x) {
            uint16_t c = 0;
            switch (pattern) {
            case 0: c = RGB565(0, 0, 0); break;
            case 1: c = RGB565(255, 255, 255); break;
            case 2: c = RGB565(255, 0, 0); break;
            case 3: c = RGB565(0, 255, 0); break;
            case 4: c = RGB565(0, 0, 255); break;
            case 5:
                c = color_bars_pixel(x);
                break;
            case 6: {
                const uint8_t v = (uint8_t)((x * 255) / (LCD_H_RES - 1));
                c = RGB565(v, v, v);
                break;
            }
            case 7:
            default:
                c = geometry_pixel(x, y);
                break;
            }
            buf[row * LCD_H_RES + x] = c;
        }
    }
}

static void show_pattern(esp_lcd_panel_handle_t panel,
                         uint16_t *draw_buf,
                         int pattern,
                         const char *name)
{
    ESP_LOGI(TAG, "PATTERN %d: %s", pattern, name);

    for (int y = 0; y < LCD_V_RES; y += LCD_DRAW_LINES) {
        const int lines = (y + LCD_DRAW_LINES <= LCD_V_RES)
                              ? LCD_DRAW_LINES
                              : (LCD_V_RES - y);
        render_strip(draw_buf, y, lines, pattern);
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel,
                                                  0,
                                                  y,
                                                  LCD_H_RES,
                                                  y + lines,
                                                  draw_buf));
        wait_for_color_transfer();
    }
}

static void configure_backlight(void)
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
}

void app_main(void)
{
    printf("\n");
    printf("================================================================\n");
    printf(" WT32-SC01-PLUS-Lab / 01_display_test\n");
    printf(" ST7796 + ESP32-S3 I80 visual hardware validation\n");
    printf("================================================================\n");
    printf(" Logical resolution        : 480 x 320 landscape\n");
    printf(" I80 clock                 : 10 MHz\n");
    printf(" BL/RST/DC/WR              : 45 / 4 / 0 / 47\n");
    printf(" D0..D7                    : 9,46,3,8,18,17,16,15\n");
    printf(" CS                         : tied/unused (-1)\n");
    printf(" TE GPIO48                  : intentionally unused\n");
    printf(" Touch                      : intentionally not initialized\n");
    printf("================================================================\n\n");

    configure_backlight();

    s_tx_done_sem = xSemaphoreCreateBinary();
    if (s_tx_done_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create LCD transfer semaphore");
        abort();
    }

    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    const esp_lcd_i80_bus_config_t bus_cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .dc_gpio_num = PIN_LCD_DC,
        .wr_gpio_num = PIN_LCD_WR,
        .data_gpio_nums = {
            PIN_LCD_D0,
            PIN_LCD_D1,
            PIN_LCD_D2,
            PIN_LCD_D3,
            PIN_LCD_D4,
            PIN_LCD_D5,
            PIN_LCD_D6,
            PIN_LCD_D7,
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

    esp_lcd_panel_handle_t panel = NULL;
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io, &panel_cfg, &panel));

    ESP_LOGI(TAG, "Resetting and initializing ST7796");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));

    /* BGR (0x08) + axis swap/MV (0x20) -> expected MADCTL 0x28. */
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    uint16_t *draw_buf = (uint16_t *)esp_lcd_i80_alloc_draw_buffer(
        io, LCD_DRAW_BUF_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (draw_buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate %u-byte internal DMA draw buffer",
                 (unsigned)LCD_DRAW_BUF_BYTES);
        abort();
    }

    /* Turn backlight on only after the controller has been initialized. */
    ESP_ERROR_CHECK(gpio_set_level(PIN_LCD_BL, 1));

    ESP_LOGI(TAG, "LCD initialized; starting visual pattern loop");
    printf("\nVISUAL ACCEPTANCE TARGET:\n");
    printf("  - solid black, white, red, green, blue screens\n");
    printf("  - 8 vertical color bars\n");
    printf("  - smooth left-to-right grayscale\n");
    printf("  - geometry screen: TL red, TR green, BL blue, BR white\n");
    printf("  - cyan border, yellow center cross, gray 40-pixel grid\n");
    printf("  - no persistent corruption, gross flicker or missing regions\n\n");

    static const char *pattern_names[] = {
        "SOLID BLACK",
        "SOLID WHITE",
        "SOLID RED",
        "SOLID GREEN",
        "SOLID BLUE",
        "8 COLOR BARS",
        "HORIZONTAL GRAYSCALE",
        "ORIENTATION / GEOMETRY",
    };

    unsigned cycle = 0;
    while (true) {
        ESP_LOGI(TAG, "=== DISPLAY TEST CYCLE %u ===", cycle++);
        for (int pattern = 0; pattern < 8; ++pattern) {
            show_pattern(panel, draw_buf, pattern, pattern_names[pattern]);
            vTaskDelay(pdMS_TO_TICKS(pattern == 7 ? 4000 : 1800));
        }
        ESP_LOGI(TAG, "Cycle complete. Physical PASS requires operator visual confirmation.");
    }
}
