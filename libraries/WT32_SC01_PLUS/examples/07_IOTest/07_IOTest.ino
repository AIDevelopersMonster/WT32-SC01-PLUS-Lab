#include <WT32_SC01_PLUS.h>

namespace {
constexpr int kPins[] = {
  wt32sc01plus::pins::EXT_IO0,
  wt32sc01plus::pins::EXT_IO1,
  wt32sc01plus::pins::EXT_IO2,
  wt32sc01plus::pins::EXT_IO3,
  wt32sc01plus::pins::EXT_IO4,
  wt32sc01plus::pins::EXT_IO5,
};

constexpr size_t kPinCount = sizeof(kPins) / sizeof(kPins[0]);
bool seen[kPinCount] = {false, false, false, false, false, false};

uint8_t readMask() {
  uint8_t mask = 0;
  for (size_t i = 0; i < kPinCount; ++i) {
    if (digitalRead(kPins[i])) {
      mask |= static_cast<uint8_t>(1u << i);
    }
  }
  return mask;
}

bool isOneHot(uint8_t value) {
  return value != 0 && (value & static_cast<uint8_t>(value - 1)) == 0;
}

int indexFromMask(uint8_t mask) {
  for (size_t i = 0; i < kPinCount; ++i) {
    if (mask == (1u << i)) return static_cast<int>(i);
  }
  return -1;
}

void printMask(uint8_t mask) {
  Serial.print("mask=");
  for (int i = static_cast<int>(kPinCount) - 1; i >= 0; --i) {
    Serial.print((mask >> i) & 1u);
  }
}

bool allSeen() {
  for (bool value : seen) {
    if (!value) return false;
  }
  return true;
}

void printStatus() {
  Serial.print("[STATUS]");
  for (size_t i = 0; i < kPinCount; ++i) {
    Serial.printf(" GPIO%d=%s", kPins[i], seen[i] ? "PASS" : "WAIT");
  }
  Serial.println();
}
}

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 07_IOTest");
  Serial.println(" SAFE INPUT-ONLY external IO validation");
  Serial.println("============================================================");
  Serial.println("Factory-recovered IO pins: GPIO10,11,12,13,14,21");
  Serial.println("Mode: INPUT_PULLDOWN on all six pins");
  Serial.println("Apply ONLY 3.3 V to ONE target pin at a time.");
  Serial.println("NEVER apply 5 V to ESP32-S3 GPIO.");
  Serial.println("The test requires a strict one-hot input state.");
  Serial.println();

  for (int pin : kPins) {
    pinMode(pin, INPUT_PULLDOWN);
  }

  printStatus();
  Serial.println("[READY] Touch 3.3 V to GPIO10 first, then continue through GPIO21.");
}

void loop() {
  static uint8_t lastMask = 0xFF;
  static uint32_t stableSince = 0;
  static uint8_t candidateMask = 0;
  static bool finished = false;

  if (finished) {
    delay(1000);
    return;
  }

  const uint8_t mask = readMask();
  if (mask != candidateMask) {
    candidateMask = mask;
    stableSince = millis();
  }

  if (mask != lastMask) {
    Serial.print("[INPUT] ");
    printMask(mask);
    Serial.println();
    lastMask = mask;
  }

  if (millis() - stableSince < 120) {
    delay(10);
    return;
  }

  if (mask == 0) {
    delay(20);
    return;
  }

  if (!isOneHot(mask)) {
    Serial.print("[REJECT] more than one input is HIGH: ");
    printMask(mask);
    Serial.println(" -- remove voltage and try again");
    delay(300);
    return;
  }

  const int index = indexFromMask(mask);
  if (index < 0) {
    delay(20);
    return;
  }

  if (!seen[index]) {
    seen[index] = true;
    Serial.printf("[PASS] GPIO%d observed HIGH as a stable one-hot input\n", kPins[index]);
    printStatus();
  }

  if (allSeen()) {
    finished = true;
    Serial.println();
    Serial.println("============================================================");
    Serial.println(" EXTERNAL IO INPUT TEST PHYSICAL PASS CANDIDATE");
    Serial.println(" GPIO10/11/12/13/14/21 all observed as stable one-hot inputs.");
    Serial.println(" Input-only test: output-drive capability is NOT claimed.");
    Serial.println("============================================================");
  }

  delay(100);
}
