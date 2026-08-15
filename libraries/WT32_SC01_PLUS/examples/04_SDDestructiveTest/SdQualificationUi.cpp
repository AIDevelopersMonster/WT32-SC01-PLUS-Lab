#include "SdQualificationUi.h"

#include <cstdio>
#include <cstring>

namespace {
constexpr uint8_t FONT[][5] = {
  {0x3E,0x51,0x49,0x45,0x3E}, // 0
  {0x00,0x42,0x7F,0x40,0x00}, // 1
  {0x42,0x61,0x51,0x49,0x46}, // 2
  {0x21,0x41,0x45,0x4B,0x31}, // 3
  {0x18,0x14,0x12,0x7F,0x10}, // 4
  {0x27,0x45,0x45,0x45,0x39}, // 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 6
  {0x01,0x71,0x09,0x05,0x03}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x06,0x49,0x49,0x29,0x1E}, // 9
  {0x7E,0x11,0x11,0x11,0x7E}, // A
  {0x7F,0x49,0x49,0x49,0x36}, // B
  {0x3E,0x41,0x41,0x41,0x22}, // C
  {0x7F,0x41,0x41,0x22,0x1C}, // D
  {0x7F,0x49,0x49,0x49,0x41}, // E
  {0x7F,0x09,0x09,0x09,0x01}, // F
  {0x3E,0x41,0x49,0x49,0x7A}, // G
  {0x7F,0x08,0x08,0x08,0x7F}, // H
  {0x00,0x41,0x7F,0x41,0x00}, // I
  {0x20,0x40,0x41,0x3F,0x01}, // J
  {0x7F,0x08,0x14,0x22,0x41}, // K
  {0x7F,0x40,0x40,0x40,0x40}, // L
  {0x7F,0x02,0x0C,0x02,0x7F}, // M
  {0x7F,0x04,0x08,0x10,0x7F}, // N
  {0x3E,0x41,0x41,0x41,0x3E}, // O
  {0x7F,0x09,0x09,0x09,0x06}, // P
  {0x3E,0x41,0x51,0x21,0x5E}, // Q
  {0x7F,0x09,0x19,0x29,0x46}, // R
  {0x46,0x49,0x49,0x49,0x31}, // S
  {0x01,0x01,0x7F,0x01,0x01}, // T
  {0x3F,0x40,0x40,0x40,0x3F}, // U
  {0x1F,0x20,0x40,0x20,0x1F}, // V
  {0x7F,0x20,0x18,0x20,0x7F}, // W
  {0x63,0x14,0x08,0x14,0x63}, // X
  {0x03,0x04,0x78,0x04,0x03}, // Y
  {0x61,0x51,0x49,0x45,0x43}, // Z
  {0x00,0x00,0x00,0x00,0x00}, // space
  {0x00,0x00,0x5F,0x00,0x00}, // !
  {0x00,0x36,0x36,0x00,0x00}, // :
  {0x40,0x40,0x40,0x40,0x40}, // _
  {0x08,0x08,0x08,0x08,0x08}, // -
  {0x00,0x60,0x60,0x00,0x00}, // .
  {0x20,0x10,0x08,0x04,0x02}, // /
  {0x23,0x13,0x08,0x64,0x62}, // %
};

constexpr int START_X = 145;
constexpr int START_Y = 230;
constexpr int START_W = 190;
constexpr int START_H = 60;
constexpr int YES_X = 75;
constexpr int NO_X = 275;
constexpr int CONFIRM_Y = 235;
constexpr int CONFIRM_W = 130;
constexpr int CONFIRM_H = 55;
}  // namespace

const uint8_t *SdQualificationUi::glyph(char c) const {
  if (c >= '0' && c <= '9') return FONT[c - '0'];
  if (c >= 'A' && c <= 'Z') return FONT[10 + c - 'A'];
  switch (c) {
    case ' ': return FONT[36];
    case '!': return FONT[37];
    case ':': return FONT[38];
    case '_': return FONT[39];
    case '-': return FONT[40];
    case '.': return FONT[41];
    case '/': return FONT[42];
    case '%': return FONT[43];
    default: return FONT[36];
  }
}

void SdQualificationUi::drawChar(int x, int y, char c, uint16_t color, int scale) {
  const uint8_t *g = glyph(c);
  for (int col = 0; col < 5; ++col) {
    const uint8_t bits = g[col];
    for (int row = 0; row < 7; ++row) {
      if (bits & (1U << row)) {
        board_.display().fillRect(x + col * scale, y + row * scale, scale, scale, color);
      }
    }
  }
}

