#include <WT32_SC01_PLUS.h>

WT32_SC01_PLUS board;

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8U) << 8) | ((g & 0xFCU) << 3) | (b >> 3));
}

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("WT32-SC01-PLUS Arduino BSP / 01_DisplayTest");
  Serial.println("Profile: Panlee ZX3D50CE08S-V15-USRC / 230208");
  Serial.println("Expected LCD: 480x320 landscape, ST7796-class, 8-bit I80");

  if (!board.begin()) {
    Serial.println("[FAIL] board.begin()");
    while (true) delay(1000);
  }

  board.printBoardInfo();
  Serial.println("[PASS] display initialized");
  Serial.println("Starting visual sequence...");
}

void loop() {
  Serial.println("BLACK");
  board.display().fillScreen(rgb565(0, 0, 0));
  delay(1200);

  Serial.println("WHITE");
  board.display().fillScreen(rgb565(255, 255, 255));
  delay(1200);

  Serial.println("RED");
  board.display().fillScreen(rgb565(255, 0, 0));
  delay(1200);

  Serial.println("GREEN");
  board.display().fillScreen(rgb565(0, 255, 0));
  delay(1200);

  Serial.println("BLUE");
  board.display().fillScreen(rgb565(0, 0, 255));
  delay(1200);

  Serial.println("COLOR / GRAYSCALE / GEOMETRY");
  board.display().drawTestPattern();
  delay(4000);

  Serial.println("BACKLIGHT PWM 100 -> 10 -> 50 -> 100");
  board.backlight().set(10);
  delay(800);
  board.backlight().set(50);
  delay(800);
  board.backlight().set(100);
  delay(1500);

  Serial.println("Cycle complete. Physical PASS requires operator visual confirmation.");
}
