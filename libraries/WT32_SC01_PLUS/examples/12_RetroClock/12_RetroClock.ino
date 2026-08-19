#include <WT32_SC01_PLUS.h>

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>

WT32_SC01_PLUS board;
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

namespace {

constexpr const char *kHostname = "wt32-clock";
constexpr const char *kApPassword = "WT32SETUP";
constexpr const char *kNtp1 = "pool.ntp.org";
constexpr const char *kNtp2 = "time.nist.gov";
constexpr uint32_t kConnectTimeoutMs = 20000;
constexpr uint32_t kApGraceAfterConnectMs = 60000;
constexpr uint32_t kClockRefreshMs = 250;
constexpr uint32_t kReconnectIntervalMs = 30000;
constexpr int kMaxScanNetworks = 20;

constexpr uint16_t C_BLACK = 0x0000;
constexpr uint16_t C_WHITE = 0xFFFF;

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8U) << 8) |
                               ((g & 0xFCU) << 3) |
                               (b >> 3));
}

const uint16_t C_CYAN = rgb565(70, 220, 255);
const uint16_t C_BLUE = rgb565(35, 90, 220);
const uint16_t C_GREEN = rgb565(70, 235, 120);
const uint16_t C_AMBER = rgb565(255, 135, 15);
const uint16_t C_AMBER_DIM = rgb565(42, 17, 2);
const uint16_t C_RED = rgb565(255, 70, 70);
const uint16_t C_MUTED = rgb565(105, 120, 135);

struct ZonePreset {
  const char *id;
  const char *webLabel;
  const char *displayLabel;
  const char *tz;
};

const ZonePreset kZones[] = {
    {"utc", "UTC", "UTC", "UTC0"},
    {"moscow", "Moscow", "MOSCOW", "MSK-3"},
    {"stockholm", "Stockholm / Central Europe", "STOCKHOLM", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"london", "London", "LONDON", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"newyork", "New York", "NEW YORK", "EST5EDT,M3.2.0/2,M11.1.0/2"},
    {"tokyo", "Tokyo", "TOKYO", "JST-9"},
    {"custom", "Custom POSIX TZ", "CUSTOM", ""},
};
constexpr size_t kZoneCount = sizeof(kZones) / sizeof(kZones[0]);

struct Glyph {
  char ch;
  uint8_t row[7];
};

const Glyph kFont[] = {
    {' ', {0,0,0,0,0,0,0}},
    {'-', {0,0,0,31,0,0,0}},
    {'.', {0,0,0,0,0,12,12}},
    {'/', {1,2,4,8,16,0,0}},
    {':', {0,4,4,0,4,4,0}},
    {'0', {14,17,19,21,25,17,14}},
    {'1', {4,12,4,4,4,4,14}},
    {'2', {14,17,1,2,4,8,31}},
    {'3', {30,1,1,14,1,1,30}},
    {'4', {2,6,10,18,31,2,2}},
    {'5', {31,16,16,30,1,1,30}},
    {'6', {14,16,16,30,17,17,14}},
    {'7', {31,1,2,4,8,8,8}},
    {'8', {14,17,17,14,17,17,14}},
    {'9', {14,17,17,15,1,1,14}},
    {'A', {14,17,17,31,17,17,17}},
    {'B', {30,17,17,30,17,17,30}},
    {'C', {15,16,16,16,16,16,15}},
    {'D', {30,17,17,17,17,17,30}},
    {'E', {31,16,16,30,16,16,31}},
    {'F', {31,16,16,30,16,16,16}},
    {'G', {15,16,16,23,17,17,15}},
    {'H', {17,17,17,31,17,17,17}},
    {'I', {14,4,4,4,4,4,14}},
    {'J', {7,2,2,2,2,18,12}},
    {'K', {17,18,20,24,20,18,17}},
    {'L', {16,16,16,16,16,16,31}},
    {'M', {17,27,21,21,17,17,17}},
    {'N', {17,25,21,19,17,17,17}},
    {'O', {14,17,17,17,17,17,14}},
    {'P', {30,17,17,30,16,16,16}},
    {'Q', {14,17,17,17,21,18,13}},
    {'R', {30,17,17,30,20,18,17}},
    {'S', {15,16,16,14,1,1,30}},
    {'T', {31,4,4,4,4,4,4}},
    {'U', {17,17,17,17,17,17,14}},
    {'V', {17,17,17,17,17,10,4}},
    {'W', {17,17,17,21,21,21,10}},
    {'X', {17,17,10,4,10,17,17}},
    {'Y', {17,17,10,4,4,4,4}},
    {'Z', {31,1,2,4,8,16,31}},
};
constexpr size_t kFontCount = sizeof(kFont) / sizeof(kFont[0]);