void SdQualificationUi::drawText(int x, int y, const char *text, uint16_t color, int scale) {
  if (!text) return;
  while (*text) {
    char c = *text++;
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    drawChar(x, y, c, color, scale);
    x += 6 * scale;
  }
}

void SdQualificationUi::drawCentered(int y, const char *text, uint16_t color, int scale) {
  const int w = static_cast<int>(strlen(text)) * 6 * scale;
  drawText((board_.display().width() - w) / 2, y, text, color, scale);
}

void SdQualificationUi::drawButton(const Rect &r, const char *label, uint16_t fill, uint16_t text) {
  board_.display().fillRect(r.x, r.y, r.w, r.h, fill);
  board_.display().fillRect(r.x + 3, r.y + 3, r.w - 6, r.h - 6, BLACK);
  board_.display().fillRect(r.x + 6, r.y + 6, r.w - 12, r.h - 12, fill);
  const int scale = 3;
  const int tw = static_cast<int>(strlen(label)) * 6 * scale;
  const int tx = r.x + (r.w - tw) / 2;
  const int ty = r.y + (r.h - 7 * scale) / 2;
  drawText(tx, ty, label, text, scale);
}

void SdQualificationUi::drawProgressBar(int x, int y, int w, int h, uint8_t percent) {
  board_.display().fillRect(x, y, w, h, DARK_GRAY);
  board_.display().fillRect(x + 3, y + 3, w - 6, h - 6, BLACK);
  const int inner = w - 10;
  const int filled = (inner * percent) / 100;
  if (filled > 0) board_.display().fillRect(x + 5, y + 5, filled, h - 10, GREEN);
}

bool SdQualificationUi::pointInRect(uint16_t x, uint16_t y, const Rect &r) const {
  return x >= r.x && x < static_cast<uint16_t>(r.x + r.w) &&
         y >= r.y && y < static_cast<uint16_t>(r.y + r.h);
}

bool SdQualificationUi::released(uint32_t timeoutMs) {
  const uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    WT32_SC01_PLUS_TouchPoint points[2];
    uint8_t count = 0;
    if (!board_.touch().readPoints(points, 2, count)) return false;
    if (count == 0) return true;
    delay(20);
  }
  return false;
}

bool SdQualificationUi::waitForTap(const Rect &r) {
  released();
  for (;;) {
    WT32_SC01_PLUS_TouchPoint points[2];
    uint8_t count = 0;
    if (!board_.touch().readPoints(points, 2, count)) return false;
    for (uint8_t i = 0; i < count; ++i) {
      if (pointInRect(points[i].x, points[i].y, r)) {
        delay(80);
        released();
        return true;
      }
    }
    delay(20);
  }
}

bool SdQualificationUi::begin() {
  if (!board_.begin()) return false;
  delay(250);
  if (!board_.touch().begin()) return false;
  board_.backlight().set(100);
  return true;
}

void SdQualificationUi::showReady(const SdCardInfo &info) {
  char line[64];
  board_.display().fillScreen(BLACK);
  drawCentered(18, "SD FULL QUALIFICATION", CYAN, 3);
  drawCentered(62, "BLOCK MODE 32 KIB", WHITE, 2);

  snprintf(line, sizeof(line), "CARD: %s", info.name[0] ? info.name : "SD");
  drawText(30, 105, line, WHITE, 2);
  snprintf(line, sizeof(line), "SIZE: %llu MIB",
           static_cast<unsigned long long>(info.bytes / (1024ULL * 1024ULL)));
  drawText(30, 132, line, WHITE, 2);
  snprintf(line, sizeof(line), "SECTORS: %llu",
           static_cast<unsigned long long>(info.sectors));
  drawText(30, 159, line, WHITE, 2);
  drawText(30, 188, "00 / AA / 55 + VERIFY + FAT", YELLOW, 2);

  drawButton({START_X, START_Y, START_W, START_H}, "START", GREEN, BLACK);
}

bool SdQualificationUi::waitForStart() {
  return waitForTap({START_X, START_Y, START_W, START_H});
}

