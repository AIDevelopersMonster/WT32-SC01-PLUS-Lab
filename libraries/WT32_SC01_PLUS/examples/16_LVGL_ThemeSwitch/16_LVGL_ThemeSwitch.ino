/*
 * WT32-SC01-PLUS-Lab
 * Example 16: LVGL Theme Switch
 *
 * Target hardware:
 *   Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208
 *
 * Purpose:
 *   Extend the physically validated LVGL navigation + Device Info examples
 *   with a reusable Light/Dark theme layer and live theme switching.
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
constexpr uint32_t kKiB = 1024UL;
constexpr uint32_t kMiB = 1024UL * 1024UL;

enum class PageId : uint8_t {
    Home = 0,
    Remote,
    Settings,
    Info,
};

enum class ThemeMode : uint8_t {
    Dark = 0,
    Light,
};

struct ThemePalette {
    uint32_t background;
    uint32_t panel;
    uint32_t nav;
    uint32_t accent;
    uint32_t muted;
    uint32_t text;
    uint32_t good;
};

constexpr ThemePalette kDarkTheme = {
    0x101820,
    0x18242F,
    0x0B1118,
    0x26A69A,
    0x90A4AE,
    0xFFFFFF,
    0x80CBC4,
};

constexpr ThemePalette kLightTheme = {
    0xF3F6F8,
    0xFFFFFF,
    0xDCE5EA,
    0x00796B,
    0x546E7A,
    0x102027,
    0x00695C,
};

static lv_color_t drawBufferPixels[wt32sc01plus::pins::LCD_WIDTH * kBufferLines];
static lv_disp_draw_buf_t drawBuffer;
static lv_disp_drv_t displayDriver;
static lv_indev_drv_t touchDriver;

static lv_obj_t *contentArea = nullptr;
static lv_obj_t *navBar = nullptr;
static lv_obj_t *navButtons[kPageCount] = {nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *chipValue = nullptr;
static lv_obj_t *memoryValue = nullptr;
static lv_obj_t *runtimeValue = nullptr;

static PageId currentPage = PageId::Home;
static ThemeMode currentTheme = ThemeMode::Dark;

const ThemePalette &theme() {
    return currentTheme == ThemeMode::Light ? kLightTheme : kDarkTheme;
}

const char *themeName() {
    return currentTheme == ThemeMode::Light ? "LIGHT" : "DARK";
}

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

lv_obj_t *makeLabel(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y,
                    uint32_t color) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_pos(label, x, y);
    return label;
}

lv_obj_t *makeTextLabel(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y) {
    return makeLabel(parent, text, x, y, theme().text);
}

lv_obj_t *makeMutedLabel(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y) {
    return makeLabel(parent, text, x, y, theme().muted);
}

lv_obj_t *makeCard(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                   lv_coord_t width, lv_coord_t height) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, width, height);
    lv_obj_set_style_bg_color(card, lv_color_hex(theme().panel), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

void clearContent() {
    chipValue = nullptr;
    memoryValue = nullptr;
    runtimeValue = nullptr;
    lv_obj_clean(contentArea);
}

void styleActionButton(lv_obj_t *button) {
    lv_obj_set_style_bg_color(button, lv_color_hex(theme().accent), LV_PART_MAIN);
    lv_obj_set_style_text_color(button, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_radius(button, 10, LV_PART_MAIN);
}

void buildHomePage() {
    makeLabel(contentArea, "HOME", 20, 18, theme().accent);
    makeTextLabel(contentArea, "Reusable LVGL HMI with live Light/Dark themes.", 20, 54);
    makeMutedLabel(contentArea, "Theme changes preserve the navigation architecture.", 20, 84);

    lv_obj_t *card = makeCard(contentArea, 40, 122, 400, 96);
    lv_obj_t *label = lv_label_create(card);
    lv_label_set_text_fmt(label,
                          "Current theme: %s\n"
                          "Open SETTINGS to switch appearance.",
                          themeName());
    lv_obj_set_style_text_color(label, lv_color_hex(theme().good), 0);
    lv_obj_center(label);
}

void onRemoteButton(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    const intptr_t index = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    Serial.printf("REMOTE action placeholder: %d\n", static_cast<int>(index));
}

void buildRemotePage() {
    makeLabel(contentArea, "REMOTE", 20, 18, theme().accent);
    makeMutedLabel(contentArea, "Theme-aware command surface", 20, 50);

    for (int i = 0; i < 6; ++i) {
        lv_obj_t *button = lv_btn_create(contentArea);
        lv_obj_set_size(button, 110, 56);
        const int col = i % 3;
        const int row = i / 3;
        lv_obj_set_pos(button, 28 + col * 140, 92 + row * 76);
        styleActionButton(button);
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

void showPage(PageId page);

void applyThemeAsync(void *) {
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(theme().background), 0);
    lv_obj_set_style_bg_color(contentArea, lv_color_hex(theme().background), 0);
    lv_obj_set_style_bg_color(navBar, lv_color_hex(theme().nav), 0);

    Serial.printf("THEME: %s\n", themeName());
    showPage(currentPage);
}

void onThemeSwitch(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) return;

    lv_obj_t *toggle = lv_event_get_target(event);
    const ThemeMode requested = lv_obj_has_state(toggle, LV_STATE_CHECKED)
                                    ? ThemeMode::Light
                                    : ThemeMode::Dark;

    if (requested == currentTheme) return;

    currentTheme = requested;

    // The current Settings page contains the switch that generated this event.
    // Rebuild it asynchronously so LVGL does not delete the event source while
    // its callback is still executing.
    lv_async_call(applyThemeAsync, nullptr);
}

void buildSettingsPage() {
    makeLabel(contentArea, "SETTINGS", 20, 18, theme().accent);

    makeTextLabel(contentArea, "Appearance", 20, 58);
    makeMutedLabel(contentArea, "Dark", 150, 58);

    lv_obj_t *themeSwitch = lv_switch_create(contentArea);
    lv_obj_set_pos(themeSwitch, 205, 51);
    if (currentTheme == ThemeMode::Light) {
        lv_obj_add_state(themeSwitch, LV_STATE_CHECKED);
    }
    lv_obj_set_style_bg_color(themeSwitch, lv_color_hex(theme().panel), LV_PART_MAIN);
    lv_obj_set_style_bg_color(themeSwitch, lv_color_hex(theme().accent), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(themeSwitch, onThemeSwitch, LV_EVENT_VALUE_CHANGED, nullptr);

    makeMutedLabel(contentArea, "Light", 270, 58);

    makeTextLabel(contentArea, "Backlight", 20, 116);
    lv_obj_t *slider = lv_slider_create(contentArea);
    lv_obj_set_width(slider, 300);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, 80, LV_ANIM_OFF);
    lv_obj_set_pos(slider, 130, 119);
    lv_obj_set_style_bg_color(slider, lv_color_hex(theme().panel), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(theme().accent), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(theme().accent), LV_PART_KNOB);
    lv_obj_add_event_cb(slider, onBrightness, LV_EVENT_VALUE_CHANGED, nullptr);

    makeMutedLabel(contentArea,
                   "Theme affects UI only; brightness remains physical BSP control.",
                   20, 174);
}

void updateDeviceInfo(lv_timer_t *) {
    if (!chipValue || !memoryValue || !runtimeValue) return;

    lv_label_set_text_fmt(
        chipValue,
        "%s rev %u | %u cores | %u MHz\nArduino %s | IDF %s",
        ESP.getChipModel(),
        static_cast<unsigned>(ESP.getChipRevision()),
        static_cast<unsigned>(ESP.getChipCores()),
        static_cast<unsigned>(ESP.getCpuFreqMHz()),
        ESP.getCoreVersion(),
        ESP.getSdkVersion());

    lv_label_set_text_fmt(
        memoryValue,
        "Flash %lu MiB | PSRAM %lu MiB\nHeap %lu / %lu KiB free | PSRAM %lu KiB free",
        static_cast<unsigned long>(ESP.getFlashChipSize() / kMiB),
        static_cast<unsigned long>(ESP.getPsramSize() / kMiB),
        static_cast<unsigned long>(ESP.getFreeHeap() / kKiB),
        static_cast<unsigned long>(ESP.getHeapSize() / kKiB),
        static_cast<unsigned long>(ESP.getFreePsram() / kKiB));

    lv_label_set_text_fmt(
        runtimeValue,
        "Sketch %lu KiB | free slot %lu KiB\nUptime %lu s | min heap %lu KiB\nTheme %s | touch 0x%02X fw 0x%02X",
        static_cast<unsigned long>(ESP.getSketchSize() / kKiB),
        static_cast<unsigned long>(ESP.getFreeSketchSpace() / kKiB),
        static_cast<unsigned long>(millis() / 1000UL),
        static_cast<unsigned long>(ESP.getMinFreeHeap() / kKiB),
        themeName(),
        board.touch().chipCode(),
        board.touch().firmwareId());
}

void buildInfoPage() {
    makeLabel(contentArea, "DEVICE INFO", 20, 12, theme().accent);
    makeMutedLabel(contentArea, "Live runtime values", 170, 14);

    lv_obj_t *chipCard = makeCard(contentArea, 18, 46, 444, 60);
    chipValue = lv_label_create(chipCard);
    lv_obj_set_style_text_color(chipValue, lv_color_hex(theme().text), 0);
    lv_obj_center(chipValue);

    lv_obj_t *memoryCard = makeCard(contentArea, 18, 112, 444, 60);
    memoryValue = lv_label_create(memoryCard);
    lv_obj_set_style_text_color(memoryValue, lv_color_hex(theme().good), 0);
    lv_obj_center(memoryValue);

    lv_obj_t *runtimeCard = makeCard(contentArea, 18, 178, 444, 72);
    runtimeValue = lv_label_create(runtimeCard);
    lv_obj_set_style_text_color(runtimeValue, lv_color_hex(theme().text), 0);
    lv_obj_center(runtimeValue);

    updateDeviceInfo(nullptr);
}

void updateNavStyle() {
    for (uint8_t i = 0; i < kPageCount; ++i) {
        const bool active = i == static_cast<uint8_t>(currentPage);
        lv_obj_set_style_bg_color(
            navButtons[i], lv_color_hex(active ? theme().accent : theme().panel), LV_PART_MAIN);
        lv_obj_set_style_text_color(
            navButtons[i], lv_color_hex(active ? 0xFFFFFF : theme().muted), LV_PART_MAIN);
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
    lv_obj_set_style_bg_color(screen, lv_color_hex(theme().background), 0);

    contentArea = lv_obj_create(screen);
    lv_obj_set_pos(contentArea, 0, 0);
    lv_obj_set_size(contentArea, 480, 258);
    lv_obj_set_style_bg_color(contentArea, lv_color_hex(theme().background), 0);
    lv_obj_set_style_border_width(contentArea, 0, 0);
    lv_obj_set_style_radius(contentArea, 0, 0);
    lv_obj_clear_flag(contentArea, LV_OBJ_FLAG_SCROLLABLE);

    navBar = lv_obj_create(screen);
    lv_obj_set_pos(navBar, 0, 258);
    lv_obj_set_size(navBar, 480, 62);
    lv_obj_set_style_bg_color(navBar, lv_color_hex(theme().nav), 0);
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
    Serial.println("WT32-SC01-PLUS 16_LVGL_ThemeSwitch");

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
    lv_timer_create(updateDeviceInfo, 1000, nullptr);

    Serial.println("THEME: DARK");
    Serial.println("READY: LVGL theme switch initialized");
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
