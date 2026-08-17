#pragma once
#include "TestFramework.h"
#include <WiFi.h>

inline TestStatus runWifiTest(TestContext &ctx) {
  ctx.out.println("[06] WIFI quick scan check");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);
  delay(200);
  const int n = WiFi.scanNetworks(false, true);
  if (n < 0) {
    ctx.out.printf("[FAIL] Wi-Fi scan returned %d\n", n);
    return TestStatus::Fail;
  }
  ctx.out.printf("[PASS] Wi-Fi scan found %d network(s)\n", n);
  WiFi.scanDelete();
  return TestStatus::Pass;
}
