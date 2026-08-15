#include <WT32_SC01_PLUS.h>
#include <SD.h>
#include <SPI.h>

namespace {
SPIClass sdSpi(FSPI);

constexpr char ARM_PHRASE[] = "ERASE-ALL-SD";
constexpr uint32_t PROGRESS_STEP_PERCENT = 1;

struct PassResult {
  bool ok = true;
  uint32_t firstBadSector = 0;
  uint16_t firstBadOffset = 0;
  uint8_t expected = 0;
  uint8_t actual = 0;
  uint