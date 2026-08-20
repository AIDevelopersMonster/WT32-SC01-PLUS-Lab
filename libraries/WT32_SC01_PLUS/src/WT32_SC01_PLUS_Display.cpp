#include "WT32_SC01_PLUS.h"

#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_lcd_io_i80.h>
#include <esp_lcd_panel_io.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {
constexpr size_t kLinePixels = wt32sc01plus::pins::LCD_WIDTH;
constexpr size_t kLineBytes = kLinePixels * sizeof(uint16_t);

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8U) << 8) |
                                 ((g & 0xFCU) << 3) |
                                 (b >> 3));
}

static bool onColorTransferDone(esp_lcd_panel_io_handle_t,
                                esp_lcd_panel_io_event_data_t *,
                                void *userCtx) {
    auto semaphore = static_cast<SemaphoreHandle_t>(userCtx);
    if (!semaphore) return false;

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(semaphore, &higherPriorityTaskWoken);
    return higherPriorityTaskWoken == pdTRUE;
}
}

bool WT32_SC01_PLUS_Display::command(uint8_t cmd, const uint8_t *data, size_t len) {
    if (!io_) return false;
    auto io = static_cast<esp_lcd_panel_io_handle_t>(io_);
    return esp_lcd_panel_io_tx_param(io, cmd, data, len) == ESP_OK;
}

bool WT32_SC01_PLUS_Display::setWindow(int x0, int y0, int x1, int y1) {
    const uint8_t cols[] = {
        static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0),
        static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1)
    };
    const uint8_t rows[] = {
        static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0),
        static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1)
    };
    return command(0x2A, cols, sizeof(cols)) && command(0x2B, rows, sizeof(rows));
}

bool WT32_SC01_PLUS_Display::pushPixels(const uint16_t *pixels, size_t count) {
    if (!io_ || !transferDone_ || !pixels || count == 0) return false;

    auto io = static_cast<esp_lcd_panel_io_handle_t>(io_);
    auto semaphore = static_cast<SemaphoreHandle_t>(transferDone_);

    if (esp_lcd_panel_io_tx_color(io, 0x2C, pixels, count * sizeof(uint16_t)) != ESP_OK) {
        return false;
    }

    return xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE;
}

