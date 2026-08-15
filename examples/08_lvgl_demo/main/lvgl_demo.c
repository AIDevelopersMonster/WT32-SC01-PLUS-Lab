#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7796.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "08_lvgl_demo";

#define LCD_H_RES               480
#define LCD_V_RES               320
#define LCD_BUS_WIDTH           8
#define LCD_PCLK_HZ             (10 * 1000 * 1000)
#define LCD_LVGL_BUFFER_LINES   32
#define LCD_LVGL_BUFFER_PIXELS  (LCD_H_RES * LCD_LVGL_BUFFER_LINES)

#define PIN_LCD_BL              GPIO_NUM_45
#define PIN_LCD_RST             GPIO_NUM_4
#define PIN_LCD_DC              GPIO_NUM_0
#define PIN_LCD_WR              GPIO_NUM_47
#define PIN_LCD_CS              (-1)
#define PIN_LCD_D0              GPIO_NUM_9
#define PIN_LCD_D1              GPIO_NUM_46
#define PIN_LCD_D2              GPIO_NUM_3
#define PIN_LCD_D3              GPIO_NUM_8
#define PIN_LCD_D4              GPIO_NUM_18
#define PIN_LCD_D5              GPIO_NUM_17
#define PIN_LCD_D6              GPIO_NUM_16
#define PIN_LCD_D7              GPIO_NUM_15

#define TOUCH_SDA_GPIO          GPIO_NUM_6
#define TOUCH_SCL_GPIO          GPIO_NUM_5
#define TOUCH_INT_GPIO          GPIO_NUM_7
#define TOUCH_I2C_PORT          I2C_NUM_1
#define TOUCH_I2C_FREQ_HZ       400000
#define TOUCH_I2C_ADDR          0x38
#define TOUCH_READ_TIMEOUT_MS   20

#define REG_TD_STATUS           0x02
#define REG_P1_XH               0x03
#define REG_ID_G_CIPHER_LOW     0xA0
#define FT6336U_CHIP_CODE       0x02

static esp_lcd_i80_bus_handle_t s_i80_bus;
static esp_lcd_panel_io_handle_t s_lcd_io;
static esp_lcd_panel_handle_t s_panel;
static i2c_master_bus_handle_t s_touch_bus;
static i2c_master_dev_handle_t s_touch_dev;
static lv_display_t *s_lvgl_display;
static lv_indev_t *s_touch_indev;
static lv_obj_t *s_status_label;
static lv_obj_t *s_count_label;
static unsigned s_click_count;
static int32_t s_last_x = LCD_H_RES / 2;
static int32_t s_last_y = LCD_V_RES / 2;

typedef struct {
    const char *name;
    uint32_t color;
} demo_button_info_t;

static const demo_button_info_t s_button_info[] = {
    { "RED",   0xD73535 },
    { "GREEN", 0x2E9B55 },
    { "BLUE",  0x3478C8 },
};

static esp_err_t touch_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_touch_dev, &reg, 1, data, len, TOUCH_READ_TIMEOUT_MS);
}

static bool touch_read_raw(uint16_t *raw_x, uint16_t *raw_y)
{
    uint8_t points_raw = 0;
    if (touch_read_reg(REG_TD_STATUS, &points_raw, 1) != ESP_OK) {
        return false;
    }

    const uint8_t points = points_raw & 0x0F;
    if (points == 0 || points > 2) {
        return false;
    }

    uint8_t raw[4] = {0};
    if (touch_read_reg(REG_P1_XH, raw, sizeof(raw)) != ESP_OK) {
        return false;
    }

    *raw_x = ((uint16_t)(raw[0] & 0x0F) << 8) | raw[1];
    *raw_y = ((uint16_t)(raw[2] & 0x0F) << 8) | raw[3];
    return true;
}

