#include <Arduino.h>

// Modified by WT32-SC01-PLUS-Lab for Moscow UTC+3 without DST.
#include <ESP32Time.h>
#include "time_manager.h"
#include "../WiFiManager/wifi_manager.h"
#include "../SDManager/sd_manager.h"

// Timezone settings
const int GMT_OFFSET = 3 * 3600; // Moscow: UTC+3
const int DST_OFFSET = 0;        // Moscow does not observe DST

tm timeinfo;
ESP32Time rtc(0); // Initialize RTC with GMT+0
String hour = "00";
String minute = "00";
String date = "2024.04.01";

// Checks if the time is valid
void checkTime() {
    if (!getLocalTime(&timeinfo)) {
        SD_LOG("Failed to obtain time");
        return;
    }
}

// Formats the time and date
void trimTimeDate() {
    char buffer[25]; // Buffer for formatting time and date

    // Format hour
    snprintf(buffer, sizeof(buffer), "%02d", rtc.getHour(true));
    hour = buffer;

    // Format minute
    snprintf(buffer, sizeof(buffer), "%02d", rtc.getMinute());
    minute = buffer;

    // Format date
    snprintf(buffer, sizeof(buffer), "%04d.%02d.%02d", rtc.getYear(), rtc.getMonth() + 1, rtc.getDay());
    date = buffer;
}

// Initializes the time
void InitTime() {
    if (isConnected) {
        SD_LOG("Configuring Moscow time: UTC+3, DST=0");
        configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVERS);
        SD_LOG("Updating time...");
        checkTime();
        SD_LOG("Time & Date synchronized!");
        trimTimeDate();
        rtc.setTimeStruct(timeinfo);
    }
}