String savedSSID;
String savedPassword;
String savedZone;
String savedTZ;
String apSSID;
String scanSSID[kMaxScanNetworks];
int scanCount = 0;

bool portalActive = false;
bool serverStarted = false;
bool pendingConnect = false;
bool stationReady = false;
bool timeConfigured = false;
bool timeSynced = false;
bool connectFailed = false;
uint32_t connectStartedMs = 0;
uint32_t apStopAtMs = 0;
uint32_t lastClockRefreshMs = 0;
uint32_t lastReconnectMs = 0;
int lastRenderedSecond = -1;

const Glyph *findGlyph(char c) {
  if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  for (size_t i = 0; i < kFontCount; ++i) {
    if (kFont[i].ch == c) return &kFont[i];
  }
  return &kFont[0];
}

int textWidth(const String &text, int scale) {
  if (text.length() == 0) return 0;
  return static_cast<int>(text.length()) * 6 * scale - scale;
}

void drawChar(char c, int x, int y, int scale, uint16_t color) {
  const Glyph *glyph = findGlyph(c);
  for (int row = 0; row < 7; ++row) {
    for (int col = 0; col < 5; ++col) {
      if (glyph->row[row] & (1U << (4 - col))) {
        board.display().fillRect(x + col * scale, y + row * scale,
                                 scale, scale, color);
      }
    }
  }
}

void drawText(const String &text, int x, int y, int scale, uint16_t color) {
  int cursor = x;
  for (size_t i = 0; i < text.length(); ++i) {
    drawChar(text[i], cursor, y, scale, color);
    cursor += 6 * scale;
  }
}

void drawCenteredText(const String &text, int y, int scale, uint16_t color) {
  int x = (board.display().width() - textWidth(text, scale)) / 2;
  if (x < 0) x = 0;
  drawText(text, x, y, scale, color);
}

String upperForDisplay(String value) {
  value.toUpperCase();
  return value;
}

String htmlEscape(const String &input) {
  String out;
  out.reserve(input.length() + 16);
  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];
    if (c == '&') out += F("&amp;");
    else if (c == '<') out += F("&lt;");
    else if (c == '>') out += F("&gt;");
    else if (c == '"') out += F("&quot;");
    else if (c == '\'') out += F("&#39;");
    else out += c;
  }
  return out;
}

const ZonePreset *findZone(const String &id) {
  for (size_t i = 0; i < kZoneCount; ++i) {
    if (id == kZones[i].id) return &kZones[i];
  }
  return nullptr;
}

String zoneDisplayLabel() {
  const ZonePreset *zone = findZone(savedZone);
  return zone ? String(zone->displayLabel) : String("CUSTOM");
}

void loadSettings() {
  preferences.begin("retroclock", true);
  savedSSID = preferences.getString("ssid", "");
  savedPassword = preferences.getString("pass", "");
  savedZone = preferences.getString("zone", "");
  savedTZ = preferences.getString("tz", "UTC0");
  preferences.end();
}

void saveSettings(const String &ssid, const String &password,
                  const String &zone, const String &tz) {
  preferences.begin("retroclock", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", password);
  preferences.putString("zone", zone);
  preferences.putString("tz", tz);
  preferences.end();

  savedSSID = ssid;
  savedPassword = password;
  savedZone = zone;
  savedTZ = tz;
}

void buildApSSID() {
  const uint16_t suffix = static_cast<uint16_t>(ESP.getEfuseMac() & 0xFFFFU);
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "WT32-CLOCK-%04X", suffix);
  apSSID = buffer;
}

