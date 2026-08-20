/*
 * WT32-SC01-PLUS-Lab
 * Example 13: LVGL Basic UI
 *
 * Target hardware:
 *   Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208
 *   ESP32-S3 + ST7796 480x320 I80 + FT6336U-compatible touch
 *
 * Project lead and physical validation: Alex Malachevsky
 * Engineering collaboration: Commander Sol
 * Repository: https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab
 * License: MIT
 *
 * Purpose
 * -------
 * This example is the minimal bridge between the WT32_SC01_PLUS BSP and LVGL 8.
 * It demonstrates the reusable application architecture used by later HMI examples:
 *
 *   Arduino application
 *          -> LVGL widgets/events
 *          -> WT32_SC01_PLUS BSP
 *          -> ST7796 display + FT6336U-compatible touch + backlight PWM
 *
 * The sketch intentionally contains no board GPIO table and no direct ST7796 or
 * FT6336 register handling. Hardware-specific work remains inside the BSP.
 *
 * Demonstrated controls:
 *   - LVGL push button with a live press counter
 *   - LVGL slider connected to the physical LCD backlight
 *   - touch input exposed to LVGL as a pointer device
 *
 * Dependency: LVGL 8.3.11
 * Build note: build_opt.h applies -DLV_CONF_SKIP to the complete Arduino build
 * so LVGL's own C/C++ translation units use the same configuration path.
 * Physical status: PASS on the named Panlee specimen, 2026-08-20
 */

#include <WT32_SC01_PLUS.h>

// LV_CONF_SKIP is supplied globally by this example's build_opt.h. Defining it
// there (rather than only in this .ino file) is important because Arduino builds
// LVGL's library sources as separate translation units.
#include <lvgl.h>

WT32_SC01_PLUS board;

namespace {
// A 20-line partial draw buffer keeps RAM use modest. LVGL renders into this
// buffer and flushDisplay() sends each dirty rectangle through the BSP.
constexpr uint16_t kBufferLines = 20;
static lv_color_t drawBufferPixels[wt32sc01plus::pins::LCD_WIDTH * kBufferLines];
static lv_disp_draw_buf_t drawBuffer;
static lv_disp_drv_t displayDriver;
static lv_indev_drv_t touchDriver;

// Application state used by the demonstration button.
static uint32_t buttonPresses = 0;
static lv_obj_t *counterLabel = nullptr;

// LVGL display callback.
// LVGL supplies an RGB565 rectangle; the BSP performs the synchronized I80
// transfer to the physical ST7796 display. LVGL is then told the flush is done.
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

// LVGL pointer callback.
// The BSP already converts the physical touch controller's raw coordinates to
// the validated 480x320 landscape coordinate system, so LVGL receives normal
// screen coordinates and does not need to know the touch-controller pinout.
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

// Standard LVGL event handler: every completed click increments application
// state and immediately updates the label on screen.
void onButton(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;

    ++buttonPresses;
    lv_label_set_text_fmt(counterLabel, "Button presses: %lu",
                          static_cast<unsigned long>(buttonPresses));
}

// The slider demonstrates a direct UI -> hardware control path. Its value is
// passed to the BSP backlight driver as a percentage from 10 to 100.
void onBrightness(lv_event_t *event) {
    auto *slider = lv_event_get_target(event);
    board.backlight().set(static_cast<uint8_t>(lv_slider_get_value(slider)));
}

// Build a deliberately small UI so each LVGL concept is easy to reuse in a
// larger HMI: labels, one button/event, and one hardware-connected slider.
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
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("WT32-SC01-PLUS 13_LVGL_BasicUI");

    // board.begin() intentionally initializes the display and backlight only.
    // Touch has its own explicit begin() call because applications may use the
    // BSP without a touch interface. The first physical revision of this example
    // omitted the next call; that failure and correction are preserved in README.
    if (!board.begin()) {
        Serial.println("ERROR: BSP display/backlight initialization failed");
        while (true) delay(1000);
    }

    if (!board.touch().begin()) {
        Serial.println("ERROR: touch initialization failed");
        while (true) delay(1000);
    }

    Serial.printf("Touch ready: chip=0x%02X firmware=0x%02X focaltech=0x%02X\n",
                  board.touch().chipCode(),
                  board.touch().firmwareId(),
                  board.touch().focalTechId());

    board.backlight().set(80);

    // LVGL is configured for RGB565. This compile-time check protects the
    // zero-copy reinterpretation used by flushDisplay().
    lv_init();
    static_assert(sizeof(lv_color_t) == sizeof(uint16_t), "LVGL must use 16-bit color");

    lv_disp_draw_buf_init(
        &drawBuffer,
        drawBufferPixels,
        nullptr,
        wt32sc01plus::pins::LCD_WIDTH * kBufferLines);

    // Register the BSP-backed physical display with LVGL.
    lv_disp_drv_init(&displayDriver);
    displayDriver.hor_res = wt32sc01plus::pins::LCD_WIDTH;
    displayDriver.ver_res = wt32sc01plus::pins::LCD_HEIGHT;
    displayDriver.flush_cb = flushDisplay;
    displayDriver.draw_buf = &drawBuffer;
    lv_disp_drv_register(&displayDriver);

    // Register the validated touch path as a standard LVGL pointer device.
    lv_indev_drv_init(&touchDriver);
    touchDriver.type = LV_INDEV_TYPE_POINTER;
    touchDriver.read_cb = readTouch;
    lv_indev_drv_register(&touchDriver);

    buildUi();
    Serial.println("READY: LVGL display + touch UI initialized");
}

void loop() {
    // LVGL 8 needs a monotonically advancing millisecond tick plus regular
    // calls to lv_timer_handler() for input polling, event dispatch and redraws.
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
