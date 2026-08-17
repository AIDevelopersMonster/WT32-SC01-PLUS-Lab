#pragma once
#include "TestFramework.h"

inline TestStatus runSystemTest(TestContext &ctx) {
  ctx.out.println("[09] SYSTEM / memory info");
  ctx.board.printBoardInfo(ctx.out);
  ctx.out.printf("Heap free : %u bytes\n", ESP.getFreeHeap());
  ctx.out.printf("PSRAM free: %u bytes\n", ESP.getFreePsram());
  ctx.out.println("[PASS] system information path completed");
  return TestStatus::Pass;
}