void drawSetupScreen(const String &status = "") {
  board.display().fillScreen(C_BLACK);
  drawCenteredText("RETRO CLOCK SETUP", 15, 3, C_CYAN);
  drawCenteredText("CONNECT WI-FI", 72, 2, C_WHITE);
  drawCenteredText(apSSID, 107, 2, C_GREEN);
  drawCenteredText("PASS: WT32SETUP", 145, 2, C_AMBER);
  drawCenteredText("OPEN: 192.168.4.1", 183, 2, C_WHITE);
  drawCenteredText("PHONE OR PC", 224, 2, C_MUTED);
  if (status.length()) drawCenteredText(status, 274, 2, C_RED);
}

void drawConnectingScreen() {
  board.display().fillScreen(C_BLACK);
  drawCenteredText("RETRO CLOCK", 35, 3, C_CYAN);
  drawCenteredText("CONNECTING", 105, 3, C_AMBER);
  drawCenteredText("HOME WI-FI", 165, 2, C_WHITE);
  drawCenteredText("PLEASE WAIT", 220, 2, C_MUTED);
}

void drawConnectedScreen() {
  board.display().fillScreen(C_BLACK);
  drawCenteredText("CONNECTED", 28, 3, C_GREEN);
  drawCenteredText(WiFi.localIP().toString(), 88, 3, C_WHITE);
  drawCenteredText("WT32-CLOCK.LOCAL", 145, 2, C_CYAN);
  drawCenteredText("NTP SYNC", 205, 3, C_AMBER);
  drawCenteredText(zoneDisplayLabel(), 260, 2, C_MUTED);
}

void drawSevenSegmentDigit(int digit, int x, int y, int w, int h, int t,
                           uint16_t onColor, uint16_t offColor) {
  static const uint8_t masks[10] = {
      0x3F, 0x06, 0x5B, 0x4F, 0x66,
      0x6D, 0x7D, 0x07, 0x7F, 0x6F};
  if (digit < 0 || digit > 9) digit = 0;
  const uint8_t mask = masks[digit];

  auto segment = [&](uint8_t bit, int sx, int sy, int sw, int sh) {
    board.display().fillRect(sx, sy, sw, sh,
                             (mask & bit) ? onColor : offColor);
  };

  const int half = h / 2;
  segment(0x01, x + t, y, w - 2 * t, t);                  // A
  segment(0x02, x + w - t, y + t, t, half - t);          // B
  segment(0x04, x + w - t, y + half, t, half - t);       // C
  segment(0x08, x + t, y + h - t, w - 2 * t, t);         // D
  segment(0x10, x, y + half, t, half - t);               // E
  segment(0x20, x, y + t, t, half - t);                  // F
  segment(0x40, x + t, y + half - t / 2, w - 2 * t, t);  // G
}

void drawClockColon(int x, int y, int size, uint16_t color) {
  board.display().fillRect(x, y + 32, size, size, color);
  board.display().fillRect(x, y + 76, size, size, color);
}

