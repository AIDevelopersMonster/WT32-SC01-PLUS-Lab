#include <Arduino.h>

// Arduino-ESP32 creates setup()/loop() inside loopTask. The core default is
// 8 KiB, which was physically observed to overflow in the 0.1.1 GitHub OTA
// DOWNLOAD & INSTALL path before the first OTA image byte reached app1.
//
// main.cpp in Arduino-ESP32 exposes getArduinoLoopTaskStackSize() as a weak
// symbol specifically so sketches can override the loop task stack budget.
// Keep this in a separate translation unit so the fix is explicit and does
// not depend on Arduino preprocessor ordering in the .ino file.
size_t getArduinoLoopTaskStackSize(void) {
    return 16U * 1024U;
}
