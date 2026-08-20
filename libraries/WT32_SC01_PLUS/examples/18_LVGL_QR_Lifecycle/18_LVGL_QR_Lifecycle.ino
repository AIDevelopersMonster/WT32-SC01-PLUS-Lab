/*
 * WT32-SC01-PLUS-Lab
 * Example 18: LVGL QR Lifecycle
 *
 * Target hardware:
 *   Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208
 *
 * Purpose:
 *   Demonstrate the official Espressif QR-driven provisioning protocol:
 *   - before Wi-Fi configuration: SoftAP provisioning QR for the Espressif app;
 *   - after Wi-Fi connection: the same QR area becomes an information URL.
 *
 * Dependencies:
 *   LVGL 8.3.11 with LV_USE_QRCODE=1
 *   Arduino-ESP32 WiFiProv
 *
 * Important:
 *   Panlee V15 uses QSPI PSRAM. Select Tools -> PSRAM -> QSPI PSRAM.
 *   BLE provisioning is intentionally not used here because Arduino-ESP32
 *   3.3.7/3.3.8 has a reproducible ESP32-S3 BLE-controller startup regression.
 *
 * Status: SOURCE / CI TARGET. Physical validation required before PASS.
 */

#include <WT32_SC01_PLUS.h>
#include <lvgl.h>
#include <WiFi.h>
#include <WiFiProv.h>
#include <Preferences.h>
#include <esp_system.h>
#include "sdkconfig.h"

#ifndef BOARD_HAS_PSRAM
#error "Panlee WT32-SC01-PLUS requires Tools -> PSRAM -> QSPI PSRAM for example 18"
#endif

#if !defined(CONFIG_SPIRAM_MODE_QUAD)
#error "Wrong PSRAM mode: select Tools -> PSRAM -> QSPI PSRAM, not OPI PSRAM"
#endif

WT32_SC01_PLUS board;
Preferences preferences;

