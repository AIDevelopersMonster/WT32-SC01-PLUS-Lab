/*
 * WT32-SC01-PLUS-Lab
 * Example 20: LVGL GitHub OTA
 *
 * First installation: USB/Web Flasher.
 * Subsequent installation: direct HTTPS download from GitHub Releases.
 *
 * Target specimen:
 *   Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC / 230208
 *   ESP32-S3, 16 MiB Flash, 2 MiB QSPI PSRAM
 *
 * Status: SOURCE / CI TARGET. Physical validation required before PASS.
 */

#include <WT32_SC01_PLUS.h>
#include <lvgl.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <esp_ota_ops.h>
#include "firmware_version.h"

WT32_SC01_PLUS board;

namespace {
constexpr uint16_t kBufferLines = 20;
constexpr uint8_t kDefaultBrightness = 80;
constexpr uint32_t kWiFiConnectTimeoutMs = 15000;
constexpr uint32_t kHttpTimeoutMs = 20000;
constexpr size_t kManifestMaxBytes = 8192;
constexpr size_t kDownloadBufferSize = 4096;

constexpr const char *kManifestUrl =
    "https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab/releases/latest/download/"
    WT32_OTA_MANIFEST_NAME;

// Root CAs used by GitHub's current TLS hierarchy. Certificate verification is
// intentionally enabled; this example does NOT use setInsecure().
// DigiCert Global Root G2 + DigiCert Global Root G3.
static const char kGitHubRootCAs[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICPzCCAcWgAwIBAgIQBVVWvPJepDU1w6QP1atFcjAKBggqhkjOPQQDAzBhMQsw
CQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3d3cu
ZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBHMzAe
Fw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVTMRUw
EwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5jb20x
IDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEczMHYwEAYHKoZIzj0CAQYF
K4EEACIDYgAE3afZu4q4C/sLfyHS8L6+c/MzXRq8NOrexpu80JX28MzQC7phW1FG
fp4tn+6OYwwX7Adw9c+ELkCDnOg/QW07rdOkFFk2eJ0DQ+4QE2xy3q6Ip6FrtUPO
Z9wj/wMco+I+o0IwQDAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAd
BgNVHQ4EFgQUs9tIpPmhxdiuNkHMEWNpYim8S8YwCgYIKoZIzj0EAwMDaAAwZQIx
AK288mw/EkrRLTnDCgmXc/SINoyIJ7vmiI1Qhadj+Z4y3maTD/HMsQmP3Wyr+mt/
oAIwOWZbwmSNuJ5Q3KjVSaLtx9zRSX8XAbjIho9OjIgrqJqpisXRAL34VOKa5Vt8
sycX
-----END CERTIFICATE-----
)EOF";

struct OtaManifest {
    String board;
    String version;
    String channel;
    String firmwareUrl;
    String sha256;
    size_t size = 0;
    bool valid = false;
};

static lv_color_t drawBufferPixels[wt32sc01plus::pins::LCD_WIDTH * kBufferLines];
static lv_disp_draw_buf_t drawBuffer;
static lv_disp_drv_t displayDriver;
static lv_indev_drv_t touchDriver;

static lv_obj_t *statusLabel = nullptr;
static lv_obj_t *detailLabel = nullptr;
static lv_obj_t *versionLabel = nullptr;
static lv_obj_t *progressBar = nullptr;
static lv_obj_t *progressLabel = nullptr;
static lv_obj_t *checkButton = nullptr;
static lv_obj_t *installButton = nullptr;

static bool checkRequested = false;
static bool installRequested = false;
static bool busy = false;
static OtaManifest availableManifest;

void serviceLvgl();

void flushDisplay(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *colorPixels) {
    board.display().drawRGB565(
        area->x1, area->y1,
        area->x2 - area->x1 + 1,
        area->y2 - area->y1 + 1,
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

void setStatus(const String &status, const String &detail = "") {
    Serial.printf("OTA STATUS: %s", status.c_str());
    if (detail.length()) Serial.printf(" | %s", detail.c_str());
    Serial.println();
    if (statusLabel) lv_label_set_text(statusLabel, status.c_str());
    if (detailLabel) lv_label_set_text(detailLabel, detail.c_str());
    serviceLvgl();
}

void setProgress(size_t done, size_t total) {
    int percent = total ? static_cast<int>((done * 100ULL) / total) : 0;
    percent = constrain(percent, 0, 100);
    if (progressBar) lv_bar_set_value(progressBar, percent, LV_ANIM_OFF);
    if (progressLabel) {
        lv_label_set_text_fmt(progressLabel, "%d%%  %lu / %lu KiB",
                              percent,
                              static_cast<unsigned long>(done / 1024UL),
                              static_cast<unsigned long>(total / 1024UL));
    }
    serviceLvgl();
}

bool parseJsonString(const String &json, const char *key, String &out) {
    String token = String('"') + key + '"';
    int keyPos = json.indexOf(token);
    if (keyPos < 0) return false;
    int colon = json.indexOf(':', keyPos + token.length());
    if (colon < 0) return false;
    int first = json.indexOf('"', colon + 1);
    if (first < 0) return false;
    int second = json.indexOf('"', first + 1);
    if (second < 0) return false;
    out = json.substring(first + 1, second);
    return out.length() > 0;
}

bool parseJsonSize(const String &json, const char *key, size_t &out) {
    String token = String('"') + key + '"';
    int keyPos = json.indexOf(token);
    if (keyPos < 0) return false;
    int colon = json.indexOf(':', keyPos + token.length());
    if (colon < 0) return false;
    int start = colon + 1;
    while (start < static_cast<int>(json.length()) && isspace(json[start])) ++start;
    int end = start;
    while (end < static_cast<int>(json.length()) && isdigit(json[end])) ++end;
    if (end == start) return false;
    out = static_cast<size_t>(strtoull(json.substring(start, end).c_str(), nullptr, 10));
    return out > 0;
}

bool parseManifest(const String &json, OtaManifest &manifest) {
    manifest = OtaManifest{};
    if (!parseJsonString(json, "board", manifest.board)) return false;
    if (!parseJsonString(json, "version", manifest.version)) return false;
    if (!parseJsonString(json, "channel", manifest.channel)) return false;
    if (!parseJsonString(json, "firmware", manifest.firmwareUrl)) return false;
    if (!parseJsonString(json, "sha256", manifest.sha256)) return false;
    if (!parseJsonSize(json, "size", manifest.size)) return false;
    manifest.sha256.toLowerCase();
    manifest.valid = manifest.sha256.length() == 64;
    return manifest.valid;
}

bool parseVersion(const String &version, int &major, int &minor, int &patch) {
    return sscanf(version.c_str(), "%d.%d.%d", &major, &minor, &patch) == 3;
}

int compareVersions(const String &a, const String &b) {
    int amaj = 0, amin = 0, apat = 0;
    int bmaj = 0, bmin = 0, bpat = 0;
    if (!parseVersion(a, amaj, amin, apat) || !parseVersion(b, bmaj, bmin, bpat)) {
        return a.compareTo(b);
    }
    if (amaj != bmaj) return amaj < bmaj ? -1 : 1;
    if (amin != bmin) return amin < bmin ? -1 : 1;
    if (apat != bpat) return apat < bpat ? -1 : 1;
    return 0;
}

bool ensureWiFi() {
    if (WiFi.status() == WL_CONNECTED) return true;

    setStatus("CONNECTING WIFI", "Using saved station credentials");
    WiFi.mode(WIFI_STA);
    WiFi.begin();
    const uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < kWiFiConnectTimeoutMs) {
        serviceLvgl();
        delay(50);
    }

    if (WiFi.status() != WL_CONNECTED) {
        setStatus("WIFI REQUIRED",
                  "No saved connection. Run QR provisioning (#18) or connect once first.");
        return false;
    }

    setStatus("WIFI CONNECTED", WiFi.localIP().toString());
    return true;
}

bool fetchManifest(OtaManifest &manifest) {
    if (!ensureWiFi()) return false;

    setStatus("CHECKING GITHUB", WT32_OTA_MANIFEST_NAME);
    NetworkClientSecure client;
    client.setCACert(kGitHubRootCAs);
    client.setHandshakeTimeout(15);

    HTTPClient http;
    http.setConnectTimeout(kHttpTimeoutMs);
    http.setTimeout(kHttpTimeoutMs);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setRedirectLimit(8);
    http.setUserAgent("WT32-SC01-PLUS-GitHubOTA/" WT32_OTA_VERSION);

    if (!http.begin(client, kManifestUrl)) {
        setStatus("MANIFEST ERROR", "HTTP begin failed");
        return false;
    }

    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        String error = String("HTTP ") + code;
        http.end();
        setStatus("MANIFEST ERROR", error);
        return false;
    }

    const int declaredSize = http.getSize();
    if (declaredSize > static_cast<int>(kManifestMaxBytes)) {
        http.end();
        setStatus("MANIFEST ERROR", "Manifest exceeds 8 KiB safety limit");
        return false;
    }

    String body = http.getString();
    http.end();
    if (body.length() == 0 || body.length() > kManifestMaxBytes) {
        setStatus("MANIFEST ERROR", "Empty/oversized manifest");
        return false;
    }

    if (!parseManifest(body, manifest)) {
        setStatus("MANIFEST ERROR", "Required fields missing or invalid");
        return false;
    }

    if (manifest.board != WT32_OTA_BOARD_ID) {
        setStatus("BOARD MISMATCH", manifest.board);
        return false;
    }
    if (manifest.channel != WT32_OTA_CHANNEL) {
        setStatus("CHANNEL MISMATCH", manifest.channel);
        return false;
    }

    const esp_partition_t *next = esp_ota_get_next_update_partition(nullptr);
    if (!next) {
        setStatus("OTA LAYOUT ERROR", "No inactive OTA partition found");
        return false;
    }
    if (manifest.size > next->size) {
        setStatus("IMAGE TOO LARGE",
                  String(manifest.size) + " > slot " + String(next->size));
        return false;
    }

    manifest.valid = true;
    return true;
}

String digestToHex(const unsigned char digest[32]) {
    static const char *hex = "0123456789abcdef";
    char text[65];
    for (int i = 0; i < 32; ++i) {
        text[i * 2] = hex[(digest[i] >> 4) & 0x0F];
        text[i * 2 + 1] = hex[digest[i] & 0x0F];
    }
    text[64] = '\0';
    return String(text);
}

bool installManifest(const OtaManifest &manifest) {
    if (!manifest.valid) {
        setStatus("OTA ERROR", "No validated manifest loaded");
        return false;
    }
    if (!ensureWiFi()) return false;

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(nullptr);
    if (!running || !next) {
        setStatus("OTA LAYOUT ERROR", "Running/next OTA partition unavailable");
        return false;
    }

    Serial.printf("OTA PARTITIONS: running=%s @0x%08lx next=%s @0x%08lx size=%lu\n",
                  running->label, static_cast<unsigned long>(running->address),
                  next->label, static_cast<unsigned long>(next->address),
                  static_cast<unsigned long>(next->size));

    setStatus("DOWNLOADING", manifest.version);
    setProgress(0, manifest.size);

    NetworkClientSecure client;
    client.setCACert(kGitHubRootCAs);
    client.setHandshakeTimeout(15);

    HTTPClient http;
    http.setConnectTimeout(kHttpTimeoutMs);
    http.setTimeout(kHttpTimeoutMs);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setRedirectLimit(8);
    http.setUserAgent("WT32-SC01-PLUS-GitHubOTA/" WT32_OTA_VERSION);

    if (!http.begin(client, manifest.firmwareUrl)) {
        setStatus("DOWNLOAD ERROR", "HTTP begin failed");
        return false;
    }

    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        String error = String("HTTP ") + code;
        http.end();
        setStatus("DOWNLOAD ERROR", error);
        return false;
    }

