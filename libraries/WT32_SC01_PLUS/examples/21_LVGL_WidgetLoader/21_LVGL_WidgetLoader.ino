/*
 * WT32-SC01-PLUS-Lab
 * Example 21: LVGL Widget Loader
 *
 * Declarative widgets are uploaded through a temporary password-protected
 * SoftAP web portal, validated, stored in LittleFS, and rendered by LVGL.
 * No arbitrary native code is loaded from widget files.
 *
 * Target specimen:
 *   Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208
 *   ESP32-S3, 16 MiB Flash, 2 MiB QSPI PSRAM
 *
 * Status: SOURCE / PHYSICAL VALIDATION REQUIRED.
 */

#include <WT32_SC01_PLUS.h>
#include <lvgl.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <cJSON.h>
#include <time.h>

WT32_SC01_PLUS board;
WebServer server(80);

namespace {
constexpr uint16_t kBufferLines = 20;
constexpr uint8_t kDefaultBrightness = 80;
constexpr size_t kMaxWidgetBytes = 32 * 1024;
constexpr uint8_t kMaxWidgetObjects = 24;
constexpr uint8_t kMaxBoundLabels = 8;
constexpr uint32_t kPortalTimeoutMs = 10UL * 60UL * 1000UL;
constexpr uint32_t kBindingRefreshMs = 1000;
constexpr int kWidgetTop = 36;
constexpr const char *kWidgetPath = "/widgets/widget.json";
constexpr const char *kTempWidgetPath = "/widgets/widget.tmp";

static lv_color_t drawBufferPixels[wt32sc01plus::pins::LCD_WIDTH * kBufferLines];
static lv_disp_draw_buf_t drawBuffer;
static lv_disp_drv_t displayDriver;
static lv_indev_drv_t touchDriver;

static lv_obj_t *managerScreen = nullptr;
static lv_obj_t *managerStatus = nullptr;
static lv_obj_t *managerInstalled = nullptr;
static lv_obj_t *managerNetwork = nullptr;
static lv_obj_t *runButton = nullptr;
static lv_obj_t *deleteButton = nullptr;
static lv_obj_t *widgetScreen = nullptr;

static String apSsid;
static String apPassword;
static bool portalRunning = false;
static uint32_t portalStartedMs = 0;
static File uploadFile;
static size_t uploadBytes = 0;
static bool uploadFailed = false;
static bool uploadSucceeded = false;
static String uploadMessage;
static bool runUploadedWidgetRequested = false;
static uint32_t runUploadedWidgetAtMs = 0;

struct BoundLabel {
    lv_obj_t *object = nullptr;
    String binding;
    String prefix;
    String suffix;
};

static BoundLabel boundLabels[kMaxBoundLabels];
static uint8_t boundLabelCount = 0;
static uint32_t lastBindingRefreshMs = 0;

void serviceLvgl();
void showManager();
bool loadWidgetFromFlash(String &reason);

void flushDisplay(lv_disp_drv_t *, const lv_area_t *area, lv_color_t *colorPixels) {
    board.display().drawRGB565(
        area->x1, area->y1,
        area->x2 - area->x1 + 1,
        area->y2 - area->y1 + 1,
        reinterpret_cast<const uint16_t *>(colorPixels));
    lv_disp_flush_ready(&displayDriver);
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

void serviceLvgl() {
    static uint32_t previous = millis();
    const uint32_t now = millis();
    const uint32_t elapsed = now - previous;
    if (elapsed) {
        lv_tick_inc(elapsed);
        previous = now;
    }
    lv_timer_handler();
}

lv_color_t parseColor(const char *text, uint32_t fallback) {
    if (!text || text[0] != '#' || strlen(text) != 7) return lv_color_hex(fallback);
    char *end = nullptr;
    const unsigned long value = strtoul(text + 1, &end, 16);
    if (!end || *end != '\0') return lv_color_hex(fallback);
    return lv_color_hex(static_cast<uint32_t>(value));
}

bool validColor(const cJSON *item) {
    if (!item) return true;
    if (!cJSON_IsString(item) || !item->valuestring) return false;
    const char *s = item->valuestring;
    if (strlen(s) != 7 || s[0] != '#') return false;
    for (size_t i = 1; i < 7; ++i) {
        if (!isxdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

bool validShortString(const cJSON *item, size_t maxLen) {
    return cJSON_IsString(item) && item->valuestring &&
           strlen(item->valuestring) > 0 && strlen(item->valuestring) <= maxLen;
}

bool validateWidgetJson(const String &json, String &reason) {
    if (json.length() == 0 || json.length() > kMaxWidgetBytes) {
        reason = "Widget must be 1..32768 bytes";
        return false;
    }

    cJSON *root = cJSON_ParseWithLength(json.c_str(), json.length());
    if (!root) {
        reason = "JSON parse failed";
        return false;
    }

    bool ok = true;
    const cJSON *schema = cJSON_GetObjectItemCaseSensitive(root, "schema");
    const cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    const cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    const cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    const cJSON *background = cJSON_GetObjectItemCaseSensitive(root, "background");
    const cJSON *objects = cJSON_GetObjectItemCaseSensitive(root, "objects");

    if (!cJSON_IsNumber(schema) || schema->valueint != 1) {
        reason = "schema must equal 1";
        ok = false;
    }
    if (ok && !validShortString(id, 48)) {
        reason = "id missing/too long";
        ok = false;
    }
    if (ok && !validShortString(name, 64)) {
        reason = "name missing/too long";
        ok = false;
    }
    if (ok && !validShortString(version, 24)) {
        reason = "version missing/too long";
        ok = false;
    }
    if (ok && !validColor(background)) {
        reason = "background must be #RRGGBB";
        ok = false;
    }
    if (ok && (!cJSON_IsArray(objects) || cJSON_GetArraySize(objects) < 1 ||
               cJSON_GetArraySize(objects) > kMaxWidgetObjects)) {
        reason = "objects must contain 1..24 items";
        ok = false;
    }

    if (ok) {
        const int count = cJSON_GetArraySize(objects);
        for (int i = 0; i < count && ok; ++i) {
            const cJSON *object = cJSON_GetArrayItem(objects, i);
            const cJSON *type = cJSON_GetObjectItemCaseSensitive(object, "type");
            if (!cJSON_IsObject(object) || !validShortString(type, 16)) {
                reason = "object type missing";
                ok = false;
                break;
            }

            const String kind(type->valuestring);
            const cJSON *color = cJSON_GetObjectItemCaseSensitive(object, "color");
            if (!validColor(color)) {
                reason = "object color must be #RRGGBB";
                ok = false;
                break;
            }

            if (kind == "label") {
                const cJSON *text = cJSON_GetObjectItemCaseSensitive(object, "text");
                const cJSON *bind = cJSON_GetObjectItemCaseSensitive(object, "bind");
                if (!validShortString(text, 160) && !validShortString(bind, 32)) {
                    reason = "label requires text or bind";
                    ok = false;
                }
            } else if (kind == "bar") {
                const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, "value");
                if (value && (!cJSON_IsNumber(value) || value->valuedouble < 0 || value->valuedouble > 100)) {
                    reason = "bar value must be 0..100";
                    ok = false;
                }
            } else if (kind == "button") {
                const cJSON *text = cJSON_GetObjectItemCaseSensitive(object, "text");
                const cJSON *action = cJSON_GetObjectItemCaseSensitive(object, "action");
                const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, "value");
                if (!validShortString(text, 64) || !validShortString(action, 32) ||
                    String(action->valuestring) != "set_brightness" ||
                    !cJSON_IsNumber(value) || value->valuedouble < 0 || value->valuedouble > 100) {
                    reason = "button v1 supports set_brightness 0..100";
                    ok = false;
                }
            } else {
                reason = String("unsupported object type: ") + kind;
                ok = false;
            }
        }
    }

    cJSON_Delete(root);
    if (ok) reason = "OK";
    return ok;
}

bool readWidgetFile(String &json, String &reason) {
    if (!LittleFS.exists(kWidgetPath)) {
        reason = "No widget installed";
        return false;
    }
    File file = LittleFS.open(kWidgetPath, "r");
    if (!file) {
        reason = "Cannot open widget.json";
        return false;
    }
    const size_t size = file.size();
    if (size == 0 || size > kMaxWidgetBytes) {
        file.close();
        reason = "Stored widget has invalid size";
        return false;
    }
    json.reserve(size + 1);
    while (file.available()) json += static_cast<char>(file.read());
    file.close();
    return validateWidgetJson(json, reason);
}

String valueForBinding(const String &binding) {
    if (binding == "system.uptime") {
        const uint32_t total = millis() / 1000UL;
        const uint32_t h = total / 3600UL;
        const uint32_t m = (total / 60UL) % 60UL;
        const uint32_t s = total % 60UL;
        char text[24];
        snprintf(text, sizeof(text), "%02lu:%02lu:%02lu",
                 static_cast<unsigned long>(h),
                 static_cast<unsigned long>(m),
                 static_cast<unsigned long>(s));
        return String(text);
    }
    if (binding == "system.heap") return String(ESP.getFreeHeap() / 1024UL) + " KiB";
    if (binding == "system.psram") return String(ESP.getFreePsram() / 1024UL) + " KiB";
    if (binding == "wifi.rssi") {
        if (WiFi.status() != WL_CONNECTED) return "offline";
        return String(WiFi.RSSI()) + " dBm";
    }
    if (binding == "wifi.ip") {
        if (WiFi.status() != WL_CONNECTED) return "offline";
        return WiFi.localIP().toString();
    }
    if (binding == "wifi.ap_ip") {
        if (!portalRunning) return "off";
        return WiFi.softAPIP().toString();
    }
    if (binding == "system.time") {
        struct tm now;
        if (!getLocalTime(&now, 0)) return "--:--:--";
        char text[16];
        strftime(text, sizeof(text), "%H:%M:%S", &now);
        return String(text);
    }
    return "unsupported";
}

void updateBindings(bool force = false) {
    const uint32_t now = millis();
    if (!force && now - lastBindingRefreshMs < kBindingRefreshMs) return;
    lastBindingRefreshMs = now;
    for (uint8_t i = 0; i < boundLabelCount; ++i) {
        if (!boundLabels[i].object) continue;
        const String value = boundLabels[i].prefix + valueForBinding(boundLabels[i].binding) + boundLabels[i].suffix;
        lv_label_set_text(boundLabels[i].object, value.c_str());
    }
}

int jsonInt(const cJSON *object, const char *key, int fallback) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    return cJSON_IsNumber(item) ? item->valueint : fallback;
}

const char *jsonString(const cJSON *object, const char *key, const char *fallback = "") {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    return (cJSON_IsString(item) && item->valuestring) ? item->valuestring : fallback;
}

void onBrightness(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    const intptr_t value = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    board.backlight().set(static_cast<uint8_t>(constrain(static_cast<int>(value), 0, 100)));
}

void onManager(lv_event_t *event) {
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) showManager();
}

bool renderWidget(const String &json, String &reason) {
    cJSON *root = cJSON_ParseWithLength(json.c_str(), json.length());
    if (!root) {
        reason = "JSON parse failed during render";
        return false;
    }

    boundLabelCount = 0;
    widgetScreen = lv_obj_create(nullptr);
    lv_obj_clear_flag(widgetScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(widgetScreen,
                              parseColor(jsonString(root, "background", "#101820"), 0x101820), 0);

    lv_obj_t *manager = lv_btn_create(widgetScreen);
    lv_obj_set_size(manager, 104, 30);
    lv_obj_align(manager, LV_ALIGN_TOP_RIGHT, -6, 4);
    lv_obj_add_event_cb(manager, onManager, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *managerText = lv_label_create(manager);
    lv_label_set_text(managerText, "MANAGER");
    lv_obj_center(managerText);

    lv_obj_t *runtime = lv_label_create(widgetScreen);
    lv_label_set_text(runtime, "WIDGET RUNTIME v1");
    lv_obj_set_style_text_color(runtime, lv_color_hex(0x607D8B), 0);
    lv_obj_set_pos(runtime, 8, 10);

    const cJSON *objects = cJSON_GetObjectItemCaseSensitive(root, "objects");
    const int count = cJSON_GetArraySize(objects);
    for (int i = 0; i < count; ++i) {
        const cJSON *object = cJSON_GetArrayItem(objects, i);
        const String kind(jsonString(object, "type"));
        const int x = constrain(jsonInt(object, "x", 16), 0, 460);
        const int y = constrain(jsonInt(object, "y", 16), 0, 270) + kWidgetTop;
        const int w = constrain(jsonInt(object, "w", kind == "button" ? 180 : 300), 20, 460);
        const int h = constrain(jsonInt(object, "h", kind == "button" ? 44 : 30), 18, 250);
        const lv_color_t color = parseColor(jsonString(object, "color", "#FFFFFF"), 0xFFFFFF);

        if (kind == "label") {
            lv_obj_t *label = lv_label_create(widgetScreen);
            lv_obj_set_pos(label, x, y);
            lv_obj_set_width(label, w);
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_color(label, color, 0);
            const char *binding = jsonString(object, "bind");
            if (binding[0] && boundLabelCount < kMaxBoundLabels) {
                boundLabels[boundLabelCount].object = label;
                boundLabels[boundLabelCount].binding = binding;
                boundLabels[boundLabelCount].prefix = jsonString(object, "prefix");
                boundLabels[boundLabelCount].suffix = jsonString(object, "suffix");
                ++boundLabelCount;
            } else {
                lv_label_set_text(label, jsonString(object, "text", ""));
            }
        }

        if (kind == "bar") {
            lv_obj_t *bar = lv_bar_create(widgetScreen);
            lv_obj_set_pos(bar, x, y);
            lv_obj_set_size(bar, w, h);
            lv_bar_set_range(bar, 0, 100);
            lv_bar_set_value(bar, constrain(jsonInt(object, "value", 50), 0, 100), LV_ANIM_OFF);
        }

        if (kind == "button") {
            lv_obj_t *button = lv_btn_create(widgetScreen);
            lv_obj_set_pos(button, x, y);
            lv_obj_set_size(button, w, h);
            const int value = constrain(jsonInt(object, "value", 80), 0, 100);
            lv_obj_add_event_cb(button, onBrightness, LV_EVENT_CLICKED,
                                reinterpret_cast<void *>(static_cast<intptr_t>(value)));
            lv_obj_t *label = lv_label_create(button);
            lv_label_set_text(label, jsonString(object, "text", "BUTTON"));
            lv_obj_center(label);
        }
    }

    cJSON_Delete(root);
    lv_scr_load(widgetScreen);
    updateBindings(true);
    reason = "Widget running";
    return true;
}

bool loadWidgetFromFlash(String &reason) {
    String json;
    if (!readWidgetFile(json, reason)) return false;
    return renderWidget(json, reason);
}

String installedWidgetSummary() {
    String json;
    String reason;
    if (!readWidgetFile(json, reason)) return reason;
    cJSON *root = cJSON_ParseWithLength(json.c_str(), json.length());
    if (!root) return "Invalid widget";
    const String summary = String(jsonString(root, "name", "Widget")) + "  v" + jsonString(root, "version", "?");
    cJSON_Delete(root);
    return summary;
}

void updateManagerLabels(const String &status) {
    if (managerStatus) lv_label_set_text(managerStatus, status.c_str());
    if (managerInstalled) {
        const String text = String("Installed: ") + installedWidgetSummary();
        lv_label_set_text(managerInstalled, text.c_str());
    }
    if (managerNetwork) {
        String text;
        if (portalRunning) {
            text = String("AP: ") + apSsid + "\nPASS: " + apPassword + "\nOPEN: http://" + WiFi.softAPIP().toString();
        } else {
            text = "Web upload AP is OFF";
        }
        lv_label_set_text(managerNetwork, text.c_str());
    }
    if (runButton) {
        if (LittleFS.exists(kWidgetPath)) lv_obj_clear_state(runButton, LV_STATE_DISABLED);
        else lv_obj_add_state(runButton, LV_STATE_DISABLED);
    }
    if (deleteButton) {
        if (LittleFS.exists(kWidgetPath)) lv_obj_clear_state(deleteButton, LV_STATE_DISABLED);
        else lv_obj_add_state(deleteButton, LV_STATE_DISABLED);
    }
}

String makePortalPage() {
    String html;
    html.reserve(2500);
    html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
    html += F("<title>WT32 Widget Manager</title><style>body{font-family:sans-serif;background:#101820;color:#eee;max-width:720px;margin:40px auto;padding:20px}main{background:#18252d;padding:24px;border-radius:16px}button,input{font-size:18px;padding:12px;margin:8px 0;width:100%}code{color:#80cbc4}</style></head><body><main>");
    html += F("<h1>WT32 Widget Manager</h1><p>Upload one declarative <code>widget.json</code>. Maximum 32 KiB. Native code is not accepted.</p>");
    html += F("<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='widget' accept='.json,application/json' required><button type='submit'>UPLOAD & INSTALL</button></form>");
    html += F("<p>After validation the file is stored in internal LittleFS and automatically launched. Press MANAGER on the display to return here.</p>");
    html += F("</main></body></html>");
    return html;
}

bool installTempWidget(String &reason) {
    File file = LittleFS.open(kTempWidgetPath, "r");
    if (!file) {
        reason = "Temporary upload missing";
        return false;
    }
    const size_t size = file.size();
    if (size == 0 || size > kMaxWidgetBytes) {
        file.close();
        reason = "Upload size invalid";
        return false;
    }
    String json;
    json.reserve(size + 1);
    while (file.available()) json += static_cast<char>(file.read());
    file.close();
    if (!validateWidgetJson(json, reason)) return false;

    LittleFS.remove(kWidgetPath);
    if (!LittleFS.rename(kTempWidgetPath, kWidgetPath)) {
        reason = "Could not activate widget.json";
        return false;
    }
    reason = "UPLOAD + SCHEMA PASS";
    return true;
}

void handleUploadStream() {
    HTTPUpload &upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        if (uploadFile) uploadFile.close();
        LittleFS.remove(kTempWidgetPath);
        uploadBytes = 0;
        uploadFailed = false;
        uploadSucceeded = false;
        uploadMessage = "Receiving widget.json";
        uploadFile = LittleFS.open(kTempWidgetPath, "w");
        if (!uploadFile) {
            uploadFailed = true;
            uploadMessage = "Cannot create temporary file";
        }
    }

    if (upload.status == UPLOAD_FILE_WRITE) {
        if (!uploadFailed) {
            if (uploadBytes + upload.currentSize > kMaxWidgetBytes) {
                uploadFailed = true;
                uploadMessage = "Widget exceeds 32 KiB";
            }
        }
        if (!uploadFailed) {
            const size_t written = uploadFile.write(upload.buf, upload.currentSize);
            if (written != upload.currentSize) {
                uploadFailed = true;
                uploadMessage = "LittleFS write failed";
            }
            uploadBytes += written;
        }
    }

    if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) uploadFile.close();
        if (!uploadFailed) uploadSucceeded = installTempWidget(uploadMessage);
        if (!uploadSucceeded) LittleFS.remove(kTempWidgetPath);
        Serial.printf("WIDGET UPLOAD: bytes=%lu result=%s detail=%s\n",
                      static_cast<unsigned long>(uploadBytes),
                      uploadSucceeded ? "PASS" : "FAIL",
                      uploadMessage.c_str());
        updateManagerLabels(uploadMessage);
    }

