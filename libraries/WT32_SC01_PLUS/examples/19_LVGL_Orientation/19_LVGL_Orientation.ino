/*
 * WT32-SC01-PLUS-Lab
 * Example 19: LVGL Orientation
 *
 * Target hardware:
 *   Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208
 *
 * Purpose:
 *   Exercise LVGL 8 software display rotation and its built-in pointer
 *   coordinate transform at 0/90/180/270 degrees while keeping the
 *   physically validated WT32_SC01_PLUS landscape BSP as the native layer.
 *
 * Dependency: LVGL 8.3.11 with LV_USE_QRCODE=1
 * Status: SOURCE / CI TARGET. Physical validation required before PASS.
 */

#include <WT32_SC01_PLUS.h>
#include <lvgl.h>
#include <Preferences.h>

WT32_SC01_PLUS board;
Preferences preferences;

namespace {
constexpr uint16_t kBufferLines = 20;
constexpr uint8_t kPageCount = 4;
constexpr uint8_t kTargetCount = 5;
constexpr uint8_t kDefaultBrightness = 80;
constexpr uint8_t kMinBrightness = 10;
constexpr uint8_t kMaxBrightness = 100;
constexpr uint32_t kBrightnessSaveDelayMs = 900;
constexpr uint32_t kKiB = 1024UL;
constexpr uint32_t kMiB = 1024UL * 1024UL;
constexpr const char *kPrefsNamespace = "wt32ui";
constexpr const char *kThemeKey = "theme";
constexpr const char *kBrightnessKey = "bright";
constexpr const char *kOrientationKey = "orient";
constexpr const char *kProjectUrl = "https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab";
constexpr lv_coord_t kHeaderHeight = 36;
constexpr lv_coord_t kNavHeight = 54;

enum class PageId : uint8_t {
    Test = 0,
    Qr,
    Settings,
    Info,
};

enum class ThemeMode : uint8_t {
    Dark = 0,
    Light,
};

enum class OrientationMode : uint8_t {
    Deg0 = 0,
    Deg90,
    Deg180,
    Deg270,
};

enum class TargetId : uint8_t {
    TopLeft = 0,
    TopRight,
    Center,
    BottomLeft,
    BottomRight,
};

struct ThemePalette {
    uint32_t background;
    uint32_t panel;
    uint32_t nav;
    uint32_t accent;
    uint32_t muted;
    uint32_t text;
    uint32_t good;
    uint32_t warning;
};

constexpr ThemePalette kDarkTheme = {
    0x101820, 0x18242F, 0x0B1118, 0x26A69A,
    0x90A4AE, 0xFFFFFF, 0x80CBC4, 0xFFCC80,
};

constexpr ThemePalette kLightTheme = {
    0xF3F6F8, 0xFFFFFF, 0xDCE5EA, 0x00796B,
    0x546E7A, 0x102027, 0x00695C, 0xEF6C00,
};

static lv_color_t drawBufferPixels[wt32sc01plus::pins::LCD_WIDTH * kBufferLines];
static lv_disp_draw_buf_t drawBuffer;
static lv_disp_drv_t displayDriver;
static lv_indev_drv_t touchDriver;
static lv_disp_t *displayHandle = nullptr;

static lv_obj_t *contentArea = nullptr;
static lv_obj_t *navBar = nullptr;
static lv_obj_t *navButtons[kPageCount] = {nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *targetButtons[kTargetCount] = {nullptr, nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *targetStatus = nullptr;
static lv_obj_t *brightnessValue = nullptr;
static lv_obj_t *saveStateValue = nullptr;
static lv_obj_t *infoValue = nullptr;

static PageId currentPage = PageId::Test;
static ThemeMode currentTheme = ThemeMode::Dark;
static OrientationMode currentOrientation = OrientationMode::Deg0;
static OrientationMode pendingOrientation = OrientationMode::Deg0;
static uint8_t currentBrightness = kDefaultBrightness;
static bool brightnessSavePending = false;
static uint32_t brightnessChangedAtMs = 0;
static uint8_t targetMasks[4] = {0, 0, 0, 0};

const ThemePalette &theme() {
    return currentTheme == ThemeMode::Light ? kLightTheme : kDarkTheme;
}

const char *themeName() {
    return currentTheme == ThemeMode::Light ? "LIGHT" : "DARK";
}

const char *orientationName(OrientationMode orientation) {
    switch (orientation) {
        case OrientationMode::Deg90:  return "90";
        case OrientationMode::Deg180: return "180";
        case OrientationMode::Deg270: return "270";
        default:                      return "0";
    }
}

lv_disp_rot_t lvRotation(OrientationMode orientation) {
    switch (orientation) {
        case OrientationMode::Deg90:  return LV_DISP_ROT_90;
        case OrientationMode::Deg180: return LV_DISP_ROT_180;
        case OrientationMode::Deg270: return LV_DISP_ROT_270;
        default:                      return LV_DISP_ROT_NONE;
    }
}

lv_coord_t logicalWidth() {
    return displayHandle ? lv_disp_get_hor_res(displayHandle)
                         : wt32sc01plus::pins::LCD_WIDTH;
}

lv_coord_t logicalHeight() {
    return displayHandle ? lv_disp_get_ver_res(displayHandle)
                         : wt32sc01plus::pins::LCD_HEIGHT;
}

uint8_t orientationIndex() {
    return static_cast<uint8_t>(currentOrientation);
}

uint8_t countBits(uint8_t value) {
    uint8_t count = 0;
    while (value) {
        count += value & 1U;
        value >>= 1U;
    }
    return count;
}

void flushDisplay(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *colorPixels) {
    const int width = area->x2 - area->x1 + 1;
    const int height = area->y2 - area->y1 + 1;

    // With sw_rotate enabled LVGL rotates the draw buffer and area back into
    // the native 480x320 physical coordinate system before this callback.
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

    // The BSP supplies the physically validated native landscape coordinate.
    // LVGL 8 transforms pointer coordinates according to display->rotated.
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

lv_obj_t *makeMutedLabel(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y) {
    return makeLabel(parent, text, x, y, theme().muted);
}

lv_obj_t *makeButton(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y,
                     lv_coord_t width, lv_coord_t height, uint32_t color) {
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_bg_color(button, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_radius(button, 8, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return button;
}

void loadPersistentSettings() {
    const uint8_t storedTheme = preferences.getUChar(kThemeKey, 0);
    currentTheme = storedTheme == static_cast<uint8_t>(ThemeMode::Light)
                       ? ThemeMode::Light
                       : ThemeMode::Dark;

    const uint8_t storedBrightness = preferences.getUChar(kBrightnessKey, kDefaultBrightness);
    currentBrightness = constrain(storedBrightness, kMinBrightness, kMaxBrightness);

    const uint8_t storedOrientation = preferences.getUChar(kOrientationKey, 0);
    currentOrientation = storedOrientation <= static_cast<uint8_t>(OrientationMode::Deg270)
                             ? static_cast<OrientationMode>(storedOrientation)
                             : OrientationMode::Deg0;
    pendingOrientation = currentOrientation;

    Serial.printf("PREF LOAD: theme=%s brightness=%u orientation=%s\n",
                  themeName(), static_cast<unsigned>(currentBrightness),
                  orientationName(currentOrientation));
}

void saveTheme() {
    preferences.putUChar(kThemeKey, static_cast<uint8_t>(currentTheme));
    Serial.printf("PREF SAVE: theme=%s\n", themeName());
}

void saveOrientation() {
    preferences.putUChar(kOrientationKey, static_cast<uint8_t>(currentOrientation));
    Serial.printf("PREF SAVE: orientation=%s\n", orientationName(currentOrientation));
}

void scheduleBrightnessSave() {
    brightnessSavePending = true;
    brightnessChangedAtMs = millis();
    if (saveStateValue) lv_label_set_text(saveStateValue, "Brightness pending NVS save");
}

void serviceBrightnessSave() {
    if (!brightnessSavePending) return;
    if (millis() - brightnessChangedAtMs < kBrightnessSaveDelayMs) return;

    preferences.putUChar(kBrightnessKey, currentBrightness);
    brightnessSavePending = false;
    if (saveStateValue) lv_label_set_text(saveStateValue, "Settings saved to NVS");
    Serial.printf("PREF SAVE: brightness=%u\n", static_cast<unsigned>(currentBrightness));
}

void clearContentPointers() {
    for (uint8_t i = 0; i < kTargetCount; ++i) targetButtons[i] = nullptr;
    targetStatus = nullptr;
    brightnessValue = nullptr;
    saveStateValue = nullptr;
    infoValue = nullptr;
}

void updateTargetUi() {
    if (!targetStatus) return;

    const uint8_t mask = targetMasks[orientationIndex()];
    const uint8_t passed = countBits(mask);
    lv_label_set_text_fmt(targetStatus, "%s deg touch: %u/%u targets",
                          orientationName(currentOrientation),
                          static_cast<unsigned>(passed),
                          static_cast<unsigned>(kTargetCount));
    lv_obj_set_style_text_color(
        targetStatus,
        lv_color_hex(passed == kTargetCount ? theme().good : theme().warning), 0);

    for (uint8_t i = 0; i < kTargetCount; ++i) {
        if (!targetButtons[i]) continue;
        const bool hit = (mask & (1U << i)) != 0;
        lv_obj_set_style_bg_color(
            targetButtons[i], lv_color_hex(hit ? theme().good : theme().accent), LV_PART_MAIN);
    }
}

void onTarget(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    const uint8_t target = static_cast<uint8_t>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    if (target >= kTargetCount) return;

    targetMasks[orientationIndex()] |= static_cast<uint8_t>(1U << target);
    Serial.printf("TOUCH TARGET: orientation=%s target=%u mask=0x%02X\n",
                  orientationName(currentOrientation), static_cast<unsigned>(target),
                  static_cast<unsigned>(targetMasks[orientationIndex()]));
    updateTargetUi();
}

void buildTestPage() {
    const lv_coord_t width = lv_obj_get_width(contentArea);
    const lv_coord_t height = lv_obj_get_height(contentArea);
    const lv_coord_t buttonW = width < 380 ? 58 : 72;
    const lv_coord_t buttonH = 38;
    const lv_coord_t margin = 8;

    targetButtons[static_cast<uint8_t>(TargetId::TopLeft)] =
        makeButton(contentArea, "TL", margin, margin, buttonW, buttonH, theme().accent);
    targetButtons[static_cast<uint8_t>(TargetId::TopRight)] =
        makeButton(contentArea, "TR", width - buttonW - margin, margin,
                   buttonW, buttonH, theme().accent);
    targetButtons[static_cast<uint8_t>(TargetId::Center)] =
        makeButton(contentArea, "CENTER", (width - buttonW) / 2,
                   (height - buttonH) / 2, buttonW, buttonH, theme().accent);
    targetButtons[static_cast<uint8_t>(TargetId::BottomLeft)] =
        makeButton(contentArea, "BL", margin, height - buttonH - margin,
                   buttonW, buttonH, theme().accent);
    targetButtons[static_cast<uint8_t>(TargetId::BottomRight)] =
        makeButton(contentArea, "BR", width - buttonW - margin,
                   height - buttonH - margin, buttonW, buttonH, theme().accent);

    for (uint8_t i = 0; i < kTargetCount; ++i) {
        lv_obj_add_event_cb(targetButtons[i], onTarget, LV_EVENT_CLICKED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(i)));
    }

    targetStatus = makeLabel(contentArea, "", 0, 0, theme().warning);
    lv_obj_set_width(targetStatus, width - 2 * (buttonW + margin + 4));
    lv_obj_set_style_text_align(targetStatus, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(targetStatus, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *hint = makeMutedLabel(contentArea,
                                    "Touch all five targets in every orientation.", 0, 0);
    lv_obj_set_width(hint, width - 24);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -52);

    updateTargetUi();
}

void buildQrPage() {
    const lv_coord_t width = lv_obj_get_width(contentArea);
    const lv_coord_t height = lv_obj_get_height(contentArea);
    lv_coord_t qrSize = width - 40;
    if (height - 70 < qrSize) qrSize = height - 70;
    if (qrSize > 200) qrSize = 200;
    if (qrSize < 100) qrSize = 100;

    lv_obj_t *qr = lv_qrcode_create(contentArea, qrSize,
                                    lv_color_hex(theme().text),
                                    lv_color_hex(theme().panel));
    lv_qrcode_update(qr, kProjectUrl, strlen(kProjectUrl));
    lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *label = makeLabel(contentArea, "Project QR - verify after each rotation",
                                0, 0, theme().accent);
    lv_obj_set_width(label, width - 20);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(label, qr, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
}

void showPage(PageId page);
void buildShell();

void applyThemeAsync(void *) {
    buildShell();
}

void onThemeSwitch(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) return;
    const ThemeMode requested = lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED)
                                    ? ThemeMode::Light
                                    : ThemeMode::Dark;
    if (requested == currentTheme) return;

    currentTheme = requested;
    saveTheme();
    lv_async_call(applyThemeAsync, nullptr);
}

void onBrightness(lv_event_t *event) {
    currentBrightness = static_cast<uint8_t>(lv_slider_get_value(lv_event_get_target(event)));
    board.backlight().set(currentBrightness);
    if (brightnessValue) {
        lv_label_set_text_fmt(brightnessValue, "%u%%", static_cast<unsigned>(currentBrightness));
    }
    scheduleBrightnessSave();
}

void applyOrientationAsync(void *) {
    currentOrientation = pendingOrientation;
    lv_disp_set_rotation(displayHandle, lvRotation(currentOrientation));
    saveOrientation();
    Serial.printf("ORIENTATION: %s deg logical=%dx%d\n",
                  orientationName(currentOrientation),
                  static_cast<int>(logicalWidth()), static_cast<int>(logicalHeight()));
    buildShell();
}

void onOrientation(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    const uint8_t requested = static_cast<uint8_t>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    if (requested > static_cast<uint8_t>(OrientationMode::Deg270)) return;

    pendingOrientation = static_cast<OrientationMode>(requested);
    if (pendingOrientation == currentOrientation) return;

    // Defer because buildShell() deletes the button that generated this event.
    lv_async_call(applyOrientationAsync, nullptr);
}

void onClearTargets(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    for (uint8_t &mask : targetMasks) mask = 0;
    Serial.println("TOUCH TARGETS: all orientation masks cleared");
    if (currentPage == PageId::Test) updateTargetUi();
}

void buildSettingsPage() {
    const lv_coord_t width = lv_obj_get_width(contentArea);
    const bool portrait = width < 400;
    const lv_coord_t buttonW = portrait ? 104 : 84;
    const lv_coord_t buttonH = 38;
    const lv_coord_t gap = 10;
    const lv_coord_t startX = portrait ? (width - (buttonW * 2 + gap)) / 2
                                       : (width - (buttonW * 4 + gap * 3)) / 2;
    const lv_coord_t startY = 12;

    const char *labels[4] = {"0 deg", "90 deg", "180 deg", "270 deg"};
    for (uint8_t i = 0; i < 4; ++i) {
        const uint8_t col = portrait ? i % 2 : i;
        const uint8_t row = portrait ? i / 2 : 0;
        lv_obj_t *button = makeButton(
            contentArea, labels[i],
            startX + col * (buttonW + gap),
            startY + row * (buttonH + gap),
            buttonW, buttonH,
            i == orientationIndex() ? theme().good : theme().accent);
        lv_obj_add_event_cb(button, onOrientation, LV_EVENT_CLICKED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(i)));
    }

    const lv_coord_t controlsY = portrait ? 116 : 68;
    makeLabel(contentArea, "Theme", 18, controlsY + 4, theme().text);
    lv_obj_t *themeSwitch = lv_switch_create(contentArea);
    lv_obj_set_pos(themeSwitch, 102, controlsY);
    if (currentTheme == ThemeMode::Light) lv_obj_add_state(themeSwitch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(themeSwitch, onThemeSwitch, LV_EVENT_VALUE_CHANGED, nullptr);
    makeMutedLabel(contentArea, currentTheme == ThemeMode::Light ? "Light" : "Dark",
                   170, controlsY + 4);

    const lv_coord_t brightnessY = controlsY + 50;
    makeLabel(contentArea, "Backlight", 18, brightnessY + 2, theme().text);
    lv_obj_t *slider = lv_slider_create(contentArea);
    lv_obj_set_width(slider, width - 180);
    lv_slider_set_range(slider, kMinBrightness, kMaxBrightness);
    lv_slider_set_value(slider, currentBrightness, LV_ANIM_OFF);
    lv_obj_set_pos(slider, 105, brightnessY + 5);
    lv_obj_add_event_cb(slider, onBrightness, LV_EVENT_VALUE_CHANGED, nullptr);
    brightnessValue = makeLabel(contentArea, "", width - 58, brightnessY - 3, theme().accent);
    lv_label_set_text_fmt(brightnessValue, "%u%%", static_cast<unsigned>(currentBrightness));

    const lv_coord_t clearY = brightnessY + 44;
    lv_obj_t *clearButton = makeButton(contentArea, "RESET TOUCH COUNTERS", 18, clearY,
                                       width - 36, 38, theme().warning);
    lv_obj_add_event_cb(clearButton, onClearTargets, LV_EVENT_CLICKED, nullptr);

    saveStateValue = makeMutedLabel(contentArea,
                                    "Orientation, theme and brightness persist through NVS.",
                                    18, clearY + 46);
    lv_obj_set_width(saveStateValue, width - 36);
}

void updateInfo(lv_timer_t *) {
    if (!infoValue) return;

    lv_label_set_text_fmt(
        infoValue,
        "Orientation %s deg | logical %dx%d\n"
        "Native panel %dx%d | LVGL sw_rotate=1\n"
        "Flash %lu MiB | PSRAM %lu MiB\n"
        "Heap %lu KiB | uptime %lu s\n"
        "Touch targets: 0:%u/5 90:%u/5 180:%u/5 270:%u/5\n"
        "Theme %s | brightness %u%%",
        orientationName(currentOrientation),
        static_cast<int>(logicalWidth()), static_cast<int>(logicalHeight()),
        wt32sc01plus::pins::LCD_WIDTH, wt32sc01plus::pins::LCD_HEIGHT,
        static_cast<unsigned long>(ESP.getFlashChipSize() / kMiB),
        static_cast<unsigned long>(ESP.getPsramSize() / kMiB),
        static_cast<unsigned long>(ESP.getFreeHeap() / kKiB),
        static_cast<unsigned long>(millis() / 1000UL),
        static_cast<unsigned>(countBits(targetMasks[0])),
        static_cast<unsigned>(countBits(targetMasks[1])),
        static_cast<unsigned>(countBits(targetMasks[2])),
        static_cast<unsigned>(countBits(targetMasks[3])),
        themeName(), static_cast<unsigned>(currentBrightness));
}

void buildInfoPage() {
    const lv_coord_t width = lv_obj_get_width(contentArea);
    const lv_coord_t height = lv_obj_get_height(contentArea);

    infoValue = lv_label_create(contentArea);
    lv_label_set_long_mode(infoValue, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(infoValue, width - 28);
    lv_obj_set_style_text_color(infoValue, lv_color_hex(theme().text), 0);
    lv_obj_set_pos(infoValue, 14, 16);

    lv_obj_t *note = makeMutedLabel(contentArea,
                                    "Pointer coordinates are transformed by LVGL after the BSP read.",
                                    14, height - 42);
    lv_obj_set_width(note, width - 28);
    updateInfo(nullptr);
}

void updateNavStyle() {
    for (uint8_t i = 0; i < kPageCount; ++i) {
        if (!navButtons[i]) continue;
        const bool active = i == static_cast<uint8_t>(currentPage);
        lv_obj_set_style_bg_color(navButtons[i],
                                  lv_color_hex(active ? theme().accent : theme().panel),
                                  LV_PART_MAIN);
        lv_obj_set_style_text_color(navButtons[i],
                                    lv_color_hex(active ? 0xFFFFFF : theme().muted),
                                    LV_PART_MAIN);
    }
}

void showPage(PageId page) {
    currentPage = page;
    clearContentPointers();
    lv_obj_clean(contentArea);

    switch (page) {
        case PageId::Test:     buildTestPage(); break;
        case PageId::Qr:       buildQrPage(); break;
        case PageId::Settings: buildSettingsPage(); break;
        case PageId::Info:     buildInfoPage(); break;
    }

    updateNavStyle();
    Serial.printf("PAGE: %u orientation=%s logical=%dx%d\n",
                  static_cast<unsigned>(page), orientationName(currentOrientation),
                  static_cast<int>(logicalWidth()), static_cast<int>(logicalHeight()));
}

void onNav(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    const intptr_t page = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    showPage(static_cast<PageId>(page));
}

void buildShell() {
    const lv_coord_t width = logicalWidth();
    const lv_coord_t height = logicalHeight();
    lv_obj_t *screen = lv_scr_act();

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(theme().background), 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, width, kHeaderHeight);
    lv_obj_set_style_bg_color(header, lv_color_hex(theme().panel), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *headerLabel = lv_label_create(header);
    lv_label_set_text_fmt(headerLabel, "ORIENTATION %s deg | %dx%d",
                          orientationName(currentOrientation),
                          static_cast<int>(width), static_cast<int>(height));
    lv_obj_set_style_text_color(headerLabel, lv_color_hex(theme().accent), 0);
    lv_obj_center(headerLabel);

    contentArea = lv_obj_create(screen);
    lv_obj_set_pos(contentArea, 0, kHeaderHeight);
    lv_obj_set_size(contentArea, width, height - kHeaderHeight - kNavHeight);
    lv_obj_set_style_bg_color(contentArea, lv_color_hex(theme().background), 0);
    lv_obj_set_style_border_width(contentArea, 0, 0);
    lv_obj_set_style_radius(contentArea, 0, 0);
    lv_obj_set_style_pad_all(contentArea, 0, 0);
    lv_obj_clear_flag(contentArea, LV_OBJ_FLAG_SCROLLABLE);

    navBar = lv_obj_create(screen);
    lv_obj_set_pos(navBar, 0, height - kNavHeight);
    lv_obj_set_size(navBar, width, kNavHeight);
    lv_obj_set_style_bg_color(navBar, lv_color_hex(theme().nav), 0);
    lv_obj_set_style_border_width(navBar, 0, 0);
    lv_obj_set_style_radius(navBar, 0, 0);
    lv_obj_set_style_pad_all(navBar, 4, 0);
    lv_obj_set_flex_flow(navBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(navBar, LV_FLEX_ALIGN_SPACE_AROUND,
                         LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(navBar, LV_OBJ_FLAG_SCROLLABLE);

    const char *labels[kPageCount] = {"TEST", "QR", "SET", "INFO"};
    const lv_coord_t navButtonWidth = width / 4 - 8;
    for (uint8_t i = 0; i < kPageCount; ++i) {
        lv_obj_t *button = lv_btn_create(navBar);
        navButtons[i] = button;
        lv_obj_set_size(button, navButtonWidth, kNavHeight - 10);
        lv_obj_set_style_radius(button, 8, 0);
        lv_obj_add_event_cb(button, onNav, LV_EVENT_CLICKED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(i)));
        lv_obj_t *label = lv_label_create(button);
        lv_label_set_text(label, labels[i]);
        lv_obj_center(label);
    }

    showPage(currentPage);
}
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("WT32-SC01-PLUS 19_LVGL_Orientation");

    if (!preferences.begin(kPrefsNamespace, false)) {
        Serial.println("ERROR: Preferences/NVS open failed");
        while (true) delay(1000);
    }
    loadPersistentSettings();

    if (!board.begin()) {
        Serial.println("ERROR: BSP display/backlight initialization failed");
        while (true) delay(1000);
    }
    if (!board.touch().begin()) {
        Serial.println("ERROR: touch initialization failed");
        while (true) delay(1000);
    }
    board.backlight().set(currentBrightness);

    lv_init();
    static_assert(sizeof(lv_color_t) == sizeof(uint16_t), "LVGL must use 16-bit color");

    lv_disp_draw_buf_init(&drawBuffer, drawBufferPixels, nullptr,
                          wt32sc01plus::pins::LCD_WIDTH * kBufferLines);

    lv_disp_drv_init(&displayDriver);
    displayDriver.hor_res = wt32sc01plus::pins::LCD_WIDTH;
    displayDriver.ver_res = wt32sc01plus::pins::LCD_HEIGHT;
    displayDriver.flush_cb = flushDisplay;
    displayDriver.draw_buf = &drawBuffer;
    displayDriver.sw_rotate = 1;
    displayHandle = lv_disp_drv_register(&displayDriver);
    if (!displayHandle) {
        Serial.println("ERROR: LVGL display registration failed");
        while (true) delay(1000);
    }

    lv_indev_drv_init(&touchDriver);
    touchDriver.type = LV_INDEV_TYPE_POINTER;
    touchDriver.read_cb = readTouch;
    lv_indev_drv_register(&touchDriver);

    lv_disp_set_rotation(displayHandle, lvRotation(currentOrientation));
    buildShell();
    lv_timer_create(updateInfo, 1000, nullptr);

    Serial.printf("READY: orientation=%s logical=%dx%d native=%dx%d sw_rotate=1\n",
                  orientationName(currentOrientation),
                  static_cast<int>(logicalWidth()), static_cast<int>(logicalHeight()),
                  wt32sc01plus::pins::LCD_WIDTH, wt32sc01plus::pins::LCD_HEIGHT);
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
    serviceBrightnessSave();
    delay(5);
}