void drawClock() {
  time_t now = time(nullptr);
  struct tm local{};
  if (now < 1700000000 || !localtime_r(&now, &local)) return;

  timeSynced = true;
  if (local.tm_sec == lastRenderedSecond) return;
  lastRenderedSecond = local.tm_sec;

  board.display().fillRect(0, 0, 480, 188, C_BLACK);

  const int mainY = 30;
  const int mainW = 60;
  const int mainH = 116;
  const int mainT = 10;
  drawSevenSegmentDigit(local.tm_hour / 10, 34, mainY, mainW, mainH, mainT, C_AMBER, C_AMBER_DIM);
  drawSevenSegmentDigit(local.tm_hour % 10, 102, mainY, mainW, mainH, mainT, C_AMBER, C_AMBER_DIM);
  drawClockColon(174, mainY, 10, (local.tm_sec % 2) ? C_AMBER_DIM : C_AMBER);
  drawSevenSegmentDigit(local.tm_min / 10, 194, mainY, mainW, mainH, mainT, C_AMBER, C_AMBER_DIM);
  drawSevenSegmentDigit(local.tm_min % 10, 262, mainY, mainW, mainH, mainT, C_AMBER, C_AMBER_DIM);

  drawSevenSegmentDigit(local.tm_sec / 10, 372, 65, 28, 56, 5, C_CYAN, rgb565(2, 22, 28));
  drawSevenSegmentDigit(local.tm_sec % 10, 408, 65, 28, 56, 5, C_CYAN, rgb565(2, 22, 28));

  static const char *months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  static const char *weekdays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  char dateLine[32];
  snprintf(dateLine, sizeof(dateLine), "%s %02d %s %04d",
           weekdays[local.tm_wday], local.tm_mday, months[local.tm_mon], local.tm_year + 1900);

  board.display().fillRect(0, 188, 480, 132, C_BLACK);
  drawCenteredText(dateLine, 199, 3, C_WHITE);
  drawCenteredText(zoneDisplayLabel(), 244, 2, C_GREEN);

  String networkLine = "IP " + WiFi.localIP().toString();
  drawCenteredText(networkLine, 282, 1, WiFi.status() == WL_CONNECTED ? C_CYAN : C_RED);
  drawCenteredText("SETUP: WT32-CLOCK.LOCAL", 300, 1, C_MUTED);
}

void refreshNetworkScan() {
  scanCount = 0;
  int found = WiFi.scanNetworks(false, true);
  if (found <= 0) {
    WiFi.scanDelete();
    return;
  }

  for (int i = 0; i < found && scanCount < kMaxScanNetworks; ++i) {
    String candidate = WiFi.SSID(i);
    if (candidate.length() == 0) continue;

    bool duplicate = false;
    for (int j = 0; j < scanCount; ++j) {
      if (scanSSID[j] == candidate) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) scanSSID[scanCount++] = candidate;
  }
  WiFi.scanDelete();
}

String networkOptionsHtml() {
  String html;
  bool savedSeen = false;
  for (int i = 0; i < scanCount; ++i) {
    const bool selected = scanSSID[i] == savedSSID;
    if (selected) savedSeen = true;
    html += F("<option value=\"");
    html += htmlEscape(scanSSID[i]);
    html += F("\"");
    if (selected) html += F(" selected");
    html += F(">");
    html += htmlEscape(scanSSID[i]);
    html += F("</option>");
  }

  if (savedSSID.length() && !savedSeen) {
    html += F("<option value=\"");
    html += htmlEscape(savedSSID);
    html += F("\" selected>");
    html += htmlEscape(savedSSID);
    html += F(" (saved)</option>");
  }

  html += F("<option value=\"__manual__\">Other / hidden network...</option>");
  return html;
}

String zoneOptionsHtml() {
  String html;
  for (size_t i = 0; i < kZoneCount; ++i) {
    html += F("<option value=\"");
    html += kZones[i].id;
    html += F("\"");
    if (savedZone == kZones[i].id) html += F(" selected");
    html += F(">");
    html += kZones[i].webLabel;
    html += F("</option>");
  }
  return html;
}

