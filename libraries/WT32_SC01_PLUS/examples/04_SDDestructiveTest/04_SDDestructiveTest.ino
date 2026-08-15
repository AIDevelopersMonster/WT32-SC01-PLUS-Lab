#include <WT32_SC01_PLUS.h>
#include <SD.h>
#include <SPI.h>

namespace {
SPIClass sdSpi(FSPI);

constexpr char ARM_PHRASE[] = "ERASE-ALL-SD";
constexpr uint32_t PROGRESS_STEP_PERCENT = 1;
constexpr uint32_t SECTOR_BYTES = 512;

struct PassResult {
  bool ok = true;
  uint32_t badLba = 0;
  uint16_t badOffset = 0;
  uint8_t expected = 0;
  uint8_t actual = 0;
};

void printBanner() {
  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 04_SDDestructiveTest");
  Serial.println(" FULL-MEDIA DESTRUCTIVE RAW SD QUALIFICATION");
  Serial.println("============================================================");
  Serial.println("WARNING: ALL PARTITIONS, FILESYSTEMS AND FILES WILL BE DESTROYED.");
  Serial.println("Sequence: 0x00 write+verify -> 0xAA write+verify -> 0x55 write+verify");
  Serial.println("Final raw media state after PASS: 0x55, no filesystem.");
  Serial.println();
}

String readLineBlocking() {
  String line;
  for (;;) {
    while (Serial.available()) {
      const char c = static_cast<char>(Serial.read());
      if (c == '\r') continue;
      if (c == '\n') {
        line.trim();
        return line;
      }
      line += c;
    }
    delay(10);
  }
}

void printProgress(const char *phase,
                   uint8_t pattern,
                   uint64_t done,
                   uint64_t total,
                   uint32_t startMs,
                   uint32_t &lastPrintedPercent) {
  if (total == 0) return;

  uint32_t percent = static_cast<uint32_t>((done * 100ULL) / total);
  if (percent > 100) percent = 100;

  if (done != total &&
      lastPrintedPercent != 0xFFFFFFFFu &&
      percent < lastPrintedPercent + PROGRESS_STEP_PERCENT) {
    return;
  }
  lastPrintedPercent = percent;

  const uint32_t elapsedMs = millis() - startMs;
  const double elapsedSec = elapsedMs > 0 ? elapsedMs / 1000.0 : 0.001;
  const double doneMiB = static_cast<double>(done) * SECTOR_BYTES / (1024.0 * 1024.0);
  const double rateMiB = doneMiB / elapsedSec;

  uint32_t etaSec = 0;
  if (rateMiB > 0.0 && done < total) {
    const double remainingMiB = static_cast<double>(total - done) * SECTOR_BYTES / (1024.0 * 1024.0);
    etaSec = static_cast<uint32_t>(remainingMiB / rateMiB);
  }

  Serial.printf("[%s 0x%02X] %3lu%% sectors=%llu/%llu rate=%.2f MiB/s elapsed=%lu s ETA=%lu s\n",
                phase,
                pattern,
                static_cast<unsigned long>(percent),
                static_cast<unsigned long long>(done),
                static_cast<unsigned long long>(total),
                rateMiB,
                static_cast<unsigned long>(elapsedMs / 1000UL),
                static_cast<unsigned long>(etaSec));
}

PassResult writePattern(uint8_t pattern, uint64_t sectors) {
  PassResult result;
  result.expected = pattern;

  uint8_t buffer[SECTOR_BYTES];
  memset(buffer, pattern, sizeof(buffer));

  const uint32_t startMs = millis();
  uint32_t lastPrintedPercent = 0xFFFFFFFFu;

  Serial.printf("\n[WRITE 0x%02X] full-media write starting\n", pattern);

  for (uint64_t lba = 0; lba < sectors; ++lba) {
    if (!SD.writeRAW(buffer, static_cast<uint32_t>(lba))) {
      result.ok = false;
      result.badLba = static_cast<uint32_t>(lba);
      Serial.printf("[FAIL] WRITE 0x%02X at LBA=%lu\n",
                    pattern,
                    static_cast<unsigned long>(result.badLba));
      return result;
    }

    printProgress("WRITE", pattern, lba + 1, sectors, startMs, lastPrintedPercent);
  }

  Serial.printf("[PASS] WRITE 0x%02X complete\n", pattern);
  return result;
}

PassResult verifyPattern(uint8_t pattern, uint64_t sectors) {
  PassResult result;
  result.expected = pattern;

  uint8_t buffer[SECTOR_BYTES];
  const uint32_t startMs = millis();
  uint32_t lastPrintedPercent = 0xFFFFFFFFu;

  Serial.printf("\n[VERIFY 0x%02X] full-media readback starting\n", pattern);

  for (uint64_t lba = 0; lba < sectors; ++lba) {
    if (!SD.readRAW(buffer, static_cast<uint32_t>(lba))) {
      result.ok = false;
      result.badLba = static_cast<uint32_t>(lba);
      Serial.printf("[FAIL] READ during VERIFY 0x%02X at LBA=%lu\n",
                    pattern,
                    static_cast<unsigned long>(result.badLba));
      return result;
    }

    for (uint16_t offset = 0; offset < SECTOR_BYTES; ++offset) {
      if (buffer[offset] != pattern) {
        result.ok = false;
        result.badLba = static_cast<uint32_t>(lba);
        result.badOffset = offset;
        result.actual = buffer[offset];
        Serial.printf("[FAIL] MISMATCH LBA=%lu offset=%u expected=0x%02X actual=0x%02X\n",
                      static_cast<unsigned long>(result.badLba),
                      result.badOffset,
                      pattern,
                      result.actual);
        return result;
      }
    }

    printProgress("VERIFY", pattern, lba + 1, sectors, startMs, lastPrintedPercent);
  }

  Serial.printf("[PASS] VERIFY 0x%02X complete\n", pattern);
  return result;
}

bool runPattern(uint8_t pattern, uint64_t sectors) {
  const PassResult writeResult = writePattern(pattern, sectors);
  if (!writeResult.ok) return false;

  const PassResult verifyResult = verifyPattern(pattern, sectors);
  if (!verifyResult.ok) return false;

  Serial.printf("[PASS] PATTERN 0x%02X WRITE + FULL READBACK VERIFIED\n", pattern);
  return true;
}

void stopCard() {
  SD.end();
  sdSpi.end();
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);
  printBanner();