    const int contentLength = http.getSize();
    if (contentLength > 0 && static_cast<size_t>(contentLength) != manifest.size) {
        http.end();
        setStatus("SIZE MISMATCH",
                  String(contentLength) + " != manifest " + String(manifest.size));
        return false;
    }

    if (!Update.begin(manifest.size, U_FLASH)) {
        String error = String("Update.begin: ") + Update.errorString();
        http.end();
        setStatus("OTA WRITE ERROR", error);
        return false;
    }

    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts(&sha, 0);

    NetworkClient *stream = http.getStreamPtr();
    uint8_t buffer[kDownloadBufferSize];
    size_t total = 0;
    uint32_t lastProgressMs = 0;
    bool streamOk = true;

    while (total < manifest.size) {
        const size_t remaining = manifest.size - total;
        const size_t want = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        const int available = stream->available();

        if (available <= 0) {
            if (!http.connected()) {
                streamOk = false;
                break;
            }
            serviceLvgl();
            delay(5);
            continue;
        }

        const size_t chunk = min(want, static_cast<size_t>(available));
        const int read = stream->readBytes(buffer, chunk);
        if (read <= 0) {
            streamOk = false;
            break;
        }

        mbedtls_sha256_update(&sha, buffer, read);
        const size_t written = Update.write(buffer, read);
        if (written != static_cast<size_t>(read)) {
            streamOk = false;
            break;
        }
        total += written;

        if (millis() - lastProgressMs > 100 || total == manifest.size) {
            setProgress(total, manifest.size);
            lastProgressMs = millis();
        }
    }

