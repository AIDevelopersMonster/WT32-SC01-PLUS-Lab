#pragma once
#include "TestFramework.h"

inline TestStatus runTouchTest(TestContext &ctx) {
  ctx.out.println("[02] TOUCH quick check: touch the screen within 15 s");
  if (!ctx.board.touch().begin()) {
    ctx.out.println("[FAIL] touch init failed");
    return TestStatus::Fail;
  }
  const uint32_t start = millis();
  WT32_SC01_PLUS_TouchPoint p;
  while (millis() - start < 15000) {
    if (ctx.board.touch().read(p) && p.touched) {
      ctx.out.printf("[PASS] touch x=%u y=%u raw=%u,%u\n", p.x, p.y, p.rawX, p.rawY);
      return TestStatus::Pass;
    }
    delay(20);
  }
  ctx.out.println("[FAIL] no touch event within timeout");
  return TestStatus::Fail;
}