String setupPageHtml() {
  String page;
  page.reserve(8500);
  page += F("<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">");
  page += F("<title>WT32 Retro Clock Setup</title><style>");
  page += F("body{font-family:system-ui,sans-serif;background:#0b1020;color:#e8eef8;margin:0;padding:20px}main{max-width:680px;margin:auto}h1{font-size:1.7rem}section{background:#121a2c;border:1px solid #2c3b59;border-radius:16px;padding:18px;margin:14px 0}label{display:block;margin:12px 0 6px}select,input,button{box-sizing:border-box;width:100%;padding:12px;border-radius:10px;border:1px solid #40506f;background:#0a1222;color:#fff;font:inherit}button{background:#1f6feb;border:0;font-weight:700;margin-top:18px;cursor:pointer}.muted{color:#9aacbf}.ok{color:#6ee7a8}.warn{color:#ffcc66}code{background:#070b13;padding:3px 6px;border-radius:6px}a{color:#7dd3fc}.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}@media(max-width:600px){.grid{grid-template-columns:1fr}}</style></head><body><main>");
  page += F("<h1>WT32-SC01-PLUS Retro Clock</h1>");

  if (WiFi.status() == WL_CONNECTED) {
    page += F("<section><div class=\"ok\"><b>Clock is online</b></div><p>Home-network IP: <code>");
    page += WiFi.localIP().toString();
    page += F("</code></p><p>Local name: <a href=\"http://wt32-clock.local/\">http://wt32-clock.local/</a></p><p class=\"muted\">You can return to this page later to change Wi-Fi or timezone.</p></section>");
  } else if (portalActive) {
    page += F("<section><b>Setup access point</b><p>SSID: <code>");
    page += htmlEscape(apSSID);
    page += F("</code><br>Password: <code>WT32SETUP</code><br>Setup IP: <code>192.168.4.1</code></p></section>");
  }

  page += F("<form method=\"post\" action=\"/save\"><section><h2>1. Home Wi-Fi</h2><label for=\"ssid\">Network</label><select id=\"ssid\" name=\"ssid\" required>");
  page += networkOptionsHtml();
  page += F("</select><div id=\"manualBox\" style=\"display:none\"><label for=\"manual\">SSID for hidden/other network</label><input id=\"manual\" name=\"manual_ssid\" autocomplete=\"off\"></div><label for=\"pass\">Password</label><input id=\"pass\" name=\"pass\" type=\"password\" autocomplete=\"current-password\" placeholder=\"Leave blank to keep saved password for the same SSID\"><p><a href=\"/rescan\">Rescan Wi-Fi networks</a></p></section>");

  page += F("<section><h2>2. City / timezone</h2><label for=\"zone\">Preset</label><select id=\"zone\" name=\"zone\" required><option value=\"\"");
  if (savedZone.length() == 0) page += F(" selected");
  page += F(" disabled>Choose city or timezone...</option>");
  page += zoneOptionsHtml();
  page += F("</select><label for=\"custom_tz\">Custom POSIX TZ (advanced)</label><input id=\"custom_tz\" name=\"custom_tz\" autocomplete=\"off\" placeholder=\"Example: UTC0\" value=\"");
  if (savedZone == "custom") page += htmlEscape(savedTZ);
  page += F("\"><p class=\"muted\">Leave this blank when using a city preset. DST-capable presets use POSIX timezone rules.</p></section>");

  page += F("<section><h2>3. Save</h2><p>The board will connect to your home Wi-Fi, obtain time from NTP and start the seven-segment clock.</p><button type=\"submit\">Save and connect</button></section></form>");
  page += F("<script>const s=document.getElementById('ssid'),m=document.getElementById('manualBox');function f(){m.style.display=s.value==='__manual__'?'block':'none'}s.addEventListener('change',f);f();</script>");
  page += F("</main></body></html>");
  return page;
}

String progressPageHtml() {
  String page;
  page.reserve(5000);
  page += F("<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Connecting</title><style>body{font-family:system-ui,sans-serif;background:#0b1020;color:#e8eef8;padding:24px}main{max-width:620px;margin:auto;background:#121a2c;border:1px solid #2c3b59;border-radius:16px;padding:22px}code{background:#070b13;padding:3px 6px;border-radius:6px}.ok{color:#6ee7a8}.warn{color:#ffcc66}a{color:#7dd3fc}</style></head><body><main><h1>Settings saved</h1><p id=\"state\" class=\"warn\">Connecting the clock to your home Wi-Fi...</p><div id=\"next\"></div><script>async function poll(){try{const r=await fetch('/status',{cache:'no-store'});const s=await r.json();if(s.connected){document.getElementById('state').className='ok';document.getElementById('state').textContent='Connected. NTP is starting.';document.getElementById('next').innerHTML='<h2>Next step</h2><p>Reconnect this phone/PC to your home Wi-Fi, then open:</p><p><a href=\"http://wt32-clock.local/\">http://wt32-clock.local/</a></p><p>or <code>http://'+s.ip+'/</code></p><p>The same IP is shown on the WT32 display.</p>';return}if(s.failed){document.getElementById('state').textContent='Connection failed. Check the password and return to setup.';document.getElementById('next').innerHTML='<p><a href=\"/\">Back to setup</a></p>';return}}catch(e){}setTimeout(poll,1000)}poll();</script></main></body></html>");
  return page;
}

void redirectToPortal() {
  String target = F("http://192.168.4.1/");
  server.sendHeader("Location", target, true);
  server.send(302, "text/plain", "");
}

void startServerIfNeeded() {
  if (!serverStarted) {
    server.begin();
    serverStarted = true;
    Serial.println("[WEB] HTTP server started on port 80");
  }
}

void configureRoutes() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html; charset=utf-8", setupPageHtml());
  });

  server.on("/rescan", HTTP_GET, []() {
    refreshNetworkScan();
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.on("/status", HTTP_GET, []() {
    String json = F("{\"connected\":");
    json += (WiFi.status() == WL_CONNECTED) ? F("true") : F("false");
    json += F(",\"pending\":");
    json += pendingConnect ? F("true") : F("false");
    json += F(",\"failed\":");
    json += connectFailed ? F("true") : F("false");
    json += F(",\"timeSynced\":");
    json += timeSynced ? F("true") : F("false");
    json += F(",\"ip\":\"");
    json += (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : String("");
    json += F("\"}");
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
  });

  server.on("/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    const String manualSSID = server.arg("manual_ssid");
    if (ssid == "__manual__") ssid = manualSSID;
    ssid.trim();

    String password = server.arg("pass");
    String zone = server.arg("zone");
    String customTZ = server.arg("custom_tz");
    customTZ.trim();

    if (ssid.length() == 0) {
      server.send(400, "text/plain", "Wi-Fi SSID is required");
      return;
    }

    const ZonePreset *preset = findZone(zone);
    if (!preset) {
      server.send(400, "text/plain", "Choose a valid city/timezone");
      return;
    }

    String resolvedTZ;
    if (zone == "custom") {
      if (customTZ.length() == 0) {
        server.send(400, "text/plain", "Custom POSIX TZ is required for Custom mode");
        return;
      }
      resolvedTZ = customTZ;
    } else {
      resolvedTZ = preset->tz;
    }

    if (ssid == savedSSID && password.length() == 0) password = savedPassword;

    saveSettings(ssid, password, zone, resolvedTZ);
    pendingConnect = true;
    stationReady = false;
    timeConfigured = false;
    timeSynced = false;
    connectFailed = false;
    connectStartedMs = millis();
    apStopAtMs = 0;

    Serial.printf("[SETUP] Connecting to saved SSID: %s\n", savedSSID.c_str());
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    drawConnectingScreen();

    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html; charset=utf-8", progressPageHtml());
  });

  server.on("/generate_204", HTTP_ANY, []() { redirectToPortal(); });
  server.on("/gen_204", HTTP_ANY, []() { redirectToPortal(); });
  server.on("/hotspot-detect.html", HTTP_ANY, []() {
    server.send(200, "text/html; charset=utf-8", setupPageHtml());
  });
  server.on("/connecttest.txt", HTTP_ANY, []() { redirectToPortal(); });
  server.on("/ncsi.txt", HTTP_ANY, []() { redirectToPortal(); });

  server.onNotFound([]() {
    if (portalActive) redirectToPortal();
    else {
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
    }
  });
}

void configureTimeAndMdns() {
  if (!stationReady || WiFi.status() != WL_CONNECTED) return;

  if (!timeConfigured) {
    const String tz = savedTZ.length() ? savedTZ : String("UTC0");
    configTzTime(tz.c_str(), kNtp1, kNtp2);
    timeConfigured = true;
    Serial.printf("[NTP] Configured TZ: %s\n", tz.c_str());
  }

  MDNS.end();
  if (MDNS.begin(kHostname)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[MDNS] http://wt32-clock.local/");
  } else {
    Serial.println("[MDNS] responder start failed");
  }
}

void stationConnected(bool keepPortalTemporarily) {
  stationReady = true;
  pendingConnect = false;
  connectFailed = false;
  lastReconnectMs = millis();

  Serial.printf("[WIFI] Connected. IP=%s\n", WiFi.localIP().toString().c_str());
  configureTimeAndMdns();
  drawConnectedScreen();
  startServerIfNeeded();

  if (keepPortalTemporarily && portalActive) {
    apStopAtMs = millis() + kApGraceAfterConnectMs;
    Serial.println("[AP] Keeping setup AP for 60 seconds so browser can show the destination address");
  }
}

void startPortal(const String &status = "") {
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(apSSID.c_str(), kApPassword)) {
    Serial.println("[AP] Failed to start SoftAP");
    drawSetupScreen("AP FAILED");
    return;
  }

  portalActive = true;
  pendingConnect = false;
  connectFailed = false;
  apStopAtMs = 0;

  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.printf("[AP] SSID=%s PASS=%s IP=%s\n", apSSID.c_str(), kApPassword,
                WiFi.softAPIP().toString().c_str());

  drawSetupScreen(status);
  refreshNetworkScan();
  startServerIfNeeded();
}

