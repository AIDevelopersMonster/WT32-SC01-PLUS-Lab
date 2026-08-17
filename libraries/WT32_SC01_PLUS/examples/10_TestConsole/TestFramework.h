#pragma once

#include <Arduino.h>
#include <WT32_SC01_PLUS.h>

enum class TestStatus : uint8_t { Idle, Running, Pass, Fail, Pending };

struct TestContext {
  WT32_SC01_PLUS &board;
  Stream &out;
};

using TestRunFn = TestStatus (*)(TestContext &ctx);

struct TestEntry {
  uint8_t id;
  const char *name;
  TestRunFn run;
  TestStatus status;
};

inline const char *statusName(TestStatus s) {
  switch (s) {
    case TestStatus::Idle: return "IDLE";
    case TestStatus::Running: return "RUN";
    case TestStatus::Pass: return "PASS";
    case TestStatus::Fail: return "FAIL";
    case TestStatus::Pending: return "PENDING";
  }
  return "?";
}
