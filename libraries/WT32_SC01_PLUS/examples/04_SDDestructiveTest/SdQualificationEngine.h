#pragma once

#include <Arduino.h>

struct SdCardInfo {
  uint64_t sectors = 0;
  uint32_t sectorSize = 0;
  uint64_t bytes = 0;
  char name[16] = {0};
};

struct SdQualFailure {
  const char *phase = "NONE";
  uint8_t pattern = 0;
  uint64_t lba = 0;
  uint16_t offset = 0;
  uint8_t expected = 0;
  uint8_t actual = 0;
  int error = 0;
};

using SdProgressCallback = void (*)(void *context,
                                    const char *phase,
                                    uint8_t pattern,
                                    uint8_t stage,
                                    uint8_t totalStages,
                                    uint64_t doneSectors,
                                    uint64_t totalSectors,
                                    double rateMiBPerSec,
                                    uint32_t etaSeconds);

class SdQualificationEngine {
public:
  static constexpr uint32_t kSectorBytes = 512;
  static constexpr uint32_t kBlockSectors = 64;
  static constexpr uint32_t kBlockBytes = kSectorBytes * kBlockSectors;
  static constexpr uint32_t kSpiHz = 10000000;
  static constexpr uint8_t kTotalStages = 7;

  bool begin(SdCardInfo &info, SdQualFailure &failure);
  bool run(SdProgressCallback callback, void *context, SdQualFailure &failure);
  void end();

private:
  bool writePattern(uint8_t pattern, uint8_t stage,
                    SdProgressCallback callback, void *context,
                    SdQualFailure &failure);
  bool verifyPattern(uint8_t pattern, uint8_t stage,
                     SdProgressCallback callback, void *context,
                     SdQualFailure &failure);
  bool restoreEmptyFat(SdProgressCallback callback, void *context,
                       SdQualFailure &failure);
  void reportProgress(SdProgressCallback callback, void *context,
                      const char *phase, uint8_t pattern, uint8_t stage,
                      uint64_t done, uint64_t total, uint32_t startMs,
                      uint32_t &lastPercent);
  void deinitRaw();

  void *buffer_ = nullptr;
  void *cardStorage_ = nullptr;
  int deviceHandle_ = -1;
  int busHost_ = -1;
  bool busInitialized_ = false;
  bool deviceInitialized_ = false;
  SdCardInfo info_{};
};
