#pragma once

#include <WT32_SC01_PLUS.h>

#include "SdQualificationEngine.h"

class SdQualificationUi {
public:
  explicit SdQualificationUi(WT32_SC01_PLUS &board) : board_(board) {}

  bool begin();
  void showReady(const SdCardInfo &info);
  bool waitForStart();
  bool confirmErase();
  void showProgress(const char *phase,
                    uint8_t pattern,
                    uint8_t stage,
                    uint8_t totalStages,
                    uint64_t doneSectors,
                    uint64_t totalSectors,
                    double rateMiBPerSec,
                    uint32_t etaSeconds);
  void showPass(const SdCardInfo &info);
  void showFailure(const SdQualFailure &failure);

private:
  static constexpr uint16_t BLACK = 0x0000;
  static constexpr uint16_t WHITE = 0xFFFF;
  static constexpr uint16_t RED = 0xF800;
  static constexpr uint16_t GREEN = 0x07E0;
  static constexpr uint16_t BLUE = 0x001F;
  static constexpr uint16_t CYAN = 0x07FF;
  static constexpr uint16_t YELLOW = 0xFFE0;
  static constexpr uint16_t GRAY = 0x8410;
  static constexpr uint16_t DARK_GRAY = 0x4208;

  struct Rect {
    int x;
    int y;
    int w;
    int h;
  };

  void drawChar(int x, int y, char c, uint16_t color, int scale = 2);
  void drawText(int x, int y, const char *text, uint16_t color, int scale = 2);
  void drawCentered(int y, const char *text, uint16_t color, int scale = 2);
  void drawButton(const Rect &r, const char *label, uint16_t fill, uint16_t text);
  void drawProgressBar(int x, int y, int w, int h, uint8_t percent);
  bool waitForTap(const Rect &r);
  bool released(uint32_t timeoutMs = 3000);
  bool pointInRect(uint16_t x, uint16_t y, const Rect &r) const;
  const uint8_t *glyph(char c) const;

  WT32_SC01_PLUS &board_;
  uint8_t lastStage_ = 0xFF;
  uint8_t lastPercent_ = 0xFF;
};