bool tryStoredWiFi() {
  if (savedSSID.length() == 0) return false;

  drawConnectingScreen();
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  const uint32_t started = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - started < 12000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    stationConnected(false);
    return true;
  }

  WiFi.disconnect(false, false);
  return false;
}

void serviceNetworkState() {
  if (pendingConnect) {
    if (WiFi.status() == WL_CONNECTED) {
      stationConnected(true);
    } else if (millis() - connectStartedMs >= kConnectTimeoutMs) {
      pendingConnect = false;
      connectFailed = true;
      stationReady = false;
      WiFi.disconnect(false, false);
      Serial.println("[WIFI] Setup connection timed out; staying in AP mode");
      drawSetupScreen("CONNECT FAILED");
    }
  }

  if (portalActive && apStopAtMs != 0 && static_cast<int32_t>(millis() - apStopAtMs) >= 0) {
    Serial.println("[AP] Grace period complete; switching to station-only mode");
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    portalActive = false;
    apStopAtMs = 0;
  }

  if (stationReady && WiFi.status() != WL_CONNECTED && !portalActive && !pendingConnect) {
    if (millis() - lastReconnectMs >= kReconnectIntervalMs) {
      lastReconnectMs = millis();
      Serial.println("[WIFI] Reconnect attempt");
      WiFi.reconnect();
    }
  } else if (stationReady && WiFi.status() == WL_CONNECTED) {
    lastReconnectMs = millis();
  }
}

void serviceClock() {
  if (!stationReady || !timeConfigured) return;
  if (millis() - lastClockRefreshMs < kClockRefreshMs) return;
  lastClockRefreshMs = millis();

  const time_t now = time(nullptr);
  if (now < 1700000000) {
    timeSynced = false;
    return;
  }

  drawClock();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  if (!board.begin()) {
    Serial.println("[FAIL] WT32-SC01-PLUS display/backlight init failed");
    while (true) delay(1000);
  }

  board.backlight().set(72);
  board.display().fillScreen(C_BLACK);

  buildApSSID();
  loadSettings();
  configureRoutes();

  // Arduino-ESP32 requires the hostname to be set before Wi-Fi is started.
  WiFi.setHostname(kHostname);
  WiFi.persistent(false);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS / 12_RetroClock");
  Serial.println(" Web AP setup -> home Wi-Fi -> NTP -> seven-segment clock");
  Serial.println("============================================================");

  if (!tryStoredWiFi()) {
    startPortal(savedSSID.length() ? "WI-FI FAILED" : "");
  }
}

void loop() {
  if (portalActive) dnsServer.processNextRequest();
  if (serverStarted) server.handleClient();

  serviceNetworkState();
  serviceClock();
  delay(2);
}
