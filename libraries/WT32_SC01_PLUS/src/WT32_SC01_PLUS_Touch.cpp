#include "WT32_SC01_PLUS.h"

#include <Wire.h>

namespace {
constexpr uint8_t REG_TD_STATUS = 0x02;
constexpr uint8_t REG_P1_XH = 0x03;
constexpr uint8_t REG_CHIP_CODE = 0xA0;
constexpr uint8_t REG_FIRMWARE_ID = 0xA6;
constexpr uint8_t REG_FOCALTECH_ID = 0xA8;
constexpr uint8_t FT6336U_CHIP_CODE = 0x02;

bool decodePoint(const uint8_t *raw, WT32_SC01_PLUS_TouchPoint &point) {
    point = WT32_SC01_PLUS_TouchPoint{};
    point.event = (raw[0] >> 6) & 0x03;
    point.rawX = (static_cast<uint16_t>(raw[0] & 0x0F) << 8) | raw[1];
    point.trackId = (raw[2] >> 4) & 0x0F;
    point.rawY = (static_cast<uint16_t>(raw[2] & 0x0F) << 8) | raw[3];

    if (point.rawX > 319 || point.rawY > 479) return false;

    point.x = point.rawY;
    point.y = 319U - point.rawX;
    point.touched = true;
    return true;
}
}

bool WT32_SC01_PLUS_Touch::readReg(uint8_t reg, uint8_t *data, size_t len) {
    if (!data || len == 0) return false;

    Wire1.beginTransmission(wt32sc01plus::pins::TOUCH_ADDR);
    Wire1.write(reg);
    if (Wire1.endTransmission(false) != 0) return false;

    const size_t received = Wire1.requestFrom(
        static_cast<uint8_t>(wt32sc01plus::pins::TOUCH_ADDR), len, true);
    if (received != len) return false;

    for (size_t i = 0; i < len; ++i) {
        if (!Wire1.available()) return false;
        data[i] = static_cast<uint8_t>(Wire1.read());
    }
    return true;
}

bool WT32_SC01_PLUS_Touch::begin() {
    if (ready_) return true;

    pinMode(wt32sc01plus::pins::TOUCH_INT, INPUT);

    if (!Wire1.begin(
            wt32sc01plus::pins::TOUCH_SDA,
            wt32sc01plus::pins::TOUCH_SCL,
            wt32sc01plus::pins::TOUCH_I2C_HZ)) {
        return false;
    }
    Wire1.setTimeOut(50);

    if (!readReg(REG_CHIP_CODE, &chipCode_, 1)) return false;
    if (!readReg(REG_FIRMWARE_ID, &firmwareId_, 1)) return false;
    if (!readReg(REG_FOCALTECH_ID, &focalTechId_, 1)) return false;

    if (chipCode_ != FT6336U_CHIP_CODE) return false;

    ready_ = true;
    return true;
}

bool WT32_SC01_PLUS_Touch::readPoints(WT32_SC01_PLUS_TouchPoint *points,
                                      uint8_t capacity,
                                      uint8_t &count) {
    count = 0;
    if (!ready_ || !points || capacity == 0) return false;

    // Read status plus both FT6336U point slots as one coherent frame:
    // 0x02 TD_STATUS, 0x03..0x08 P1, 0x09..0x0E P2.
    uint8_t frame[13] = {0};
    if (!readReg(REG_TD_STATUS, frame, sizeof(frame))) return false;

    const uint8_t reported = frame[0] & 0x0F;
    if (reported == 0) return true;
    if (reported > 2) return false;

    const uint8_t usable = reported < capacity ? reported : capacity;
    for (uint8_t i = 0; i < usable; ++i) {
        const uint8_t *slot = &frame[1 + static_cast<size_t>(i) * 6U];
        if (!decodePoint(slot, points[count])) continue;
        ++count;
    }

    return true;
}

bool WT32_SC01_PLUS_Touch::read(WT32_SC01_PLUS_TouchPoint &point) {
    WT32_SC01_PLUS_TouchPoint points[2];
    uint8_t count = 0;
    if (!readPoints(points, 2, count)) return false;
    point = WT32_SC01_PLUS_TouchPoint{};
    if (count == 0) return true;
    point = points[0];
    return true;
}

int WT32_SC01_PLUS_Touch::interruptLevel() const {
    return digitalRead(wt32sc01plus::pins::TOUCH_INT);
}
