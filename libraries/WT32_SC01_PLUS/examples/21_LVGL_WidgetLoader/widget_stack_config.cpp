#include <Arduino.h>

// WebServer + JSON validation + LVGL are serviced from Arduino loopTask.
// Keep the same validated safety margin introduced after the OTA stack failure.
size_t getArduinoLoopTaskStackSize(void) {
    return 16U * 1024U;
}