    if (upload.status == UPLOAD_FILE_ABORTED) {
        if (uploadFile) uploadFile.close();
        LittleFS.remove(kTempWidgetPath);
        uploadFailed = true;
        uploadSucceeded = false;
        uploadMessage = "Upload aborted";
        updateManagerLabels(uploadMessage);
    }
}

void configureWebServer() {
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html; charset=utf-8", makePortalPage());
    });

    server.on("/health", HTTP_GET, []() {
        server.send(200, "application/json", "{\"ok\":true,\"runtime\":1}");
    });

    server.on("/upload", HTTP_POST, []() {
        if (uploadSucceeded) {
            server.send(200, "text/html; charset=utf-8",
                        "<html><body><h1>INSTALL PASS</h1><p>Widget validated and stored in LittleFS. The display will load it now.</p></body></html>");
            runUploadedWidgetRequested = true;
            runUploadedWidgetAtMs = millis() + 1000;
        } else {
            server.send(400, "text/plain; charset=utf-8", uploadMessage);
        }
    }, handleUploadStream);

    server.onNotFound([]() {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });
}

void buildApCredentials() {
    const uint64_t mac = ESP.getEfuseMac();
    char suffix[7];
    snprintf(suffix, sizeof(suffix), "%06lX", static_cast<unsigned long>(mac & 0xFFFFFFULL));
    apSsid = String("WT32-WIDGET-") + String(suffix + 2);
    apPassword = String("WT32-") + suffix;
}

