#include "WT32_SC01_PLUS.h"

#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>

bool WT32_SC01_PLUS_Backlight::begin() {
    if (ready_) return true;

    pinMode(wt32sc01plus::pins::LCD_BL, OUTPUT);
    digitalWrite(wt32sc01plus::pins::LCD_BL, LOW);

    if (!ledcAttach(wt32sc01plus::pins::LCD_BL, 12000, 8)) {
        return false;
    }

    ready_ = true;
    set(0);
    return true;
}

void WT32_SC01_PLUS_Backlight::set(uint8_t percent) {
    if (!ready_) return;
    if (percent > 100) percent = 100;
    percent_ = percent;
    const uint32_t duty = (static_cast<uint32_t>(percent) * 255U + 50U) / 100U;
    ledcWrite(wt32sc01plus::pins::LCD_BL, duty);
}

bool WT32_SC01_PLUS::begin() {
    if (!backlight_.begin()) return false;
    if (!display_.begin()) return false;
    backlight_.set(100);
    return true;
}

const char *WT32_SC01_PLUS::profileName() const {
    return "Panlee ZX3D50CE08S-V15-USRC / 230208";
}

void WT32_SC01_PLUS::printBoardInfo(Stream &out) const {
    esp_chip_info_t chip{};
    esp_chip_info(&chip);

    uint32_t flashSize = 0;
    esp_flash_get_size(nullptr, &flashSize);

    out.println();
    out.println("WT32-SC01-PLUS BoardInfo");
    out.println("------------------------");
    out.print("Profile : "); out.println(profileName());
    out.print("Cores   : "); out.println(chip.cores);
    out.print("Revision: "); out.println(chip.revision);
    out.print("Flash   : "); out.print(flashSize / (1024U * 1024U)); out.println(" MiB");
    out.print("PSRAM   : "); out.print(ESP.getPsramSize() / (1024U * 1024U)); out.println(" MiB");
    out.print("Free PSRAM: "); out.print(ESP.getFreePsram() / 1024U); out.println(" KiB");
    out.println("LCD     : ST7796-class, 480x320, 8-bit I80 @ 10 MHz");
    out.println("WARNING : pin profile is specimen-specific, not universal WT32-SC01-PLUS data.");
    out.println();
}