namespace {
constexpr uint16_t kBufferLines = 20;
constexpr uint8_t kPageCount = 4;
constexpr uint32_t kKiB = 1024UL;
constexpr uint32_t kMiB = 1024UL * 1024UL;
constexpr uint8_t kDefaultBrightness = 80;
constexpr uint8_t kMinBrightness = 10;
constexpr uint8_t kMaxBrightness = 100;
constexpr uint32_t kBrightnessSaveDelayMs = 900;
constexpr const char *kPrefsNamespace = "wt32ui";
constexpr const char *kThemeKey = "theme";
constexpr const char *kBrightnessKey = "bright";
constexpr const char *kLandingUrl = "https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab";
constexpr size_t kServiceNameSize = 24;
constexpr size_t kPopSize = 12;
constexpr size_t kQrPayloadSize = 192;

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

enum class NetworkUiState : uint8_t {
    Starting = 0,
    Provisioning,
    Connecting,
    Connected,
    Failed,
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

static lv_obj_t *contentArea = nullptr;
static lv_obj_t *navBar = nullptr;
static lv_obj_t *navButtons[kPageCount] = {nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *qrObject = nullptr;
static lv_obj_t *qrTitle = nullptr;
static lv_obj_t *qrStatus = nullptr;
static lv_obj_t *qrDetail = nullptr;
static lv_obj_t *brightnessValue = nullptr;
static lv_obj_t *saveStateValue = nullptr;
static lv_obj_t *chipValue = nullptr;
static lv_obj_t *runtimeValue = nullptr;

static PageId currentPage = PageId::Home;
static ThemeMode currentTheme = ThemeMode::Dark;
static uint8_t currentBrightness = kDefaultBrightness;
static bool brightnessSavePending = false;
static uint32_t brightnessChangedAtMs = 0;

static volatile NetworkUiState pendingNetworkState = NetworkUiState::Starting;
static volatile bool networkStateDirty = true;
static volatile bool hadStationIp = false;
static NetworkUiState currentNetworkState = NetworkUiState::Starting;

static char serviceName[kServiceNameSize] = {0};
static char proofOfPossession[kPopSize] = {0};
static char provisioningPayload[kQrPayloadSize] = {0};
static char connectedIp[24] = "-";

const ThemePalette &theme() {
    return currentTheme == ThemeMode::Light ? kLightTheme : kDarkTheme;
}

const char *themeName() {
    return currentTheme == ThemeMode::Light ? "LIGHT" : "DARK";
}

const char *networkStateName(NetworkUiState state) {
    switch (state) {
        case NetworkUiState::Provisioning: return "PROVISIONING";
        case NetworkUiState::Connecting:   return "CONNECTING";
        case NetworkUiState::Connected:    return "CONNECTED";
        case NetworkUiState::Failed:       return "FAILED";
        default:                           return "STARTING";
    }
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

void loadPersistentSettings() {
    const uint8_t storedTheme = preferences.getUChar(kThemeKey, 0);
    currentTheme = storedTheme == static_cast<uint8_t>(ThemeMode::Light)
                       ? ThemeMode::Light
                       : ThemeMode::Dark;

    const uint8_t storedBrightness = preferences.getUChar(kBrightnessKey, kDefaultBrightness);
    currentBrightness = constrain(storedBrightness, kMinBrightness, kMaxBrightness);
}

void saveTheme() {
    preferences.putUChar(kThemeKey, static_cast<uint8_t>(currentTheme));
    Serial.printf("PREF SAVE: theme=%s\n", themeName());
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

void makeProvisioningIdentity() {
    const uint64_t mac = ESP.getEfuseMac();
    const uint32_t suffix = static_cast<uint32_t>(mac & 0xFFFFFFULL);
    const uint32_t popNumber = static_cast<uint32_t>((mac ^ (mac >> 24)) % 100000000ULL);

    snprintf(serviceName, sizeof(serviceName), "PROV_%06lX", static_cast<unsigned long>(suffix));
    snprintf(proofOfPossession, sizeof(proofOfPossession), "%08lu", static_cast<unsigned long>(popNumber));
    snprintf(provisioningPayload, sizeof(provisioningPayload),
             "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"softap\"}",
             serviceName, proofOfPossession);

    Serial.printf("PROV SERVICE: %s\n", serviceName);
    Serial.printf("PROV QR PAYLOAD: %s\n", provisioningPayload);
}

void queueNetworkState(NetworkUiState state) {
    pendingNetworkState = state;
    networkStateDirty = true;
}

void provisioningEvent(arduino_event_t *event) {
    // Runs from another FreeRTOS task. Do not call LVGL from here.
    switch (event->event_id) {
        case ARDUINO_EVENT_PROV_START:
            Serial.println("PROV EVENT: START (SOFTAP)");
            queueNetworkState(NetworkUiState::Provisioning);
            break;

        case ARDUINO_EVENT_PROV_CRED_RECV:
            Serial.println("PROV EVENT: CREDENTIALS RECEIVED");
            queueNetworkState(NetworkUiState::Connecting);
            break;

        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            Serial.println("PROV EVENT: CREDENTIALS SUCCESS");
            queueNetworkState(NetworkUiState::Connecting);
            break;

        case ARDUINO_EVENT_PROV_CRED_FAIL:
            Serial.println("PROV EVENT: CREDENTIALS FAILED");
            queueNetworkState(NetworkUiState::Failed);
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
            IPAddress ip(event->event_info.got_ip.ip_info.ip.addr);
            snprintf(connectedIp, sizeof(connectedIp), "%u.%u.%u.%u",
                     ip[0], ip[1], ip[2], ip[3]);
            hadStationIp = true;
            Serial.printf("WIFI: CONNECTED %s\n", connectedIp);
            queueNetworkState(NetworkUiState::Connected);
            break;
        }

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if (hadStationIp) {
                Serial.println("WIFI: DISCONNECTED - reconnecting");
                queueNetworkState(NetworkUiState::Connecting);
            }
            break;

        default:
            break;
    }
}

void clearContent() {
    qrObject = nullptr;
    qrTitle = nullptr;
    qrStatus = nullptr;
    qrDetail = nullptr;
    brightnessValue = nullptr;
    saveStateValue = nullptr;
    chipValue = nullptr;
    runtimeValue = nullptr;
    lv_obj_clean(contentArea);
}

void updateQrArea() {
    if (currentPage != PageId::Home || !qrTitle || !qrStatus || !qrDetail) return;

    const char *payload = nullptr;
    uint32_t statusColor = theme().muted;

    switch (currentNetworkState) {
        case NetworkUiState::Provisioning:
            lv_label_set_text(qrTitle, "ESPRESSIF QR PROVISIONING");
            lv_label_set_text_fmt(qrStatus, "Scan in ESP Provisioning | %s", serviceName);
            lv_label_set_text(qrDetail, "SoftAP transport / Security 1 / PoP in QR");
            payload = provisioningPayload;
            statusColor = theme().warning;
            break;

        case NetworkUiState::Connecting:
            lv_label_set_text(qrTitle, "CONNECTING TO WI-FI");
            lv_label_set_text(qrStatus, "Credentials received - waiting for station IP");
            lv_label_set_text(qrDetail, "Provisioning channel may switch while STA connects");
            statusColor = theme().warning;
            break;

        case NetworkUiState::Connected:
            lv_label_set_text(qrTitle, "DEVICE ONLINE");
            lv_label_set_text_fmt(qrStatus, "Wi-Fi connected | IP %s", connectedIp);
            lv_label_set_text(qrDetail, "QR lifecycle: provisioning -> project information");
            payload = kLandingUrl;
            statusColor = theme().good;
            break;

        case NetworkUiState::Failed:
            lv_label_set_text(qrTitle, "PROVISIONING FAILED");
            lv_label_set_text(qrStatus, "Check Wi-Fi credentials and retry");
            lv_label_set_text(qrDetail, "Use SETTINGS -> RESET WIFI + REBOOT for clean retry");
            payload = provisioningPayload;
            statusColor = theme().warning;
            break;

        default:
            lv_label_set_text(qrTitle, "NETWORK STARTING");
            lv_label_set_text(qrStatus, "Checking saved credentials / provisioning state");
            lv_label_set_text(qrDetail, "Provisioning QR appears only after PROV_START");
            break;
    }

    lv_obj_set_style_text_color(qrStatus, lv_color_hex(statusColor), 0);

    if (qrObject) {
        if (payload) {
            lv_qrcode_update(qrObject, payload, strlen(payload));
            lv_obj_clear_flag(qrObject, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(qrObject, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void serviceNetworkState() {
    if (!networkStateDirty) return;
    currentNetworkState = pendingNetworkState;
    networkStateDirty = false;
    Serial.printf("UI NETWORK STATE: %s\n", networkStateName(currentNetworkState));
    updateQrArea();
}

void buildHomePage() {
    qrTitle = makeLabel(contentArea, "NETWORK STARTING", 18, 12, theme().accent);
    qrStatus = makeMutedLabel(contentArea, "Checking provisioning state", 18, 40);

    qrObject = lv_qrcode_create(contentArea, 166,
                                lv_color_hex(theme().text),
                                lv_color_hex(theme().panel));
    lv_obj_set_pos(qrObject, 18, 70);

    lv_obj_t *card = makeCard(contentArea, 202, 70, 260, 166);
    lv_obj_t *headline = lv_label_create(card);
    lv_label_set_text(headline, "QR LIFECYCLE");
    lv_obj_set_style_text_color(headline, lv_color_hex(theme().accent), 0);
    lv_obj_set_pos(headline, 6, 4);

    qrDetail = lv_label_create(card);
    lv_label_set_long_mode(qrDetail, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(qrDetail, 230);
    lv_obj_set_style_text_color(qrDetail, lv_color_hex(theme().text), 0);
    lv_obj_set_pos(qrDetail, 6, 38);

    lv_obj_t *hint = lv_label_create(card);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hint, 230);
    lv_label_set_text(hint,
                      "Before Wi-Fi: scan QR in the Espressif provisioning app.\n\n"
                      "After Wi-Fi: same QR becomes a project/info link.");
    lv_obj_set_style_text_color(hint, lv_color_hex(theme().muted), 0);
    lv_obj_set_pos(hint, 6, 82);

    updateQrArea();
}

void onRemoteButton(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    const intptr_t index = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    Serial.printf("REMOTE action placeholder: %d\n", static_cast<int>(index));
}

void buildRemotePage() {
    makeLabel(contentArea, "REMOTE", 20, 18, theme().accent);
    makeMutedLabel(contentArea, "QR lifecycle compatibility page", 20, 50);

    for (int i = 0; i < 6; ++i) {
        lv_obj_t *button = lv_btn_create(contentArea);
        lv_obj_set_size(button, 110, 56);
        lv_obj_set_pos(button, 28 + (i % 3) * 140, 92 + (i / 3) * 76);
        lv_obj_set_style_bg_color(button, lv_color_hex(theme().accent), LV_PART_MAIN);
        lv_obj_set_style_radius(button, 10, 0);
        lv_obj_add_event_cb(button, onRemoteButton, LV_EVENT_CLICKED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(i + 1)));
        lv_obj_t *label = lv_label_create(button);
        lv_label_set_text_fmt(label, "CMD %d", i + 1);
        lv_obj_center(label);
    }
}

void showPage(PageId page);

void applyThemeAsync(void *) {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(theme().background), 0);
    lv_obj_set_style_bg_color(contentArea, lv_color_hex(theme().background), 0);
    lv_obj_set_style_bg_color(navBar, lv_color_hex(theme().nav), 0);
    showPage(currentPage);
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

void onResetWiFi(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    Serial.println("WIFI RESET: erasing saved Wi-Fi credentials and rebooting");
    if (saveStateValue) lv_label_set_text(saveStateValue, "Wi-Fi erased - rebooting...");
    WiFi.disconnect(true, true);
    delay(600);
    ESP.restart();
}

void buildSettingsPage() {
    makeLabel(contentArea, "SETTINGS", 20, 14, theme().accent);

    makeTextLabel(contentArea, "Appearance", 20, 52);
    makeMutedLabel(contentArea, "Dark", 150, 52);
    lv_obj_t *themeSwitch = lv_switch_create(contentArea);
    lv_obj_set_pos(themeSwitch, 205, 45);
    if (currentTheme == ThemeMode::Light) lv_obj_add_state(themeSwitch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(themeSwitch, onThemeSwitch, LV_EVENT_VALUE_CHANGED, nullptr);
    makeMutedLabel(contentArea, "Light", 270, 52);

    makeTextLabel(contentArea, "Backlight", 20, 101);
    lv_obj_t *slider = lv_slider_create(contentArea);
    lv_obj_set_width(slider, 245);
    lv_slider_set_range(slider, kMinBrightness, kMaxBrightness);
    lv_slider_set_value(slider, currentBrightness, LV_ANIM_OFF);
    lv_obj_set_pos(slider, 130, 104);
    lv_obj_add_event_cb(slider, onBrightness, LV_EVENT_VALUE_CHANGED, nullptr);
    brightnessValue = makeLabel(contentArea, "", 390, 96, theme().accent);
    lv_label_set_text_fmt(brightnessValue, "%u%%", static_cast<unsigned>(currentBrightness));

    lv_obj_t *resetButton = lv_btn_create(contentArea);
    lv_obj_set_pos(resetButton, 20, 150);
    lv_obj_set_size(resetButton, 210, 54);
    lv_obj_set_style_bg_color(resetButton, lv_color_hex(theme().warning), LV_PART_MAIN);
    lv_obj_add_event_cb(resetButton, onResetWiFi, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *resetLabel = lv_label_create(resetButton);
    lv_label_set_text(resetLabel, "RESET WIFI + REBOOT");
    lv_obj_center(resetLabel);

    saveStateValue = makeMutedLabel(contentArea,
                                    "Reset Wi-Fi to repeat QR onboarding.",
                                    245, 164);
    lv_obj_set_width(saveStateValue, 215);
}

void updateDeviceInfo(lv_timer_t *) {
    if (!chipValue || !runtimeValue) return;

    lv_label_set_text_fmt(
        chipValue,
        "%s rev %u | Flash %lu MiB | PSRAM %lu MiB\nArduino %s | IDF %s",
        ESP.getChipModel(),
        static_cast<unsigned>(ESP.getChipRevision()),
        static_cast<unsigned long>(ESP.getFlashChipSize() / kMiB),
        static_cast<unsigned long>(ESP.getPsramSize() / kMiB),
        ESP.getCoreVersion(),
        ESP.getSdkVersion());

    lv_label_set_text_fmt(
        runtimeValue,
        "Network %s | IP %s\nTheme %s | brightness %u%%\nHeap %lu KiB | uptime %lu s",
        networkStateName(currentNetworkState),
        connectedIp,
        themeName(),
        static_cast<unsigned>(currentBrightness),
        static_cast<unsigned long>(ESP.getFreeHeap() / kKiB),
        static_cast<unsigned long>(millis() / 1000UL));
}

void buildInfoPage() {
    makeLabel(contentArea, "DEVICE INFO", 20, 12, theme().accent);
    makeMutedLabel(contentArea, "Runtime + provisioning state", 170, 14);

    lv_obj_t *chipCard = makeCard(contentArea, 18, 54, 444, 76);
    chipValue = lv_label_create(chipCard);
    lv_obj_set_style_text_color(chipValue, lv_color_hex(theme().text), 0);
    lv_obj_center(chipValue);

    lv_obj_t *runtimeCard = makeCard(contentArea, 18, 142, 444, 92);
    runtimeValue = lv_label_create(runtimeCard);
    lv_obj_set_style_text_color(runtimeValue, lv_color_hex(theme().good), 0);
    lv_obj_center(runtimeValue);

    updateDeviceInfo(nullptr);
}

void updateNavStyle() {
    for (uint8_t i = 0; i < kPageCount; ++i) {
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

void startProvisioningLifecycle() {
    WiFi.onEvent(provisioningEvent);
    WiFi.begin();

    Serial.println("PROV: starting Espressif SoftAP provisioning manager");
    WiFiProv.beginProvision(
        NETWORK_PROV_SCHEME_SOFTAP,
        NETWORK_PROV_SCHEME_HANDLER_NONE,
        NETWORK_PROV_SECURITY_1,
        proofOfPossession,
        serviceName,
        nullptr,
        nullptr,
        false);
    WiFiProv.printQR(serviceName, proofOfPossession, "softap");
}
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("WT32-SC01-PLUS 18_LVGL_QR_Lifecycle");
    Serial.println("BUILD: Espressif SoftAP QR provisioning / QSPI PSRAM");

    if (!preferences.begin(kPrefsNamespace, false)) {
        Serial.println("ERROR: Preferences/NVS open failed");
        while (true) delay(1000);
    }
    loadPersistentSettings();
    makeProvisioningIdentity();

    if (!board.begin()) {
        Serial.println("ERROR: BSP display/backlight initialization failed");
        while (true) delay(1000);
    }
    if (!board.touch().begin()) {
        Serial.println("ERROR: touch initialization failed");
        while (true) delay(1000);
    }
    board.backlight().set(currentBrightness);

    Serial.printf("PSRAM: %lu MiB, free %lu KiB\n",
                  static_cast<unsigned long>(ESP.getPsramSize() / kMiB),
                  static_cast<unsigned long>(ESP.getFreePsram() / kKiB));

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

    startProvisioningLifecycle();
    Serial.println("READY: Espressif SoftAP QR lifecycle initialized");
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
    serviceNetworkState();
    delay(5);
}
