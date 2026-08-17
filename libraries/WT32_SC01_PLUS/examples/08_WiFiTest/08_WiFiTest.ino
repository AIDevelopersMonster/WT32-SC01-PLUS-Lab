#include <WiFi.h>

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define WT32_WIFI_LOCAL_SECRETS 1
#else
static const char *WIFI_TEST_SSID = "";
static const char *WIFI_TEST_PASSWORD = "";
#define WT32_WIFI_LOCAL_SECRETS 0
#endif

static constexpr uint32_t CONNECT_TIMEOUT_MS = 20000;
static constexpr uint8_t RECONNECT_CYCLES = 3;
static constexpr const char *DNS_TEST_HOST = "example.com";
static constexpr const char *HTTP_TEST_HOST = "example.com";
static constexpr uint16_t HTTP_TEST_PORT = 80;

namespace {

void printDivider() {
  Serial.println("------------------------------------------------------------");
}

void printNetworkInfo() {
  Serial.printf("[INFO] SSID       : %s\n", WiFi.SSID().c_str());
  Serial.printf("[INFO] BSSID      : %s\n", WiFi.BSSIDstr().c_str());
  Serial.printf("[INFO] Channel    : %d\n", WiFi.channel());
  Serial.printf("[INFO] RSSI       : %d dBm\n", WiFi.RSSI());
  Serial.printf("[INFO] STA MAC    : %s\n", WiFi.macAddress().c_str());
  Serial.printf("[INFO] IPv4       : %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[INFO] Gateway    : %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("[INFO] Subnet     : %s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("[INFO] DNS 0      : %s\n", WiFi.dnsIP(0).toString().c_str());
  Serial.printf("[INFO] DNS 1      : %s\n", WiFi.dnsIP(1).toString().c_str());
}

bool waitForConnection(uint32_t timeoutMs) {
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

bool runScan() {
  Serial.println("[SCAN] Starting active Wi-Fi scan...");
  const int count = WiFi.scanNetworks(false, true);

  if (count < 0) {
    Serial.printf("[FAIL] Wi-Fi scan returned %d\n", count);
    return false;
  }

  Serial.printf("[PASS] Wi-Fi scan completed: %d network(s) found\n", count);

  for (int i = 0; i < count; ++i) {
    Serial.printf("  %02d  RSSI=%4d dBm  CH=%2d  AUTH=%d  BSSID=%s  SSID=%s%s\n",
                  i + 1,
                  WiFi.RSSI(i),
                  WiFi.channel(i),
                  static_cast<int>(WiFi.encryptionType(i)),
                  WiFi.BSSIDstr(i).c_str(),
                  WiFi.SSID(i).c_str(),
                  WiFi.SSID(i).length() == 0 ? " [hidden]" : "");
  }

  WiFi.scanDelete();
  return true;
}

bool connectToConfiguredNetwork() {
  Serial.printf("[CONNECT] Connecting to SSID: %s\n", WIFI_TEST_SSID);
  WiFi.begin(WIFI_TEST_SSID, WIFI_TEST_PASSWORD);

  if (!waitForConnection(CONNECT_TIMEOUT_MS)) {
    Serial.printf("[FAIL] Connection timeout, status=%d\n", static_cast<int>(WiFi.status()));
    return false;
  }

  Serial.println("[PASS] Associated and DHCP configuration acquired");
  printNetworkInfo();
  return true;
}

bool runDnsTest() {
  IPAddress resolved;
  Serial.printf("[DNS] Resolving %s ...\n", DNS_TEST_HOST);

  if (!WiFi.hostByName(DNS_TEST_HOST, resolved)) {
    Serial.println("[FAIL] DNS resolution failed");
    return false;
  }

  Serial.printf("[PASS] DNS %s -> %s\n",
                DNS_TEST_HOST,
                resolved.toString().c_str());
  return true;
}

bool runTcpHttpTest() {
  WiFiClient client;
  Serial.printf("[TCP] Connecting to %s:%u ...\n", HTTP_TEST_HOST, HTTP_TEST_PORT);

  if (!client.connect(HTTP_TEST_HOST, HTTP_TEST_PORT)) {
    Serial.println("[FAIL] TCP connection failed");
    return false;
  }

  Serial.println("[PASS] TCP connection established");

  client.printf("HEAD / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", HTTP_TEST_HOST);

  const uint32_t start = millis();
  String firstLine;
  while (millis() - start < 5000) {
    if (client.available()) {
      firstLine = client.readStringUntil('\n');
      firstLine.trim();
      break;
    }
    delay(10);
  }

  client.stop();

  if (firstLine.length() == 0) {
    Serial.println("[FAIL] HTTP response timeout / empty response");
    return false;
  }

  Serial.printf("[PASS] HTTP response: %s\n", firstLine.c_str());
  return firstLine.startsWith("HTTP/");
}

bool runReconnectTest() {
  Serial.printf("[RECONNECT] Running %u disconnect/reconnect cycle(s)\n", RECONNECT_CYCLES);

  for (uint8_t cycle = 1; cycle <= RECONNECT_CYCLES; ++cycle) {
    WiFi.disconnect(false, false);
    delay(500);

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[FAIL] cycle %u: station did not disconnect\n", cycle);
      return false;
    }

    WiFi.begin(WIFI_TEST_SSID, WIFI_TEST_PASSWORD);
    if (!waitForConnection(CONNECT_TIMEOUT_MS)) {
      Serial.printf("[FAIL] cycle %u: reconnect timeout\n", cycle);
      return false;
    }

    Serial.printf("[PASS] reconnect cycle %u/%u  RSSI=%d dBm  IP=%s\n",
                  cycle,
                  RECONNECT_CYCLES,
                  WiFi.RSSI(),
                  WiFi.localIP().toString().c_str());
  }

  return true;
}

} // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 08_WiFiTest");
  Serial.println(" Wi-Fi radio + scan + optional infrastructure validation");
  Serial.println("============================================================");
  Serial.printf("[INFO] Local secrets header: %s\n",
                WT32_WIFI_LOCAL_SECRETS ? "wifi_secrets.h loaded" : "not present (scan-only mode)");

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);
  delay(300);

  Serial.printf("[INFO] STA MAC: %s\n", WiFi.macAddress().c_str());
  printDivider();

  if (!runScan()) {
    Serial.println();
    Serial.println("RESULT = FAIL (Wi-Fi scan)");
    return;
  }

  printDivider();

  if (strlen(WIFI_TEST_SSID) == 0) {
    Serial.println("[INFO] WIFI_TEST_SSID is blank: infrastructure tests skipped.");
    Serial.println("[INFO] Copy wifi_secrets.example.h to wifi_secrets.h and fill it locally.");
    Serial.println();
    Serial.println("============================================================");
    Serial.println(" WIFI RADIO / SCAN PHYSICAL PASS CANDIDATE");
    Serial.println(" Full association/DHCP/DNS/TCP/reconnect validation: PENDING");
    Serial.println("============================================================");
    return;
  }

  if (!connectToConfiguredNetwork()) {
    Serial.println("RESULT = FAIL (association/DHCP)");
    return;
  }

  printDivider();
  if (!runDnsTest()) {
    Serial.println("RESULT = FAIL (DNS)");
    return;
  }

  printDivider();
  if (!runTcpHttpTest()) {
    Serial.println("RESULT = FAIL (TCP/HTTP)");
    return;
  }

  printDivider();
  if (!runReconnectTest()) {
    Serial.println("RESULT = FAIL (reconnect)");
    return;
  }

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WIFI TEST PHYSICAL PASS CANDIDATE");
  Serial.println(" Scan + association + DHCP + DNS + TCP/HTTP + reconnect passed.");
  Serial.println("============================================================");
}

void loop() {
  delay(1000);
}