bool startPortal() {
    if (portalRunning) return true;
    WiFi.mode(WIFI_AP_STA);
    if (WiFi.status() != WL_CONNECTED) WiFi.begin();
    if (!WiFi.softAP(apSsid.c_str(), apPassword.c_str())) {
        updateManagerLabels("SOFTAP START FAILED");
        return false;
    }
    server.begin();
    portalRunning = true;
    portalStartedMs = millis();
    Serial.printf("WIDGET PORTAL: SSID=%s PASS=%s URL=http://%s\n",
                  apSsid.c_str(), apPassword.c_str(), WiFi.softAPIP().toString().c_str());
    updateManagerLabels("WEB UPLOAD READY");
    return true;
}

void stopPortal() {
    if (!portalRunning) return;
    server.stop();
    WiFi.softAPdisconnect(true);
    portalRunning = false;
    updateManagerLabels("WEB UPLOAD AP STOPPED");
}

void onRun(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    String reason;
    if (!loadWidgetFromFlash(reason)) updateManagerLabels(String("RUN FAIL: ") + reason);
}

void onWebUpload(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    startPortal();
}

void onStopAp(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    stopPortal();
}

void onDelete(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    LittleFS.remove(kWidgetPath);
    LittleFS.remove(kTempWidgetPath);
    updateManagerLabels("WIDGET DELETED");
}

