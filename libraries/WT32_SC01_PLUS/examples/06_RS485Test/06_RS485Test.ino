#include <WT32_SC01_PLUS.h>
#include <HardwareSerial.h>
#include <driver/uart.h>

namespace {
HardwareSerial rs485(1);

constexpr uint32_t BAUD = wt32sc01plus::pins::RS485_BAUD;
constexpr uint32_t PING_INTERVAL_MS = 2000;
constexpr uint32_t RESPONSE_TIMEOUT_MS = 1200;

uint32_t sequence = 0;
uint32_t sentCount = 0;
uint32_t passCount = 0;
uint32_t timeoutCount = 0;
uint32_t rxLineCount = 0;
uint32_t lastPingMs = 0;
uint32_t awaitingSinceMs = 0;
bool awaitingPong = false;
String rxLine;

void printBanner() {
  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 06_RS485Test");
  Serial.println(" UART1 + onboard RS485 transceiver physical validation");
  Serial.println("============================================================");
  Serial.printf("UART      : UART1, %lu 8N1\n", static_cast<unsigned long>(BAUD));
  Serial.printf("TX        : GPIO%d\n", wt32sc01plus::pins::RS485_TX);
  Serial.printf("RX        : GPIO%d\n", wt32sc01plus::pins::RS485_RX);
  Serial.printf("RTS/DE    : GPIO%d\n", wt32sc01plus::pins::RS485_RTS);
  Serial.println("Mode      : ESP-IDF UART_MODE_RS485_HALF_DUPLEX");
  Serial.println("Factory evidence: TX=42 RX=1 RTS=2, physical Arduino PASS pending");
  Serial.println();
  Serial.println("External peer protocol:");
  Serial.println("  board -> peer : WT32-RS485 PING <n>");
  Serial.println("  peer  -> board: WT32-RS485 PONG <n>");
  Serial.println("A PASS requires the matching sequence number to return over RS485.");
  Serial.println();
}

void printStatus() {
  Serial.println("---------------- RS485 STATUS ----------------");
  Serial.printf("sent=%lu pass=%lu timeout=%lu rx_lines=%lu awaiting=%s\n",
                static_cast<unsigned long>(sentCount),
                static_cast<unsigned long>(passCount),
                static_cast<unsigned long>(timeoutCount),
                static_cast<unsigned long>(rxLineCount),
                awaitingPong ? "yes" : "no");
  Serial.println("------------------------------------------------");
}

void sendPing() {
  ++sequence;
  ++sentCount;

  char frame[64];
  const int len = snprintf(frame, sizeof(frame), "WT32-RS485 PING %lu\r\n",
                           static_cast<unsigned long>(sequence));

  while (rs485.available()) rs485.read();
  rs485.write(reinterpret_cast<const uint8_t *>(frame), static_cast<size_t>(len));
  rs485.flush();

  awaitingPong = true;
  awaitingSinceMs = millis();
  lastPingMs = millis();
  Serial.printf("[TX] %s", frame);
}

void processLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  ++rxLineCount;
  Serial.printf("[RX] %s\n", line.c_str());

  const String expected = String("WT32-RS485 PONG ") + String(sequence);
  if (awaitingPong && line == expected) {
    awaitingPong = false;
    ++passCount;
    const uint32_t rtt = millis() - awaitingSinceMs;
    Serial.printf("[PASS] RS485 round-trip sequence=%lu RTT=%lu ms\n",
                  static_cast<unsigned long>(sequence),
                  static_cast<unsigned long>(rtt));

    if (passCount == 1) {
      Serial.println("[PHYSICAL PASS CANDIDATE] TX + RTS direction + A/B + RX path completed.");
    }
  }
}

void pollRs485() {
  while (rs485.available()) {
    const int v = rs485.read();
    if (v < 0) break;
    const char c = static_cast<char>(v);

    if (c == '\r') continue;
    if (c == '\n') {
      processLine(rxLine);
      rxLine = "";
    } else if (rxLine.length() < 120) {
      rxLine += c;
    } else {
      Serial.println("[WARN] RX line overflow; discarded");
      rxLine = "";
    }
  }
}

void pollUsbCommands() {
  if (!Serial.available()) return;

  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toLowerCase();

  if (command == "ping") {
    if (!awaitingPong) sendPing();
  } else if (command == "status") {
    printStatus();
  } else if (command == "help") {
    Serial.println("Commands: ping | status | help");
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);
  printBanner();

  rs485.begin(BAUD,
              SERIAL_8N1,
              wt32sc01plus::pins::RS485_RX,
              wt32sc01plus::pins::RS485_TX);

  esp_err_t err = uart_set_pin(UART_NUM_1,
                               wt32sc01plus::pins::RS485_TX,
                               wt32sc01plus::pins::RS485_RX,
                               wt32sc01plus::pins::RS485_RTS,
                               UART_PIN_NO_CHANGE);
  if (err != ESP_OK) {
    Serial.printf("[FAIL] uart_set_pin error=%d\n", static_cast<int>(err));
    return;
  }

  err = uart_set_mode(UART_NUM_1, UART_MODE_RS485_HALF_DUPLEX);
  if (err != ESP_OK) {
    Serial.printf("[FAIL] uart_set_mode error=%d\n", static_cast<int>(err));
    return;
  }

  rs485.setTimeout(100);
  Serial.println("[PASS] UART1 configured for RS485 half-duplex.");
  Serial.println("Connect an external RS485 peer and return matching PONG frames.");
  Serial.println("Automatic PING begins in 2 seconds. Type 'status' for counters.");
  lastPingMs = millis();
}

void loop() {
  pollRs485();
  pollUsbCommands();

  const uint32_t now = millis();
  if (awaitingPong && now - awaitingSinceMs >= RESPONSE_TIMEOUT_MS) {
    ++timeoutCount;
    awaitingPong = false;
    Serial.printf("[TIMEOUT] no matching PONG for sequence=%lu\n",
                  static_cast<unsigned long>(sequence));
  }

  if (!awaitingPong && now - lastPingMs >= PING_INTERVAL_MS) {
    sendPing();
  }

  delay(2);
}
