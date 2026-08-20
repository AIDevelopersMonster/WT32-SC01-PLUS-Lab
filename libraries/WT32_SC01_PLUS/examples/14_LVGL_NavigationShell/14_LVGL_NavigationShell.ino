/*
 * WT32-SC01-PLUS-Lab
 * Example 14: LVGL Navigation Shell
 *
 * Target hardware:
 *   Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208
 *
 * Purpose:
 *   Reusable multi-page LVGL application shell built on the physically validated
 *   BSP/LVGL bridge from 13_LVGL_BasicUI.
 *
 * Architecture:
 *   Arduino application
 *        -> page builders + centralized navigation
 *        -> LVGL 8 widgets/events
 *        -> WT32_SC01_PLUS BSP
 *        -> ST7796 display + FT6336U-compatible touch
 *
 * Dependency: LVGL 8.3.11
 * Status: SOURCE / CI TARGET. Physical validation required before PASS.
 */

#include <WT32_SC01_PLUS.h>
#include <lvgl.h>

WT32_SC01_PLUS board;

namespace {
constexpr uint16_t kBufferLines = 20;
constexpr uint8_t kPageCount = 4;

enum class PageId : uint8_t {
    Home = 0,
    Remote,
    Settings,
    Info,
};

static lv_color_t drawBufferPixels[wt32sc01plus::pins::LCD_WIDTH * kBufferLines];
static lv_disp_draw_buf_t drawBuffer;
static lv_disp_drv_t displayDriver;
static lv_indev_drv_t touchDriver;

static lv_obj_t *contentArea = nullptr;
static lv_obj_t *navButtons[kPageCount] = {nullptr, nullptr, nullptr, nullptr};
static PageId currentPage = PageId::Home;

constexpr uint32_t kBg = 0x101820;
constexpr uint32_t kPanel = 0x18242F;
constexpr uint32_t kAccent = 0x26A69A;
constexpr uint32_t kMuted = 0x90A4AE;
constexpr uint32_t kWhite = 0xFFFFFF;

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

lv_obj_t *makeLabel(lv_obj_t *parent, const char *text, lv_coord_t y, uint32_t color = kWhite) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 20, y);
    return label;
}

void clearContent() {
    lv_obj_clean(contentArea);
}

void buildHomePage() {
    makeLabel(contentArea, "HOME", 18, kAccent);
    makeLabel(contentArea, "Reusable WT32-SC01-PLUS HMI shell", 50);
    makeLabel(contentArea, "Navigation is centralized in showPage().", 82, kMuted);

    lv_obj_t *card = lv_obj_create(contentArea);
    lv_obj_set_size(card, 400, 110);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 25);
    lv_obj_set_style_bg_color(card, lv_color_hex(kPanel), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 12, 0);

    lv_obj_t *label = lv_label_create(card);
    lv_label_set_text(label,
                      "Next modules can populate this shell with\n"
                      "status, sensors, clock, weather or controls.");
    lv_obj_set_style_text_color(label, lv_color_hex(kWhite), 0);
    lv_obj_center(label);
}

void onRemoteButton(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    const intptr_t index = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    Serial.printf("REMOTE action placeholder: %d\n", static_cast<int>(index));
}

void buildRemotePage() {
    makeLabel(contentArea, "REMOTE", 18, kAccent);
    makeLabel(contentArea, "Six generic command slots", 50, kMuted);

    for (int i = 0; i < 6; ++i) {
        lv_obj_t *button = lv_btn_create(contentArea);
        lv_obj_set_size(button, 110, 56);
        const int col = i % 3;
        const int row = i / 3;
        lv_obj_set_pos(button, 28 + col * 140, 92 + row * 76);
        lv_obj_add_event_cb(button, onRemoteButton, LV_EVENT_CLICKED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(i + 1)));

        lv_obj_t *label = lv_label_create(button);
        lv_label_set_text_fmt(label, "CMD %d", i + 1);
        lv_obj_center(label);
    }
}

void onBrightness(lv_event_t *event) {
    lv_obj_t *slider = lv_event_get_target(event);
    board.backlight().set(static_cast<uint8_t>(lv_slider_get_value(slider)));
}

