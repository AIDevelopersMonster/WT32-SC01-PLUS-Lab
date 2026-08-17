#pragma once
#include "TestFramework.h"
#include <BLEDevice.h>
#include <BLEScan.h>

inline TestStatus runBleTest(TestContext &ctx) {
  ctx.out.println("[07] BLE quick scan check");
  static bool initialized = false;
  if (!initialized) {
    if (!BLEDevice::init("WT32-SC01-PLUS-CONSOLE")) {
      ctx.out.println("[FAIL] BLE init failed");
      return TestStatus::Fail;
    }
    initialized = true;
  }
  BLEScan *scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  BLEScanResults *results = scan->start(4, false);
  if (!results) {
    ctx.out.println("[FAIL] BLE scan returned null");
    return TestStatus::Fail;
  }
  const int count = results->getCount();
  scan->clearResults();
  ctx.out.printf("[PASS] BLE scan found %d device(s)\n", count);
  return count > 0 ? TestStatus::Pass : TestStatus::Fail;
}
