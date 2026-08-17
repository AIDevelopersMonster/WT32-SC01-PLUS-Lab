#pragma once
#include "TestFramework.h"

inline TestStatus runTouchTest(TestContext &ctx) {
  ctx.out.println("[02] TOUCH quick check: release, then touch within 15 s");
  if (!ctx.board.touch().begin()) {
    ctx.out.println("[FAIL] touch init failed");
    return TestStatus::Fail;
  }

  WT32_SC01_PLUS_TouchPoint p;
  const uint32_t releaseStart = millis();
  while (millis() - releaseStart < 3000) {
    if (!ctx.board.touch().read(p) || !p.touched) break;
    delay(20);
  }
  delay(250);

  const uint32_t start = millis();
  while (millis() - start < 15000) {
    if (ctx.board.touch().read(p) && p.touched) {
      ctx.out.printf("[PASS] touch x=%u y=%u raw=%u,%u\n", p.x, p.y, p.rawX, p.rawY);
      return TestStatus::Pass;
    }
    delay(20);
  }
  ctx.out.println("[FAIL] no new touch event within timeout");
  return TestStatus::Fail;
}
