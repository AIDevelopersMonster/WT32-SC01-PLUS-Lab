#pragma once

// WT32-SC01-PLUS hardware profile
// Panlee / ZX3D50CE08S-V15-USRC / marking 230208
//
// IMPORTANT: These pins are validated only for the named specimen/profile.
// Do not assume they apply to every board sold as WT32-SC01-PLUS.

#define WT32_SC01_PLUS_BOARD_PANLEE_V15_230208 1

namespace wt32sc01plus {
namespace pins {

static constexpr int LCD_BL  = 45;
static constexpr int LCD_RST = 4;
static constexpr int LCD_DC  = 0;
static constexpr int LCD_WR  = 47;
static constexpr int LCD_CS  = -1;
static constexpr int LCD_TE  = 48; // documented/observed, unused by v0.1

static constexpr int LCD_D0 = 9;
static constexpr int LCD_D1 = 46;
static constexpr int LCD_D2 = 3;
static constexpr int LCD_D3 = 8;
static constexpr int LCD_D4 = 18;
static constexpr int LCD_D5 = 17;
static constexpr int LCD_D6 = 16;
static constexpr int LCD_D7 = 15;

static constexpr int LCD_WIDTH  = 480;
static constexpr int LCD_HEIGHT = 320;
static constexpr int LCD_PCLK_HZ = 10000000;

// PHYSICALLY VALIDATED on Panlee V15 / 230208 by ESP-IDF raw/orientation tests.
// GPIO4 is shared with LCD reset.
static constexpr int TOUCH_SDA = 6;
static constexpr int TOUCH_SCL = 5;
static constexpr int TOUCH_INT = 7;
static constexpr int TOUCH_RST = 4;
static constexpr uint8_t TOUCH_ADDR = 0x38;
static constexpr uint32_t TOUCH_I2C_HZ = 400000;

// PHYSICALLY VALIDATED on Panlee V15 / 230208 by independent ESP-IDF SDSPI read test.
static constexpr int SD_SCK  = 39;
static constexpr int SD_MOSI = 40;
static constexpr int SD_MISO = 38;
static constexpr int SD_CS   = 41;
static constexpr uint32_t SD_SPI_HZ = 10000000;

// PHYSICALLY VALIDATED on Panlee V15 / 230208 by high-power audio test.
static constexpr int AUDIO_LRCK = 35;
static constexpr int AUDIO_BCLK = 36;
static constexpr int AUDIO_DOUT = 37;

// FACTORY-FIRMWARE RECOVERED on Panlee V15 / 230208; physical Arduino validation pending.
// Factory firmware configures UART1 at 115200 8N1 in ESP-IDF RS485 half-duplex mode.
static constexpr int RS485_TX  = 42;
static constexpr int RS485_RX  = 1;
static constexpr int RS485_RTS = 2;
static constexpr uint32_t RS485_BAUD = 115200;

} // namespace pins
} // namespace wt32sc01plus
