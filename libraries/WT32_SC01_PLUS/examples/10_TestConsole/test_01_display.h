#pragma once
#include "TestFramework.h"

inline TestStatus runDisplayTest(TestContext &ctx) {
  ctx.out.println("[01] DISPLAY quick check");
  ctx.board.display().drawTestPattern();
  delay(1200);
  ctx.board.display().fillScreen(0x0000);
  ctx.out.println("[PASS] LCD draw path completed");
  return TestStatus::Pass;
}
