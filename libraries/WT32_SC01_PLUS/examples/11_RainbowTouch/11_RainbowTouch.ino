#include <WT32_SC01_PLUS.h>

WT32_SC01_PLUS board;

namespace {
constexpr int kBrushRadius = 4;

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8U) << 8) |
                               ((g & 0xFCU) << 3) |
                               (b >> 3));
}

uint16_t colorFromTouch(uint16_t x, uint16_t y) {
  const long maxX = board.display().width() - 1;
  const long maxY = board.display().height() - 1;
  const long maxSum = maxX + maxY;

  const uint8_t r = static_cast<uint8_t>(map(x, 0, maxX, 0, 255));
  const uint8_t g = static_cast<uint8_t>(map(y, 0, maxY, 0, 255));
  const uint8_t b = static_cast<uint8_t>(map(static_cast<long>(x) + y,
                                             0, maxSum, 255, 0));

  return rgb565(r, g, b);
}
}  // namespace

void setup() {
  Serial.begin(115200);

  if (!board.begin()) {
    Serial.println("[FAIL] display/backlight init failed");
    while (true) delay(1000);
  }

  if (!board.touch().begin()) {
    Serial.println("[FAIL] touch init failed");
    while (true) delay(1000);
  }

  board.printBoardInfo(Serial);
  board.backlight().set(80);
  board.display().fillScreen(0x0000);

  Serial.println("[READY] Rainbow Touch Draw");
  Serial.println("Drag a finger over the display to paint.");
}

void loop() {
  WT32_SC01_PLUS_TouchPoint point;

  if (board.touch().read(point) && point.touched) {
    const uint16_t color = colorFromTouch(point.x, point.y);

    board.display().fillRect(static_cast<int>(point.x) - kBrushRadius,
                             static_cast<int>(point.y) - kBrushRadius,
                             kBrushRadius * 2 + 1,
                             kBrushRadius * 2 + 1,
                             color);
  }

  delay(4);
}
