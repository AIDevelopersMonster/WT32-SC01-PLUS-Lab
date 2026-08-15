#pragma once

#include <Arduino.h>
#include "WT32_SC01_PLUS_Pins.h"

class WT32_SC01_PLUS_Display {
public:
    bool begin();
    void fillScreen(uint16_t rgb565);
    void drawTestPattern();
    int width() const { return wt32sc01plus::pins::LCD_WIDTH; }
    int height() const { return wt32sc01plus::pins::LCD_HEIGHT; }

private:
    void *bus_ = nullptr;
    void *io_ = nullptr;
    uint16_t *lineBuffer_ = nullptr;
    bool ready_ = false;

    bool command(uint8_t cmd, const uint8_t *data = nullptr, size_t len = 0);
    bool setWindow(int x0, int y0, int x1, int y1);
    bool pushPixels(const uint16_t *pixels, size_t count);
};

class WT32_SC01_PLUS_Backlight {
public:
    bool begin();
    void set(uint8_t percent);
    uint8_t value() const { return percent_; }

private:
    uint8_t percent_ = 0;
    bool ready_ = false;
};

class WT32_SC01_PLUS {
public:
    bool begin();
    WT32_SC01_PLUS_Display &display() { return display_; }
    WT32_SC01_PLUS_Backlight &backlight() { return backlight_; }

    const char *profileName() const;
    void printBoardInfo(Stream &out = Serial) const;

private:
    WT32_SC01_PLUS_Display display_;
    WT32_SC01_PLUS_Backlight backlight_;
};