static void map_touch_to_lcd(uint16_t raw_x, uint16_t raw_y, int32_t *lcd_x, int32_t *lcd_y)
{
    /*
     * Physically validated on panlee-v15-230208-sample-a:
     *   swap_xy=true, mirror_x=false, mirror_y=true
     *   lcd_x = raw_y
     *   lcd_y = 319 - raw_x
     */
    int32_t x = (int32_t)raw_y;
    int32_t y = (LCD_V_RES - 1) - (int32_t)raw_x;

    if (x < 0) x = 0;
    if (x >= LCD_H_RES) x = LCD_H_RES - 1;
    if (y < 0) y = 0;
    if (y >= LCD_V_RES) y = LCD_V_RES - 1;

    *lcd_x = x;
    *lcd_y = y;
}

static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    uint16_t raw_x = 0;
    uint16_t raw_y = 0;

    if (touch_read_raw(&raw_x, &raw_y)) {
        map_touch_to_lcd(raw_x, raw_y, &s_last_x, &s_last_y);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->point.x = s_last_x;
    data->point.y = s_last_y;
}

static void button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    const demo_button_info_t *info = (const demo_button_info_t *)lv_event_get_user_data(event);
    ++s_click_count;

    lv_label_set_text_fmt(s_status_label, "Last button: %s", info->name);
    lv_label_set_text_fmt(s_count_label, "LVGL touch events: %u", s_click_count);
}

static void create_demo_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "WT32-SC01-PLUS  |  LVGL + TOUCH");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "ST7796 480x320  |  FT6336U @ 0x38");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x76C7FF), LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *status_panel = lv_obj_create(screen);
    lv_obj_set_size(status_panel, 440, 64);
    lv_obj_align(status_panel, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_bg_color(status_panel, lv_color_hex(0x1C2936), LV_PART_MAIN);
    lv_obj_set_style_border_color(status_panel, lv_color_hex(0x3E8FC7), LV_PART_MAIN);
    lv_obj_set_style_border_width(status_panel, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(status_panel, 10, LV_PART_MAIN);
    lv_obj_remove_flag(status_panel, LV_OBJ_FLAG_SCROLLABLE);

    s_status_label = lv_label_create(status_panel);
    lv_label_set_text(s_status_label, "Touch a button below");
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(s_status_label, LV_ALIGN_TOP_MID, 0, 2);

    s_count_label = lv_label_create(status_panel);
    lv_label_set_text(s_count_label, "LVGL touch events: 0");
    lv_obj_set_style_text_color(s_count_label, lv_color_hex(0xA9D6A9), LV_PART_MAIN);
    lv_obj_align(s_count_label, LV_ALIGN_BOTTOM_MID, 0, -2);

    const int button_y = 166;
    const int button_w = 128;
    const int button_h = 92;
    const int button_x[] = {42, 176, 310};

    for (size_t i = 0; i < 3; ++i) {
        lv_obj_t *button = lv_button_create(screen);
        lv_obj_set_size(button, button_w, button_h);
        lv_obj_set_pos(button, button_x[i], button_y);
        lv_obj_set_style_bg_color(button, lv_color_hex(s_button_info[i].color), LV_PART_MAIN);
        lv_obj_set_style_radius(button, 14, LV_PART_MAIN);
        lv_obj_add_event_cb(button, button_event_cb, LV_EVENT_CLICKED, (void *)&s_button_info[i]);

        lv_obj_t *label = lv_label_create(button);
        lv_label_set_text(label, s_button_info[i].name);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_center(label);
    }

    lv_obj_t *footer = lv_label_create(screen);
    lv_label_set_text(footer, "mapped touch: lcd_x = raw_y, lcd_y = 319 - raw_x");
    lv_obj_set_style_text_color(footer, lv_color_hex(0x9AA9B5), LV_PART_MAIN);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -16);
}

static void init_display_hardware(void)
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

    const esp_lcd_i80_bus_config_t bus_cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .dc_gpio_num = PIN_LCD_DC,
        .wr_gpio_num = PIN_LCD_WR,
        .data_gpio_nums = {
            PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3,
            PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7,
        },
        .bus_width = LCD_BUS_WIDTH,
        .max_transfer_bytes = LCD_LVGL_BUFFER_PIXELS * sizeof(uint16_t),
        .dma_burst_size = 64,
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_cfg, &s_i80_bus));

    const esp_lcd_panel_io_i80_config_t io_cfg = {
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = LCD_PCLK_HZ,
        .trans_queue_depth = 6,
        .on_color_trans_done = NULL,
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
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(s_i80_bus, &io_cfg, &s_lcd_io));

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(s_lcd_io, &panel_cfg, &s_panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    /* GPIO4 reset is shared with touch; allow FT6336U to finish booting. */
    vTaskDelay(pdMS_TO_TICKS(250));
}

