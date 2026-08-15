#pragma once

#include <Arduino.h>
#include "WT32_SC01_PLUS_Pins.h"

class WT32_SC01_PLUS_Display {
public:
    bool begin();
    void fillScreen(uint16_t rgb565);
    void fillRect(int x, int y, int w, int h, uint16_t rgb565);
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

struct WT32_SC01_PLUS_TouchPoint {
    bool touched = false;
    uint16_t rawX = 0;
    uint16_t rawY = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    uint8_t event = 0;
    uint8_t trackId = 0;
};

class WT32_SC01_PLUS_Touch {
public:
    bool begin();
    bool read(WT32_SC01_PLUS_TouchPoint &point);
    bool ready() const { return ready_; }
    uint8_t chipCode() const { return chipCode_; }
    uint8_t firmwareId() const { return firmwareId_; }
    uint8_t focalTechId() const { return focalTechId_; }
    int interruptLevel() const;

private:
    bool ready_ = false;
    uint8_t chipCode_ = 0;
    uint8_t firmwareId_ = 0;
    uint8_t focalTechId_ = 0;

    bool readReg(uint8_t reg, uint8_t *data, size_t len);
};

class WT32_SC01_PLUS_Audio {
public:
    bool begin(uint32_t sampleRate = 44100);
    void end();
    bool tone(uint32_t frequencyHz, uint32_t durationMs, uint8_t amplitudePercent = 10,
              Stream *diagnostics = nullptr);
    bool silence(uint32_t durationMs);
    bool ready() const { return ready_; }
    uint32_t sampleRate() const { return sampleRate_; }

private:
    bool ready_ = false;
    uint32_t sampleRate_ = 0;
};

class WT32_SC01_PLUS {
public:
    bool begin();
    WT32_SC01_PLUS_Display &display() { return display_; }
    WT32_SC01_PLUS_Backlight &backlight() { return backlight_; }
    WT32_SC01_PLUS_Touch &touch() { return touch_; }
    WT32_SC01_PLUS_Audio &audio() { return audio_; }

    const char *profileName() const;
    void printBoardInfo(Stream &out = Serial) const;

private:
    WT32_SC01_PLUS_Display display_;
    WT32_SC01_PLUS_Backlight backlight_;
    WT32_SC01_PLUS_Touch touch_;
    WT32_SC01_PLUS_Audio audio_;
};