  pinMode(wt32sc01plus::pins::SD_CS, OUTPUT);
  digitalWrite(wt32sc01plus::pins::SD_CS, HIGH);

  sdSpi.begin(wt32sc01plus::pins::SD_SCK,
              wt32sc01plus::pins::SD_MISO,
              wt32sc01plus::pins::SD_MOSI,
              wt32sc01plus::pins::SD_CS);

  Serial.printf("Pins       : SCK=%d MISO=%d MOSI=%d CS=%d\n",
                wt32sc01plus::pins::SD_SCK,
                wt32sc01plus::pins::SD_MISO,
                wt32sc01plus::pins::SD_MOSI,
                wt32sc01plus::pins::SD_CS);
  Serial.printf("SPI clock  : %lu Hz\n",
                static_cast<unsigned long>(wt32sc01plus::pins::SD_SPI_HZ));
  Serial.println("[SD] Initializing card...");

  if (!SD.begin(wt32sc01plus::pins::SD_CS,
                sdSpi,
                wt32sc01plus::pins::SD_SPI_HZ,
                "/sd",
                2,
                false)) {
    Serial.println("[FAIL] SD initialization/mount failed");
    sdSpi.end();
    return;
  }

  const uint64_t sectors = SD.numSectors();
  const uint32_t sectorSize = SD.sectorSize();
  const uint64_t rawBytes = sectors * static_cast<uint64_t>(sectorSize);

  Serial.printf("Card type  : %u\n", static_cast<unsigned>(SD.cardType()));
  Serial.printf("Sectors    : %llu\n", static_cast<unsigned long long>(sectors));
  Serial.printf("Sector size: %lu bytes\n", static_cast<unsigned long>(sectorSize));
  Serial.printf("Raw bytes  : %llu\n", static_cast<unsigned long long>(rawBytes));
  Serial.printf("Raw MiB    : %llu\n", static_cast<unsigned long long>(rawBytes / (1024ULL * 1024ULL)));

  if (sectorSize != SECTOR_BYTES || sectors == 0 || sectors > 0xFFFFFFFFULL) {
    Serial.println("[FAIL] unsupported card geometry for this test");
    stopCard();
    return;
  }

  Serial.println();
  Serial.println("DESTRUCTIVE ARMING REQUIRED");
  Serial.println("Type exactly: ERASE-ALL-SD");
  Serial.println("Then press Enter. Any other text aborts without writing.");
  Serial.print("> ");

  const String arm = readLineBlocking();
  Serial.println(arm);

  if (arm != ARM_PHRASE) {
    Serial.println("[ABORT] destructive test not armed; no raw write started.");
    stopCard();
    return;
  }

  Serial.println();
  Serial.println("[ARMED] ALL DATA ON THIS CARD WILL NOW BE DESTROYED.");

  const uint32_t totalStartMs = millis();

  if (!runPattern(0x00, sectors)) {
    Serial.println("RESULT = FAIL DURING 0x00 PASS");
    stopCard();
    return;
  }

  if (!runPattern(0xAA, sectors)) {
    Serial.println("RESULT = FAIL DURING 0xAA PASS");
    stopCard();
    return;
  }

  if (!runPattern(0x55, sectors)) {
    Serial.println("RESULT = FAIL DURING 0x55 PASS");
    stopCard();
    return;
  }

  const uint32_t totalElapsedMs = millis() - totalStartMs;
  stopCard();

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" SD FULL-MEDIA DESTRUCTIVE QUALIFICATION PASS");
  Serial.println(" 0x00 WRITE + VERIFY : PASS");
  Serial.println(" 0xAA WRITE + VERIFY : PASS");
  Serial.println(" 0x55 WRITE + VERIFY : PASS");
  Serial.printf(" Tested sectors      : %llu\n", static_cast<unsigned long long>(sectors));
  Serial.printf(" Total elapsed       : %lu s\n", static_cast<unsigned long>(totalElapsedMs / 1000UL));
  Serial.println(" Final media state   : raw 0x55; NO filesystem remains");
  Serial.println("============================================================");
}

void loop() {
  delay(1000);
}