    http.end();

    unsigned char digest[32];
    mbedtls_sha256_finish(&sha, digest);
    mbedtls_sha256_free(&sha);

    if (!streamOk || total != manifest.size) {
        Update.abort();
        setStatus("DOWNLOAD ERROR",
                  String("Received ") + total + " / " + manifest.size + " bytes");
        return false;
    }

    const String calculated = digestToHex(digest);
    Serial.printf("OTA SHA256 expected=%s\n", manifest.sha256.c_str());
    Serial.printf("OTA SHA256 actual  =%s\n", calculated.c_str());
    if (!calculated.equalsIgnoreCase(manifest.sha256)) {
        Update.abort();
        setStatus("SHA256 FAILED", "Inactive slot NOT activated");
        return false;
    }

    setStatus("SHA256 PASS", "Finalizing inactive OTA slot");
    if (!Update.end(true) || !Update.isFinished()) {
        setStatus("OTA FINALIZE ERROR", Update.errorString());
        return false;
    }

    setProgress(manifest.size, manifest.size);
    setStatus("UPDATE READY",
              String(WT32_OTA_VERSION) + " -> " + manifest.version + " | rebooting");
    delay(1200);
    ESP.restart();
    return true;
}

void checkForUpdate() {
    OtaManifest manifest;
    if (!fetchManifest(manifest)) {
        availableManifest = OtaManifest{};
        if (installButton) lv_obj_add_state(installButton, LV_STATE_DISABLED);
        return;
    }

    availableManifest = manifest;
    if (versionLabel) {
        lv_label_set_text_fmt(versionLabel,
                              "Installed: %s\nAvailable: %s\nChannel: %s",
                              WT32_OTA_VERSION,
                              manifest.version.c_str(),
                              manifest.channel.c_str());
    }

    const int cmp = compareVersions(WT32_OTA_VERSION, manifest.version);
    if (cmp < 0) {
        setStatus("UPDATE AVAILABLE",
                  String(manifest.version) + " | " + String(manifest.size / 1024UL) + " KiB");
        if (installButton) lv_obj_clear_state(installButton, LV_STATE_DISABLED);
    } else if (cmp == 0) {
        setStatus("UP TO DATE", WT32_OTA_VERSION);
        if (installButton) lv_obj_add_state(installButton, LV_STATE_DISABLED);
    } else {
        setStatus("LOCAL VERSION NEWER", manifest.version);
        if (installButton) lv_obj_add_state(installButton, LV_STATE_DISABLED);
    }
}

