#pragma once
#include "TestFramework.h"
#include <SPI.h>
#include <SD.h>

inline TestStatus runStorageTest(TestContext &ctx) {
  ctx.out.println("[03] STORAGE quick read-only check");
  static SPIClass sdSpi(FSPI);
  sdSpi.begin(wt32sc01plus::pins::SD_SCK, wt32sc01plus::pins::SD_MISO,
              wt32sc01plus::pins::SD_MOSI, wt32sc01plus::pins::SD_CS);
  if (!SD.begin(wt32sc01plus::pins::SD_CS, sdSpi, wt32sc01plus::pins::SD_SPI_HZ)) {
    ctx.out.println("[FAIL] SD mount failed");
    return TestStatus::Fail;
  }
  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    ctx.out.println("[FAIL] root directory open failed");
    SD.end();
    return TestStatus::Fail;
  }
  root.close();
  ctx.out.printf("[PASS] SD mounted, cardSize=%llu MiB\n", SD.cardSize() / (1024ULL * 1024ULL));
  SD.end();
  return TestStatus::Pass;
}
