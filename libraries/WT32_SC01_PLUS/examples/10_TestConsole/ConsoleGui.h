#pragma once
#include "TestFramework.h"

static constexpr uint16_t C_BLACK = 0x0000;
static constexpr uint16_t C_BLUE  = 0x001F;
static constexpr uint16_t C_GREEN = 0x07E0;
static constexpr uint16_t C_RED   = 0xF800;
static constexpr uint16_t C_YELLOW= 0xFFE0;
static constexpr uint16_t C_GRAY  = 0x7BEF;
static constexpr uint16_t C_WHITE = 0xFFFF;

inline uint16_t tileColor(TestStatus s) {
  switch (s) {
    case TestStatus::Pass: return C_GREEN;
    case TestStatus::Fail: return C_RED;
    case TestStatus::Pending: return C_YELLOW;
    case TestStatus::Running: return C_BLUE;
    default: return C_GRAY;
  }
}

inline void drawSeg(WT32_SC01_PLUS_Display &d, int x, int y, int s, bool on, bool vertical) {
  if (!on) return;
  if (vertical) d.fillRect(x, y, s, s * 3, C_WHITE);
  else d.fillRect(x, y, s * 3, s, C_WHITE);
}

inline void drawDigit(WT32_SC01_PLUS_Display &d, int x, int y, uint8_t digit) {
  static const uint8_t seg[10] = {
    0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011,
    0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011
  };
  const uint8_t m = seg[digit % 10];
  const int s = 7;
  drawSeg(d, x + s,     y,         s, m & 0b1000000, false); // a
  drawSeg(d, x + s * 4, y + s,     s, m & 0b0100000, true);  // b
  drawSeg(d, x + s * 4, y + s * 5, s, m & 0b0010000, true);  // c
  drawSeg(d, x + s,     y + s * 8, s, m & 0b0001000, false); // d
  drawSeg(d, x,         y + s * 5, s, m & 0b0000100, true);  // e
  drawSeg(d, x,         y + s,     s, m & 0b0000010, true);  // f
  drawSeg(d, x + s,     y + s * 4, s, m & 0b0000001, false); // g
}

inline void drawMenu(TestContext &ctx, TestEntry *tests, size_t count) {
  auto &d = ctx.board.display();
  d.fillScreen(C_BLACK);
  const int gap = 8;
  const int w = 149;
  const int h = 94;
  for (size_t i = 0; i < count && i < 9; ++i) {
    const int col = i % 3;
    const int row = i / 3;
    const int x = gap + col * (w + gap);
    const int y = gap + row * (h + gap);
    d.fillRect(x, y, w, h, tileColor(tests[i].status));
    drawDigit(d, x + 57, y + 8, tests[i].id);
  }
}

inline int guiPollSelection(TestContext &ctx) {
  static uint32_t lastTouch = 0;
  WT32_SC01_PLUS_TouchPoint p;
  if (!ctx.board.touch().read(p) || !p.touched) return 0;
  if (millis() - lastTouch < 500) return 0;
  lastTouch = millis();

  const int gap = 8;
  const int w = 149;
  const int h = 94;
  const int col = p.x / (w + gap);
  const int row = p.y / (h + gap);
  if (col < 0 || col > 2 || row < 0 || row > 2) return 0;
  const int localX = p.x - (gap + col * (w + gap));
  const int localY = p.y - (gap + row * (h + gap));
  if (localX < 0 || localX >= w || localY < 0 || localY >= h) return 0;
  return row * 3 + col + 1;
}
