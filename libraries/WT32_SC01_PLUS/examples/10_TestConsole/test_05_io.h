#pragma once
#include "TestFramework.h"

inline TestStatus runIoTest(TestContext &ctx) {
  static const uint8_t pins[] = {10, 11, 12, 13, 14, 21};
  bool seen[6] = {false, false, false, false, false, false};
  ctx.out.println("[05] EXTERNAL IO interactive check");
  ctx.out.println("Apply ONLY 3.3 V to one GPIO at a time: 10,11,12,13,14,21");
  for (uint8_t p : pins) pinMode(p, INPUT_PULLDOWN);

  const uint32_t start = millis();
  while (millis() - start < 60000) {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < 6; ++i) if (digitalRead(pins[i])) mask |= (1U << i);
    if (mask && ((mask & (mask - 1U)) == 0)) {
      for (uint8_t i = 0; i < 6; ++i) {
        if (mask == (1U << i) && !seen[i]) {
          delay(80);
          uint8_t confirm = 0;
          for (uint8_t j = 0; j < 6; ++j) if (digitalRead(pins[j])) confirm |= (1U << j);
          if (confirm == mask) {
            seen[i] = true;
            ctx.out.printf("[PASS] GPIO%u one-hot observed\n", pins[i]);
          }
        }
      }
    }
    bool all = true;
    for (bool v : seen) all &= v;
    if (all) {
      ctx.out.println("[PASS] all six external IO inputs observed");
      return TestStatus::Pass;
    }
    delay(20);
  }
  ctx.out.println("[FAIL] timeout before all six IO inputs were observed");
  return TestStatus::Fail;
}
