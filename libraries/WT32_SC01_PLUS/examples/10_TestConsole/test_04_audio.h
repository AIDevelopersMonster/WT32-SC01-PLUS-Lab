#pragma once
#include "TestFramework.h"

inline TestStatus runAudioTest(TestContext &ctx) {
  ctx.out.println("[04] AUDIO quick check: 1 kHz tone");
  if (!ctx.board.audio().begin(44100)) {
    ctx.out.println("[FAIL] audio init failed");
    return TestStatus::Fail;
  }
  const bool ok = ctx.board.audio().tone(1000, 700, 25, &ctx.out);
  ctx.board.audio().end();
  if (!ok) {
    ctx.out.println("[FAIL] I2S tone generation failed");
    return TestStatus::Fail;
  }
  ctx.out.println("[PASS] I2S audio path executed");
  return TestStatus::Pass;
}
