/*
 * WT32-SC01-PLUS-Lab
 * Example 15: LVGL Device Info
 *
 * Target hardware:
 *   Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208
 *
 * Purpose:
 *   Extend the physically validated 14_LVGL_NavigationShell with a live
 *   Device Info page backed by Arduino-ESP32 runtime APIs.
 *
 * Dependency: LVGL 8.3.11
 * Physical status: PASS on the named Panlee specimen, 2026-08-20
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

static lv_color_t drawBufferPixels[wt32sc01plus::pins::LCD_WIDTH * kBufferLines];
static lv_disp_draw_buf_t drawBuffer;
static lv_disp_drv_t displayDriver;
static lv_indev_drv_t touchDriver;

static lv_obj_t *contentArea = nullptr;
static lv_obj_t *navButtons[kPageCount] = {nullptr, nullptr, nullptr, nullptr};
static PageId currentPage = PageId::Home;

static lv_obj_t *chipValue = nullptr;
static lv_obj_t *memoryValue = nullptr;
static lv_obj_t *runtimeValue = nullptr;

constexpr uint32_t kBg = 0x101820;
constexpr uint32_t kPanel = 0x18242F;
constexpr uint32_t kAccent = 0x26A69A;
constexpr uint32_t kMuted = 0x90A4AE;
constexpr uint32_t kWhite = 0xFFFFFF;
constexpr uint32_t kGood = 0x80CBC4;

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
                    uint32_t color = kWhite) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_pos(label, x, y);
    return label;
}

lv_obj_t *makeCard(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                   lv_coord_t width, lv_coord_t height) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, width, height);
    lv_obj_set_style_bg_color(card, lv_color_hex(kPanel), 0);
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

void buildHomePage() {
    makeLabel(contentArea, "HOME", 20, 18, kAccent);
    makeLabel(contentArea, "LVGL Device Info extends the validated navigation shell.", 20, 54);
    makeLabel(contentArea, "Open INFO to view live ESP32-S3 runtime data.", 20, 84, kMuted);

    lv_obj_t *card = makeCard(contentArea, 40, 122, 400, 96);
    lv_obj_t *label = lv_label_create(card);
    lv_label_set_text(label,
                      "Physical runtime evidence\n"
                      "without duplicating board pin maps.");
    lv_obj_set_style_text_color(label, lv_color_hex(kGood), 0);
    lv_obj_center(label);
}

void onRemoteButton(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    const intptr_t index = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    Serial.printf("REMOTE action placeholder: %d\n", static_cast<int>(index));
}

void buildRemotePage() {
    makeLabel(contentArea, "REMOTE", 20, 18, kAccent);
    makeLabel(contentArea, "Navigation-shell compatibility page", 20, 50, kMuted);

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
    makeLabel(contentArea, "SETTINGS", 20, 18, kAccent);
    makeLabel(contentArea, "Backlight", 20, 72);

    lv_obj_t *slider = lv_slider_create(contentArea);
    lv_obj_set_width(slider, 300);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, 80, LV_ANIM_OFF);
    lv_obj_set_pos(slider, 130, 74);
    lv_obj_add_event_cb(slider, onBrightness, LV_EVENT_VALUE_CHANGED, nullptr);

    makeLabel(contentArea, "Device Info is independent of future Wi-Fi/OTA modules.",
              20, 132, kMuted);
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
        "Sketch %lu KiB | free slot %lu KiB\nUptime %lu s | min heap %lu KiB\nTouch chip 0x%02X fw 0x%02X",
        static_cast<unsigned long>(ESP.getSketchSize() / kKiB),
        static_cast<unsigned long>(ESP.getFreeSketchSpace() / kKiB),
        static_cast<unsigned long>(millis() / 1000UL),
        static_cast<unsigned long>(ESP.getMinFreeHeap() / kKiB),
        board.touch().chipCode(),
        board.touch().firmwareId());
}

void buildInfoPage() {
    makeLabel(contentArea, "DEVICE INFO", 20, 12, kAccent);
    makeLabel(contentArea, "Live values from the running board", 170, 14, kMuted);

    lv_obj_t *chipCard = makeCard(contentArea, 18, 46, 444, 60);
    chipValue = lv_label_create(chipCard);
    lv_obj_set_style_text_color(chipValue, lv_color_hex(kWhite), 0);
    lv_obj_center(chipValue);

    lv_obj_t *memoryCard = makeCard(contentArea, 18, 112, 444, 60);
    memoryValue = lv_label_create(memoryCard);
    lv_obj_set_style_text_color(memoryValue, lv_color_hex(kGood), 0);
    lv_obj_center(memoryValue);

    lv_obj_t *runtimeCard = makeCard(contentArea, 18, 178, 444, 72);
    runtimeValue = lv_label_create(runtimeCard);
    lv_obj_set_style_text_color(runtimeValue, lv_color_hex(kWhite), 0);
    lv_obj_center(runtimeValue);

    updateDeviceInfo(nullptr);
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
    Serial.println("WT32-SC01-PLUS 15_LVGL_DeviceInfo");

    if (!board.begin()) {
        Serial.println("ERROR: BSP display/backlight initialization failed");
        while (true) delay(1000);
    }

    if (!board.touch().begin()) {
        Serial.println("ERROR: touch initialization failed");
        while (true) delay(1000);
    }

    board.backlight().set(80);

    Serial.printf("DEVICE: %s rev %u, %u cores, %u MHz\n",
                  ESP.getChipModel(),
                  static_cast<unsigned>(ESP.getChipRevision()),
                  static_cast<unsigned>(ESP.getChipCores()),
                  static_cast<unsigned>(ESP.getCpuFreqMHz()));
    Serial.printf("MEMORY: Flash=%lu MiB PSRAM=%lu MiB Heap=%lu KiB\n",
                  static_cast<unsigned long>(ESP.getFlashChipSize() / kMiB),
                  static_cast<unsigned long>(ESP.getPsramSize() / kMiB),
                  static_cast<unsigned long>(ESP.getHeapSize() / kKiB));

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
    Serial.println("READY: LVGL live device info initialized");
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