void buildSettingsPage() {
    makeLabel(contentArea, "SETTINGS", 18, kAccent);
    makeLabel(contentArea, "Backlight", 72);

    lv_obj_t *slider = lv_slider_create(contentArea);
    lv_obj_set_width(slider, 300);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, 80, LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 130, 72);
    lv_obj_add_event_cb(slider, onBrightness, LV_EVENT_VALUE_CHANGED, nullptr);

    makeLabel(contentArea, "Future: theme, rotation, Wi-Fi, timezone", 130, kMuted);
}

void buildInfoPage() {
    makeLabel(contentArea, "DEVICE INFO", 18, kAccent);
    makeLabel(contentArea, "Panlee WT32-SC01-PLUS", 58);
    makeLabel(contentArea, "ZX3D50CE08S-V15-USRC / 230208", 88, kMuted);
    makeLabel(contentArea, "ESP32-S3 | 480x320 | LVGL 8", 118);
    makeLabel(contentArea, "Shell status: SOURCE / physical test pending", 158, kMuted);
}

void updateNavStyle() {
    for (uint8_t i = 0; i < kPageCount; ++i) {
        const bool active = i == static_cast<uint8_t>(currentPage);
        lv_obj_set_style_bg_color(
            navButtons[i], lv_color_hex(active ? kAccent : kPanel), LV_PART_MAIN);
        lv_obj_set_style_text_color(
            navButtons[i], lv_color_hex(active ? kWhite : kMuted), LV_PART_MAIN);
    }
}

void showPage(PageId page) {
    currentPage = page;
    clearContent();

    switch (page) {
        case PageId::Home:     buildHomePage(); break;
        case PageId::Remote:   buildRemotePage(); break;
        case PageId::Settings: buildSettingsPage(); break;
        case PageId::Info:     buildInfoPage(); break;
    }

    updateNavStyle();
    Serial.printf("PAGE: %u\n", static_cast<unsigned>(page));
}

void onNav(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    const intptr_t page = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    showPage(static_cast<PageId>(page));
}

void buildNavigationShell() {
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(kBg), 0);

    contentArea = lv_obj_create(screen);
    lv_obj_set_pos(contentArea, 0, 0);
    lv_obj_set_size(contentArea, 480, 258);
    lv_obj_set_style_bg_color(contentArea, lv_color_hex(kBg), 0);
    lv_obj_set_style_border_width(contentArea, 0, 0);
    lv_obj_set_style_radius(contentArea, 0, 0);
    lv_obj_clear_flag(contentArea, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *navBar = lv_obj_create(screen);
    lv_obj_set_pos(navBar, 0, 258);
    lv_obj_set_size(navBar, 480, 62);
    lv_obj_set_style_bg_color(navBar, lv_color_hex(0x0B1118), 0);
    lv_obj_set_style_border_width(navBar, 0, 0);
    lv_obj_set_style_radius(navBar, 0, 0);
    lv_obj_set_style_pad_all(navBar, 6, 0);
    lv_obj_set_flex_flow(navBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(navBar, LV_FLEX_ALIGN_SPACE_AROUND,
                         LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(navBar, LV_OBJ_FLAG_SCROLLABLE);

    const char *labels[kPageCount] = {"HOME", "REMOTE", "SETTINGS", "INFO"};

    for (uint8_t i = 0; i < kPageCount; ++i) {
        lv_obj_t *button = lv_btn_create(navBar);
        navButtons[i] = button;
        lv_obj_set_size(button, 108, 46);
        lv_obj_set_style_radius(button, 10, 0);
        lv_obj_add_event_cb(button, onNav, LV_EVENT_CLICKED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(i)));

        lv_obj_t *label = lv_label_create(button);
        lv_label_set_text(label, labels[i]);
        lv_obj_center(label);
    }

    showPage(PageId::Home);
}
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("WT32-SC01-PLUS 14_LVGL_NavigationShell");

    if (!board.begin()) {
        Serial.println("ERROR: BSP display/backlight initialization failed");
        while (true) delay(1000);
    }

    if (!board.touch().begin()) {
        Serial.println("ERROR: touch initialization failed");
        while (true) delay(1000);
    }

    board.backlight().set(80);

    lv_init();
    static_assert(sizeof(lv_color_t) == sizeof(uint16_t), "LVGL must use 16-bit color");

    lv_disp_draw_buf_init(&drawBuffer, drawBufferPixels, nullptr,
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

    buildNavigationShell();
    Serial.println("READY: LVGL navigation shell initialized");
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