bool WT32_SC01_PLUS_Display::begin() {
    if (ready_) return true;

    pinMode(wt32sc01plus::pins::LCD_RST, OUTPUT);
    digitalWrite(wt32sc01plus::pins::LCD_RST, HIGH);
    delay(10);
    digitalWrite(wt32sc01plus::pins::LCD_RST, LOW);
    delay(20);
    digitalWrite(wt32sc01plus::pins::LCD_RST, HIGH);
    delay(120);

    esp_lcd_i80_bus_config_t busConfig{};
    busConfig.clk_src = LCD_CLK_SRC_DEFAULT;
    busConfig.dc_gpio_num = wt32sc01plus::pins::LCD_DC;
    busConfig.wr_gpio_num = wt32sc01plus::pins::LCD_WR;
    busConfig.data_gpio_nums[0] = wt32sc01plus::pins::LCD_D0;
    busConfig.data_gpio_nums[1] = wt32sc01plus::pins::LCD_D1;
    busConfig.data_gpio_nums[2] = wt32sc01plus::pins::LCD_D2;
    busConfig.data_gpio_nums[3] = wt32sc01plus::pins::LCD_D3;
    busConfig.data_gpio_nums[4] = wt32sc01plus::pins::LCD_D4;
    busConfig.data_gpio_nums[5] = wt32sc01plus::pins::LCD_D5;
    busConfig.data_gpio_nums[6] = wt32sc01plus::pins::LCD_D6;
    busConfig.data_gpio_nums[7] = wt32sc01plus::pins::LCD_D7;
    busConfig.bus_width = 8;
    busConfig.max_transfer_bytes = kLineBytes;

    esp_lcd_i80_bus_handle_t bus = nullptr;
    if (esp_lcd_new_i80_bus(&busConfig, &bus) != ESP_OK) return false;
    bus_ = bus;

    auto semaphore = xSemaphoreCreateBinary();
    if (!semaphore) return false;
    transferDone_ = semaphore;

    esp_lcd_panel_io_i80_config_t ioConfig{};
    ioConfig.cs_gpio_num = wt32sc01plus::pins::LCD_CS;
    ioConfig.pclk_hz = wt32sc01plus::pins::LCD_PCLK_HZ;
    ioConfig.trans_queue_depth = 1;
    ioConfig.on_color_trans_done = onColorTransferDone;
    ioConfig.user_ctx = transferDone_;
    ioConfig.dc_levels.dc_idle_level = 0;
    ioConfig.dc_levels.dc_cmd_level = 0;
    ioConfig.dc_levels.dc_dummy_level = 0;
    ioConfig.dc_levels.dc_data_level = 1;
    ioConfig.lcd_cmd_bits = 8;
    ioConfig.lcd_param_bits = 8;
    ioConfig.flags.swap_color_bytes = 1;

    esp_lcd_panel_io_handle_t io = nullptr;
    if (esp_lcd_new_panel_io_i80(bus, &ioConfig, &io) != ESP_OK) return false;
    io_ = io;

    lineBuffer_ = static_cast<uint16_t *>(heap_caps_malloc(kLineBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (!lineBuffer_) return false;

    if (!command(0x01)) return false;
    delay(150);
    if (!command(0x11)) return false;
    delay(120);

    const uint8_t pixelFormat = 0x55;
    if (!command(0x3A, &pixelFormat, 1)) return false;

    const uint8_t madctl = 0x28;
    if (!command(0x36, &madctl, 1)) return false;

    if (!command(0x21)) return false;
    if (!command(0x29)) return false;
    delay(50);

    ready_ = true;
    fillScreen(rgb565(0, 0, 0));
    return true;
}

void WT32_SC01_PLUS_Display::fillScreen(uint16_t color) {
    if (!ready_ || !lineBuffer_) return;

    for (size_t x = 0; x < kLinePixels; ++x) lineBuffer_[x] = color;

    for (int y = 0; y < height(); ++y) {
        if (!setWindow(0, y, width() - 1, y)) return;
        if (!pushPixels(lineBuffer_, kLinePixels)) return;
    }

    command(0x00);
}

void WT32_SC01_PLUS_Display::fillRect(int x, int y, int w, int h, uint16_t color) {
    if (!ready_ || !lineBuffer_ || w <= 0 || h <= 0) return;

    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    if (x1 >= width()) x1 = width() - 1;
    if (y1 >= height()) y1 = height() - 1;
    if (x0 > x1 || y0 > y1) return;

    const size_t count = static_cast<size_t>(x1 - x0 + 1);
    for (size_t i = 0; i < count; ++i) lineBuffer_[i] = color;

    for (int yy = y0; yy <= y1; ++yy) {
        if (!setWindow(x0, yy, x1, yy)) return;
        if (!pushPixels(lineBuffer_, count)) return;
    }
    command(0x00);
}

bool WT32_SC01_PLUS_Display::drawRGB565(int x, int y, int w, int h, const uint16_t *pixels) {
    if (!ready_ || !pixels || w <= 0 || h <= 0) return false;
    if (x < 0 || y < 0 || x + w > width() || y + h > height()) return false;
    if (w > static_cast<int>(kLinePixels)) return false;

    for (int row = 0; row < h; ++row) {
        if (!setWindow(x, y + row, x + w - 1, y + row)) return false;
        if (!pushPixels(pixels + static_cast<size_t>(row) * static_cast<size_t>(w),
                        static_cast<size_t>(w))) {
            return false;
        }
    }

    command(0x00);
    return true;
}

void WT32_SC01_PLUS_Display::drawTestPattern() {
    if (!ready_ || !lineBuffer_) return;

    for (int y = 0; y < height(); ++y) {
        for (int x = 0; x < width(); ++x) {
            uint16_t c;
            if (y < height() / 3) {
                const int band = (x * 8) / width();
                static const uint16_t bars[8] = {
                    rgb565(255,255,255), rgb565(255,255,0), rgb565(0,255,255), rgb565(0,255,0),
                    rgb565(255,0,255), rgb565(255,0,0), rgb565(0,0,255), rgb565(0,0,0)
                };
                c = bars[band > 7 ? 7 : band];
            } else if (y < (height() * 2) / 3) {
                const uint8_t v = static_cast<uint8_t>((x * 255) / (width() - 1));
                c = rgb565(v, v, v);
            } else {
                const bool border = x < 4 || x >= width() - 4 || y < (height() * 2) / 3 + 4 || y >= height() - 4;
                const bool cross = abs(x - width() / 2) <= 2 || abs(y - (height() * 5) / 6) <= 2;
                const bool grid = (x % 40 == 0) || (y % 40 == 0);
                c = border ? rgb565(0,255,255) : cross ? rgb565(255,255,0) : grid ? rgb565(80,80,80) : rgb565(0,0,0);
            }
            lineBuffer_[x] = c;
        }
        if (!setWindow(0, y, width() - 1, y)) return;
        if (!pushPixels(lineBuffer_, kLinePixels)) return;
    }

    command(0x00);
}