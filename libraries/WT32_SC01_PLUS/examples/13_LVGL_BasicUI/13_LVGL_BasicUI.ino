#include <WT32_SC01_PLUS.h>

#define LV_CONF_SKIP 1
#include <lvgl.h>

WT32_SC01_PLUS board;

namespace {
constexpr uint16_t kBufferLines = 20;
static lv_color_t drawBufferPixels[wt32sc01plus::pins::LCD_WIDTH * kBufferLines];
static lv_disp_draw_buf_t drawBuffer;
static lv_disp_drv_t displayDriver;
static lv_indev_drv_t touchDriver;
static uint32_t buttonPresses = 0;
static lv_obj_t *counterLabel = nullptr;

void flushDisplay(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *colorPixels) {
    const int width = area->x2 - area->x1 + 1;
    const int height = area->y2 - area->y1 + 1;

    board.display().drawRGB565(
        area->x1,
        area->y1,
        width,
        height,
        reinterpret_cast<const uint16_t *>(colorPixels));

    lv_disp_flush_ready(drv);
}

void readTouch(lv_indev_drv_t *, lv_indev_data_t *data) {
    WT32_SC01_PLUS_TouchPoint point;
    if (!board.touch().read(point) || !point.touched) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    data->state = LV_INDEV_STATE_PR;
    data->point.x = point.x;
    data->point.y = point.y;
}

void onButton(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;

    ++buttonPresses;
    lv_label_set_text_fmt(counterLabel, "Button presses: %lu",
                          static_cast<unsigned long>(buttonPresses));
}

void onBrightness(lv_event_t *event) {
    auto *slider = lv_event_get_target(event);
    board.backlight().set(static_cast<uint8_t>(lv_slider_get_value(slider)));
}

void buildUi() {
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "WT32-SC01-PLUS + LVGL 8");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "Panlee BSP basic interactive UI");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x80CBC4), 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 54);

    lv_obj_t *button = lv_btn_create(screen);
    lv_obj_set_size(button, 150, 64);
    lv_obj_align(button, LV_ALIGN_CENTER, 0, -35);
    lv_obj_add_event_cb(button, onButton, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *buttonLabel = lv_label_create(button);
    lv_label_set_text(buttonLabel, "Tap me");
    lv_obj_center(buttonLabel);

    counterLabel = lv_label_create(screen);
    lv_label_set_text(counterLabel, "Button presses: 0");
    lv_obj_set_style_text_color(counterLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(counterLabel, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t *brightnessLabel = lv_label_create(screen);
    lv_label_set_text(brightnessLabel, "Backlight");
    lv_obj_set_style_text_color(brightnessLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(brightnessLabel, LV_ALIGN_BOTTOM_LEFT, 40, -58);

    lv_obj_t *slider = lv_slider_create(screen);
    lv_obj_set_width(slider, 260);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, 80, LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_RIGHT, -40, -62);
    lv_obj_add_event_cb(slider, onBrightness, LV_EVENT_VALUE_CHANGED, nullptr);
}
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("WT32-SC01-PLUS 13_LVGL_BasicUI");

    if (!board.begin()) {
        Serial.println("ERROR: BSP initialization failed");
        while (true) delay(1000);
    }

    board.backlight().set(80);

    lv_init();
    static_assert(sizeof(lv_color_t) == sizeof(uint16_t), "LVGL must use 16-bit color");

    lv_disp_draw_buf_init(
        &drawBuffer,
        drawBufferPixels,
        nullptr,
        wt32sc01plus::pins::LCD_WIDTH * kBufferLines);

    lv_disp_drv_init(&displayDriver);
    displayDriver.hor_res = wt32sc01plus::pins::LCD_WIDTH;
    displayDriver.ver_res = wt32sc01plus::pins::LCD_HEIGHT;
    displayDriver.flush_cb = flushDisplay;
    displayDriver.draw_buf = &drawBuffer;
    lv_disp_drv_register(&displayDriver);

    lv_indev_drv_init(&touchDriver);
    touchDriver.type = LV_INDEV_TYPE_POINTER;
    touchDriver.read_cb = readTouch;
    lv_indev_drv_register(&touchDriver);

    buildUi();
    Serial.println("READY: LVGL display + touch UI initialized");
}

void loop() {
    static uint32_t previousMs = millis();
    const uint32_t nowMs = millis();
    const uint32_t elapsedMs = nowMs - previousMs;
    if (elapsedMs > 0) {
        lv_tick_inc(elapsedMs);
        previousMs = nowMs;
    }

    lv_timer_handler();
    delay(5);
}