void onCheck(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || busy) return;
    checkRequested = true;
}

void onInstall(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || busy || !availableManifest.valid) return;
    installRequested = true;
}

void buildUi() {
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "GITHUB OTA");
    lv_obj_set_style_text_color(title, lv_color_hex(0x80CBC4), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    versionLabel = lv_label_create(screen);
    lv_label_set_text_fmt(versionLabel,
                          "Installed: %s\nAvailable: not checked\nChannel: %s",
                          WT32_OTA_VERSION, WT32_OTA_CHANNEL);
    lv_obj_set_style_text_color(versionLabel, lv_color_white(), 0);
    lv_obj_set_pos(versionLabel, 24, 48);

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(nullptr);
    lv_obj_t *slot = lv_label_create(screen);
    lv_label_set_text_fmt(slot, "Slot: %s -> %s | OTA slot %.2f MiB",
                          running ? running->label : "?",
                          next ? next->label : "?",
                          next ? static_cast<double>(next->size) / 1048576.0 : 0.0);
    lv_obj_set_style_text_color(slot, lv_color_hex(0x90A4AE), 0);
    lv_obj_set_pos(slot, 24, 108);

    checkButton = lv_btn_create(screen);
    lv_obj_set_size(checkButton, 190, 46);
    lv_obj_set_pos(checkButton, 24, 140);
    lv_obj_add_event_cb(checkButton, onCheck, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *checkLabel = lv_label_create(checkButton);
    lv_label_set_text(checkLabel, "CHECK GITHUB");
    lv_obj_center(checkLabel);

    installButton = lv_btn_create(screen);
    lv_obj_set_size(installButton, 218, 46);
    lv_obj_set_pos(installButton, 238, 140);
    lv_obj_add_event_cb(installButton, onInstall, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_state(installButton, LV_STATE_DISABLED);
    lv_obj_t *installLabel = lv_label_create(installButton);
    lv_label_set_text(installLabel, "DOWNLOAD & INSTALL");
    lv_obj_center(installLabel);

    progressBar = lv_bar_create(screen);
    lv_obj_set_size(progressBar, 432, 18);
    lv_obj_set_pos(progressBar, 24, 204);
    lv_bar_set_range(progressBar, 0, 100);
    lv_bar_set_value(progressBar, 0, LV_ANIM_OFF);

    progressLabel = lv_label_create(screen);
    lv_label_set_text(progressLabel, "0%");
    lv_obj_set_style_text_color(progressLabel, lv_color_hex(0x90A4AE), 0);
    lv_obj_align_to(progressLabel, progressBar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    statusLabel = lv_label_create(screen);
    lv_label_set_text(statusLabel, "READY");
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x80CBC4), 0);
    lv_obj_set_pos(statusLabel, 24, 252);

    detailLabel = lv_label_create(screen);
    lv_label_set_long_mode(detailLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(detailLabel, 432);
    lv_label_set_text(detailLabel, "Uses saved Wi-Fi credentials. HTTPS certificate verification + SHA-256 enabled.");
    lv_obj_set_style_text_color(detailLabel, lv_color_hex(0x90A4AE), 0);
    lv_obj_set_pos(detailLabel, 24, 274);
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

void printBootInfo() {
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(nullptr);
    Serial.println("============================================================");
    Serial.println("WT32-SC01-PLUS 20_LVGL_GitHubOTA");
    Serial.printf("VERSION: %s (%s)\n", WT32_OTA_VERSION, WT32_OTA_CHANNEL);
    Serial.printf("BOARD ID: %s\n", WT32_OTA_BOARD_ID);
    Serial.printf("FLASH: %lu MiB | PSRAM: %lu MiB\n",
                  static_cast<unsigned long>(ESP.getFlashChipSize() / 1048576UL),
                  static_cast<unsigned long>(ESP.getPsramSize() / 1048576UL));
    Serial.printf("RUNNING SLOT: %s @0x%08lx size=%lu\n",
                  running ? running->label : "NONE",
                  running ? static_cast<unsigned long>(running->address) : 0UL,
                  running ? static_cast<unsigned long>(running->size) : 0UL);
    Serial.printf("NEXT SLOT: %s @0x%08lx size=%lu\n",
                  next ? next->label : "NONE",
                  next ? static_cast<unsigned long>(next->address) : 0UL,
                  next ? static_cast<unsigned long>(next->size) : 0UL);
    Serial.printf("MANIFEST: %s\n", kManifestUrl);
    Serial.println("TLS: CA verification enabled; setInsecure() is NOT used");
#if CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    Serial.println("BOOTLOADER ROLLBACK: ENABLED");
#else
    Serial.println("BOOTLOADER ROLLBACK: NOT ENABLED in current Arduino bootloader");
#endif
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

    buildUi();
    printBootInfo();

    if (ESP.getFlashChipSize() != 16UL * 1024UL * 1024UL) {
        setStatus("FLASH PROFILE ERROR", "Expected physical 16 MiB Flash");
    } else if (!esp_ota_get_next_update_partition(nullptr)) {
        setStatus("OTA LAYOUT ERROR", "Flash once with bundled partitions.csv");
    } else {
        setStatus("READY", "Press CHECK GITHUB");
    }
}

void loop() {
    serviceLvgl();

    if (checkRequested && !busy) {
        checkRequested = false;
        busy = true;
        lv_obj_add_state(checkButton, LV_STATE_DISABLED);
        lv_obj_add_state(installButton, LV_STATE_DISABLED);
        checkForUpdate();
        lv_obj_clear_state(checkButton, LV_STATE_DISABLED);
        busy = false;
    }

    if (installRequested && !busy) {
        installRequested = false;
        busy = true;
        lv_obj_add_state(checkButton, LV_STATE_DISABLED);
        lv_obj_add_state(installButton, LV_STATE_DISABLED);
        installManifest(availableManifest);
        lv_obj_clear_state(checkButton, LV_STATE_DISABLED);
        if (availableManifest.valid &&
            compareVersions(WT32_OTA_VERSION, availableManifest.version) < 0) {
            lv_obj_clear_state(installButton, LV_STATE_DISABLED);
        }
        busy = false;
    }

    delay(5);
}
