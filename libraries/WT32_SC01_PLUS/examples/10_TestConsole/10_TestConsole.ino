#include <Arduino.h>
#include <WT32_SC01_PLUS.h>

#include "TestFramework.h"
#include "test_01_display.h"
#include "test_02_touch.h"
#include "test_03_storage.h"
#include "test_04_audio.h"
#include "test_05_io.h"
#include "test_06_wifi.h"
#include "test_07_ble.h"
#include "test_08_rs485.h"
#include "test_09_system.h"
#include "ConsoleGui.h"

WT32_SC01_PLUS board;
TestContext ctx{board, Serial};

TestEntry tests[] = {
  {1, "Display",  runDisplayTest, TestStatus::Idle},
  {2, "Touch",    runTouchTest,   TestStatus::Idle},
  {3, "SD read",  runStorageTest, TestStatus::Idle},
  {4, "Audio",    runAudioTest,   TestStatus::Idle},
  {5, "Ext IO",   runIoTest,      TestStatus::Idle},
  {6, "Wi-Fi",    runWifiTest,    TestStatus::Idle},
  {7, "BLE",      runBleTest,     TestStatus::Idle},
  {8, "RS485",    runRs485Test,   TestStatus::Pending},
  {9, "System",   runSystemTest,  TestStatus::Idle},
};

static constexpr size_t TEST_COUNT = sizeof(tests) / sizeof(tests[0]);

void printMenu() {
  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 10_TestConsole");
  Serial.println(" Select a test by CLI number or touch the numbered GUI tile");
  Serial.println("============================================================");
  for (size_t i = 0; i < TEST_COUNT; ++i) {
    Serial.printf(" %u  %-12s [%s]\n", tests[i].id, tests[i].name, statusName(tests[i].status));
  }
  Serial.println();
  Serial.println(" CLI: 1..9 = run one | a = all | q = automatic quick suite | ? = menu");
  Serial.println(" GUI: tap numbered tile; green=PASS red=FAIL yellow=PENDING blue=RUN");
  Serial.println(" NOTE: test 05 is interactive and test 08 needs external RS485 hardware.");
}

TestEntry *findTest(uint8_t id) {
  for (size_t i = 0; i < TEST_COUNT; ++i) if (tests[i].id == id) return &tests[i];
  return nullptr;
}

void runTest(uint8_t id) {
  TestEntry *t = findTest(id);
  if (!t) return;
  Serial.println();
  Serial.printf("========== RUN %u: %s ==========\n", t->id, t->name);
  t->status = TestStatus::Running;
  drawMenu(ctx, tests, TEST_COUNT);
  t->status = t->run(ctx);
  Serial.printf("========== RESULT %u: %s = %s ==========\n", t->id, t->name, statusName(t->status));
  drawMenu(ctx, tests, TEST_COUNT);
  printMenu();
}

void runAll(bool quickOnly) {
  Serial.printf("[SUITE] %s\n", quickOnly ? "automatic quick suite" : "full sequential suite");
  for (size_t i = 0; i < TEST_COUNT; ++i) {
    const uint8_t id = tests[i].id;
    if (quickOnly && (id == 2 || id == 5 || id == 8)) continue;
    runTest(id);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1200);

  if (!board.begin()) {
    Serial.println("FATAL: display/backlight initialization failed");
    return;
  }
  if (!board.touch().begin()) {
    Serial.println("WARNING: touch initialization failed; CLI remains available");
  }

  drawMenu(ctx, tests, TEST_COUNT);
  printMenu();
}

void loop() {
  if (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c >= '1' && c <= '9') runTest(static_cast<uint8_t>(c - '0'));
    else if (c == 'a' || c == 'A') runAll(false);
    else if (c == 'q' || c == 'Q') runAll(true);
    else if (c == '?') printMenu();
  }

  if (board.touch().ready()) {
    const int selected = guiPollSelection(ctx);
    if (selected >= 1 && selected <= 9) runTest(static_cast<uint8_t>(selected));
  }

  delay(15);
}