bool SdQualificationUi::confirmErase() {
  board_.display().fillScreen(BLACK);
  drawCentered(25, "WARNING", RED, 4);
  drawCentered(80, "ALL SD DATA WILL BE ERASED", WHITE, 2);
  drawCentered(112, "FULL MEDIA WRITE TEST", YELLOW, 2);
  drawCentered(145, "CONTINUE?", WHITE, 3);

  const Rect yes{YES_X, CONFIRM_Y, CONFIRM_W, CONFIRM_H};
  const Rect no{NO_X, CONFIRM_Y, CONFIRM_W, CONFIRM_H};
  drawButton(yes, "ERASE", RED, WHITE);
  drawButton(no, "CANCEL", GRAY, BLACK);

  released();
  for (;;) {
    WT32_SC01_PLUS_TouchPoint points[2];
    uint8_t count = 0;
    if (!board_.touch().readPoints(points, 2, count)) return false;
    for (uint8_t i = 0; i < count; ++i) {
      if (pointInRect(points[i].x, points[i].y, yes)) {
        delay(80);
        released();
        return true;
      }
      if (pointInRect(points[i].x, points[i].y, no)) {
        delay(80);
        released();
        return false;
      }
    }
    delay(20);
  }
}

void SdQualificationUi::showProgress(const char *phase,
                                     uint8_t pattern,
                                     uint8_t stage,
                                     uint8_t totalStages,
                                     uint64_t doneSectors,
                                     uint64_t totalSectors,
                                     double rateMiBPerSec,
                                     uint32_t etaSeconds) {
  const uint8_t percent = totalSectors
      ? static_cast<uint8_t>((doneSectors * 100ULL) / totalSectors)
      : 0;

  if (stage != lastStage_) {
    board_.display().fillScreen(BLACK);
    lastStage_ = stage;
    lastPercent_ = 0xFF;
  }
  if (percent == lastPercent_ && stage != 7) return;
  lastPercent_ = percent;

  char line[64];
  board_.display().fillRect(0, 0, 480, 320, BLACK);
  drawCentered(12, "SD FULL QUALIFICATION", CYAN, 3);

  snprintf(line, sizeof(line), "STAGE %u / %u", stage, totalStages);
  drawCentered(58, line, WHITE, 2);

  if (pattern) {
    snprintf(line, sizeof(line), "%s 0X%02X", phase, pattern);
  } else {
    snprintf(line, sizeof(line), "%s", phase);
  }
  drawCentered(90, line, YELLOW, 3);

  snprintf(line, sizeof(line), "%u%%", percent);
  drawCentered(135, line, GREEN, 4);
  drawProgressBar(35, 180, 410, 36, percent);

  snprintf(line, sizeof(line), "RATE %.2F MIB/S", rateMiBPerSec);
  drawText(45, 235, line, WHITE, 2);
  snprintf(line, sizeof(line), "ETA %02LU:%02LU:%02LU",
           static_cast<unsigned long>(etaSeconds / 3600UL),
           static_cast<unsigned long>((etaSeconds / 60UL) % 60UL),
           static_cast<unsigned long>(etaSeconds % 60UL));
  drawText(45, 262, line, WHITE, 2);

  snprintf(line, sizeof(line), "LBA %llu / %llu",
           static_cast<unsigned long long>(doneSectors),
           static_cast<unsigned long long>(totalSectors));
  drawText(45, 289, line, GRAY, 1);
}

void SdQualificationUi::showPass(const SdCardInfo &info) {
  char line[64];
  board_.display().fillScreen(GREEN);
  drawCentered(30, "PASS", BLACK, 6);
  drawCentered(105, "00 AA 55 VERIFIED", BLACK, 3);
  drawCentered(145, "FAT RESTORED", BLACK, 3);
  drawCentered(185, "CARD EMPTY AND READY", BLACK, 2);
  snprintf(line, sizeof(line), "%llu MIB / %llu SECTORS",
           static_cast<unsigned long long>(info.bytes / (1024ULL * 1024ULL)),
           static_cast<unsigned long long>(info.sectors));
  drawCentered(230, line, BLACK, 2);
}

void SdQualificationUi::showFailure(const SdQualFailure &failure) {
  char line[64];
  board_.display().fillScreen(RED);
  drawCentered(15, "SD TEST FAIL", WHITE, 4);
  drawText(20, 80, failure.phase ? failure.phase : "UNKNOWN", WHITE, 2);

  snprintf(line, sizeof(line), "PATTERN 0X%02X", failure.pattern);
  drawText(20, 115, line, WHITE, 2);
  snprintf(line, sizeof(line), "LBA %llu", static_cast<unsigned long long>(failure.lba));
  drawText(20, 145, line, WHITE, 2);
  snprintf(line, sizeof(line), "OFFSET %u", failure.offset);
  drawText(20, 175, line, WHITE, 2);
  snprintf(line, sizeof(line), "EXPECTED %02X ACTUAL %02X", failure.expected, failure.actual);
  drawText(20, 205, line, WHITE, 2);
  snprintf(line, sizeof(line), "ERROR %d", failure.error);
  drawText(20, 235, line, WHITE, 2);
}