static void init_touch_hardware(void)
{
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
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &s_touch_bus));

    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TOUCH_I2C_ADDR,
        .scl_speed_hz = TOUCH_I2C_FREQ_HZ,
        .scl_wait_us = 0,
        .flags.disable_ack_check = false,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(s_touch_bus, &dev_cfg, &s_touch_dev));

    uint8_t chip_code = 0;
    ESP_ERROR_CHECK(touch_read_reg(REG_ID_G_CIPHER_LOW, &chip_code, 1));
    if (chip_code != FT6336U_CHIP_CODE) {
        ESP_LOGE(TAG, "Unexpected touch signature at 0xA0: 0x%02X", chip_code);
        abort();
    }

    ESP_LOGI(TAG, "FT6336U-compatible touch signature confirmed: 0xA0=0x%02X", chip_code);
}

static void init_lvgl(void)
{
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t display_cfg = {
        .io_handle = s_lcd_io,
        .panel_handle = s_panel,
        .control_handle = s_panel,
        .buffer_size = LCD_LVGL_BUFFER_PIXELS,
        .double_buffer = true,
        .trans_size = 0,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .rounder_cb = NULL,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
            .swap_bytes = false,
            .full_refresh = false,
            .direct_mode = false,
        },
    };

    s_lvgl_display = lvgl_port_add_disp(&display_cfg);
    if (s_lvgl_display == NULL) {
        ESP_LOGE(TAG, "lvgl_port_add_disp failed");
        abort();
    }

    if (!lvgl_port_lock(0)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        abort();
    }

    lv_display_set_default(s_lvgl_display);

    s_touch_indev = lv_indev_create();
    if (s_touch_indev == NULL) {
        lvgl_port_unlock();
        ESP_LOGE(TAG, "lv_indev_create failed");
        abort();
    }

    lv_indev_set_type(s_touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_touch_indev, lvgl_touch_read_cb);
    lv_indev_set_display(s_touch_indev, s_lvgl_display);

    create_demo_ui();
    lvgl_port_unlock();
}

void app_main(void)
{
    printf("\n");
    printf("================================================================\n");
    printf(" WT32-SC01-PLUS-Lab / 08_lvgl_demo\n");
    printf(" Integrated ST7796 + FT6336U-compatible + LVGL touch demo\n");
    printf("================================================================\n");
    printf(" LCD          : 480x320 ST7796-compatible, I80 8-bit, 10 MHz\n");
    printf(" Touch        : FT6336U-compatible @ 0x38, I2C1 400 kHz\n");
    printf(" Touch pins   : SDA6 SCL5 INT7 RST4(shared)\n");
    printf(" Touch map    : lcd_x=raw_y, lcd_y=319-raw_x\n");
    printf(" LVGL         : managed component via esp_lvgl_port\n");
    printf(" Persistence  : none\n");
    printf("================================================================\n\n");

    ESP_LOGI(TAG, "Initializing validated LCD hardware path");
    init_display_hardware();

    ESP_LOGI(TAG, "Initializing validated touch hardware path");
    init_touch_hardware();

    ESP_LOGI(TAG, "Initializing LVGL and pointer input");
    init_lvgl();

    ESP_ERROR_CHECK(gpio_set_level(PIN_LCD_BL, 1));

    printf("[READY]\n");
    printf("Touch RED / GREEN / BLUE buttons on the display.\n");
    printf("Each successful LVGL click increments the on-screen event counter.\n");
    printf("RESULT: READY FOR PHYSICAL LVGL TOUCH VALIDATION\n");
    printf("END 08_lvgl_demo startup\n");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
