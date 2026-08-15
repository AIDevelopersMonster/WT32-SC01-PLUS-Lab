#include <WT32_SC01_PLUS.h>

#include "SdQualificationEngine.h"
#include "SdQualificationUi.h"

WT32_SC01_PLUS board;
SdQualificationEngine engine;
SdQualificationUi ui(board);

namespace {
SdCardInfo cardInfo;
SdQualFailure failure;

void progressCallback(void *context,
                      const char *phase,
                      uint8_t pattern,
                      uint8_t stage,
                      uint8_t totalStages,
                      uint64_t doneSectors,
                      uint64_t totalSectors,
                      double rateMiBPerSec,
                      uint32_t etaSeconds) {
  auto *displayUi = static_cast<SdQualificationUi *>(context);
  displayUi->showProgress(phase,
                          pattern,
                          stage,
                          totalStages,
                          doneSectors,
                          totalSectors,
                          rateMiBPerSec,
                          etaSeconds);

  Serial.printf("[%u/%u] %s 0x%02X %llu/%llu %.2f MiB/s ETA=%lu s\n",
                stage,
                totalStages,
                phase,
                pattern,
                static_cast<unsigned long long>(doneSectors),
                static_cast<unsigned long long>(totalSectors),
                rateMiBPerSec,
                static_cast<unsigned long>(etaSeconds));
}

void haltForever() {
  for (;;) delay(1000);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("WT32-SC01-PLUS autonomous SD qualification");
  Serial.println("UI: LCD + touch; Serial is diagnostic only.");
  Serial.println("I/O: 64 sectors / 32 KiB per multi-sector transfer.");

  if (!ui.begin()) {
    Serial.println("[FAIL] display/touch initialization");
    haltForever();
  }

  ui.showProgress("CARD INIT", 0, 0, SdQualificationEngine::kTotalStages,
                  0, 1, 0.0, 0);

  if (!engine.begin(cardInfo, failure)) {
    Serial.printf("[FAIL] engine begin: %s err=%d\n", failure.phase, failure.error);
    ui.showFailure(failure);
    haltForever();
  }

  Serial.printf("[CARD] %s sectors=%llu sector=%lu bytes=%llu\n",
                cardInfo.name,
                static_cast<unsigned long long>(cardInfo.sectors),
                static_cast<unsigned long>(cardInfo.sectorSize),
                static_cast<unsigned long long>(cardInfo.bytes));

  ui.showReady(cardInfo);

  if (!ui.waitForStart()) {
    failure.phase = "START TOUCH";
    ui.showFailure(failure);
    engine.end();
    haltForever();
  }

  if (!ui.confirmErase()) {
    Serial.println("[ABORT] operator cancelled destructive test");
    engine.end();
    ui.showReady(cardInfo);
    haltForever();
  }

  Serial.println("[ARMED] autonomous destructive qualification started");

  const bool pass = engine.run(progressCallback, &ui, failure);
  engine.end();

  if (!pass) {
    Serial.printf("[FAIL] phase=%s pattern=0x%02X lba=%llu offset=%u expected=0x%02X actual=0x%02X err=%d\n",
                  failure.phase,
                  failure.pattern,
                  static_cast<unsigned long long>(failure.lba),
                  failure.offset,
                  failure.expected,
                  failure.actual,
                  failure.error);
    ui.showFailure(failure);
    haltForever();
  }

  Serial.println("[PASS] 0x00/0xAA/0x55 full-media write+verify complete");
  Serial.println("[PASS] FAT restored and probe file write/read/delete verified");
  ui.showPass(cardInfo);
}

void loop() {
  delay(1000);
}