lv_obj_t *makeButton(lv_obj_t *parent, const char *text, int x, int y, int w, lv_event_cb_t callback) {
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, w, 44);
    lv_obj_set_pos(button, x, y);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return button;
}

void buildManagerScreen() {
    managerScreen = lv_obj_create(nullptr);
    lv_obj_clear_flag(managerScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(managerScreen, lv_color_hex(0x101820), 0);

    lv_obj_t *title = lv_label_create(managerScreen);
    lv_label_set_text(title, "WT32 WIDGET MANAGER");
    lv_obj_set_style_text_color(title, lv_color_hex(0x80CBC4), 0);
    lv_obj_set_pos(title, 18, 14);

    managerInstalled = lv_label_create(managerScreen);
    lv_obj_set_width(managerInstalled, 440);
    lv_obj_set_pos(managerInstalled, 18, 48);
    lv_obj_set_style_text_color(managerInstalled, lv_color_white(), 0);

    managerNetwork = lv_label_create(managerScreen);
    lv_obj_set_width(managerNetwork, 440);
    lv_obj_set_pos(managerNetwork, 18, 78);
    lv_obj_set_style_text_color(managerNetwork, lv_color_hex(0x90A4AE), 0);

    runButton = makeButton(managerScreen, "RUN WIDGET", 18, 154, 140, onRun);
    makeButton(managerScreen, "WEB UPLOAD", 170, 154, 140, onWebUpload);
    makeButton(managerScreen, "STOP AP", 322, 154, 140, onStopAp);
    deleteButton = makeButton(managerScreen, "DELETE", 18, 208, 140, onDelete);

    managerStatus = lv_label_create(managerScreen);
    lv_obj_set_width(managerStatus, 286);
    lv_label_set_long_mode(managerStatus, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(managerStatus, 176, 214);
    lv_obj_set_style_text_color(managerStatus, lv_color_hex(0x80CBC4), 0);

    updateManagerLabels("READY");
}

void showManager() {
    if (!managerScreen) buildManagerScreen();
    lv_scr_load(managerScreen);
    updateManagerLabels("READY");
}

void printBootInfo() {
    Serial.println("============================================================");
    Serial.println("WT32-SC01-PLUS 21_LVGL_WidgetLoader");
    Serial.println("WIDGET SCHEMA: 1");
    Serial.printf("WIDGET STORE: %s | max=%lu bytes\n",
                  kWidgetPath, static_cast<unsigned long>(kMaxWidgetBytes));
    Serial.printf("FLASH: %lu MiB | PSRAM: %lu MiB\n",
                  static_cast<unsigned long>(ESP.getFlashChipSize() / 1048576UL),
                  static_cast<unsigned long>(ESP.getPsramSize() / 1048576UL));
    Serial.printf("LITTLEFS: total=%lu used=%lu bytes\n",
                  static_cast<unsigned long>(LittleFS.totalBytes()),
                  static_cast<unsigned long>(LittleFS.usedBytes()));
    Serial.println("SECURITY: declarative JSON only; native widget code is not loaded");
    Serial.println("============================================================");
}
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(500);

    if (!board.begin()) {
        Serial.println("ERROR: BSP initialization failed");
        while (true) delay(1000);
    }
    if (!board.touch().begin()) {
        Serial.println("ERROR: touch initialization failed");
        while (true) delay(1000);
    }
    board.backlight().set(kDefaultBrightness);

    if (!LittleFS.begin(true)) {
        Serial.println("ERROR: LittleFS mount/format failed");
        while (true) delay(1000);
    }
    if (!LittleFS.exists("/widgets")) LittleFS.mkdir("/widgets");

    lv_init();
    static_assert(sizeof(lv_color_t) == sizeof(uint16_t), "LVGL must use RGB565");
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

    buildApCredentials();
    configureWebServer();
    buildManagerScreen();
    printBootInfo();

    WiFi.mode(WIFI_STA);
    WiFi.begin();

    String reason;
    if (loadWidgetFromFlash(reason)) {
        Serial.println("WIDGET AUTOLOAD: PASS");
    } else {
        Serial.printf("WIDGET AUTOLOAD: %s\n", reason.c_str());
        showManager();
    }
}

void loop() {
    serviceLvgl();
    updateBindings();

    if (portalRunning) {
        server.handleClient();
        if (millis() - portalStartedMs >= kPortalTimeoutMs) stopPortal();
    }

    if (runUploadedWidgetRequested && millis() >= runUploadedWidgetAtMs) {
        runUploadedWidgetRequested = false;
        String reason;
        if (!loadWidgetFromFlash(reason)) updateManagerLabels(String("RUN FAIL: ") + reason);
    }

    delay(5);
}
