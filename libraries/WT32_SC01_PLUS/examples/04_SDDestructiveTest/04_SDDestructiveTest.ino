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
  uint64_t sectorsProcessed = 0;
  uint32_t elapsedMs = 0;
};

void printBanner() {
  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 04_SDDestructiveTest");
  Serial.println(" FULL-MEDIA DESTRUCTIVE RAW SD QUALIFICATION");
  Serial.println("============================================================");
  Serial.println("WARNING: THIS TEST DESTROYS ALL PARTITIONS, FILESYSTEMS AND FILES.");
  Serial.println("Patterns: 0x00 -> verify -> 0xAA -> verify -> 0x55 -> verify");
  Serial.println("The final card contents will be 0x55 across the tested raw LBA range.");
  Serial.println("No filesystem will remain after a successful run.");
  Serial.println();
}

String readLineBlocking() {
  String line;
  while (true) {
    while (Serial.available()) {
      char c = static_cast<char>(Serial.read());
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
                   uint64_t done,
                   uint64_t total,
                   uint32_t startMs,
                   uint32_t &lastPercent) {
  if (total == 0) return;
  uint32_t percent = static_cast<uint32_t>((done * 100ULL) / total);
  if (percent > 100) percent = 100;
  if (percent < lastPercent + PROGRESS_STEP_PERCENT && done != total) return;
  lastPercent = percent;

  const uint32_t elapsed = millis() - startMs;
  double mib = static_cast<double>(done) * 512.0 / (1024.0 * 1024.0);
  double sec = elapsed > 0 ? elapsed / 1000.0 : 0.001;
  double mibps = mib / sec;

  Serial.printf("[%s] %3lu%% sectors=%llu/%llu elapsed=%lu s rate=%.2f MiB/s\n",
                phase,
                static_cast<unsigned long>(percent),
                static_cast<unsigned long long>(done),
                static_cast<unsigned long long>(total),
                static_cast<unsigned long>(elapsed / 1000UL),
                mibps);
}

PassResult writePattern(uint8_t pattern, uint64_t sectors) {
  PassResult r;
  r.expected = pattern;
  uint8_t buf[512];
  memset(buf, pattern, sizeof(buf));

  const uint32_t startMs = millis();
  uint32_t lastPercent = 0xFFFFFFFFu;

  Serial.printf("\n[WRITE 0x%02X] starting full-media raw write\n", pattern);
  for (uint64_t lba = 0; lba < sectors; ++lba) {
    if (!SD.writeRAW(buf, static_cast<uint32_t>(lba))) {
      r.ok = false;
      r.firstBadSector = static_cast<uint32_t>(lba);
      r.sectorsProcessed = lba;
      r.elapsedMs = millis() - startMs;
      Serial.printf("[FAIL] write error at LBA=%lu pattern=0x%02X\n",
                    static_cast<unsigned long>(r.firstBadSector), pattern);
      return r;
    }

    r.sectorsProcessed = lba + 1;
    uint32_t percent = static_cast<uint32_t>(((lba + 1) * 100ULL) / sectors);
    if (lastPercent == 0xFFFFFFFFu || percent >= lastPercent + PROGRESS_STEP_PERCENT || lba + 1 == sectors) {
      lastPercent = percent;
      printProgress("WRITE", lba + 1, sectors, startMs, lastPercent);
    }
  }

  r.elapsedMs = millis() - startMs;
  Serial.printf("[PASS] full write 0x%02X complete\n", pattern);
  return r;
}

PassResult verifyPattern(uint8_t pattern, uint64_t sectors) {
  PassResult r;
  r.expected = pattern;
  uint8_t buf[512];

  const uint32_t startMs = millis();
  uint32_t lastPrinted = 0xFFFFFFFFu;

  Serial.printf("\n[VERIFY 0x%02X] starting full-media readback\n", pattern);
  for (uint64_t lba = 0; lba < sectors; ++lba) {
    if (!SD.readRAW(buf, static_cast<uint32_t>(lba))) {
      r.ok = false;
      r.firstBadSector = static_cast<uint32_t>(lba);
      r.sectorsProcessed = lba;
      r.elapsedMs = millis() - startMs;
      Serial.printf("[FAIL] read error at LBA=%lu during verify 0x%02X\n",
                    static_cast<unsigned long>(r.firstBadSector), pattern);
      return r;
    }

    for (uint16_t i = 0; i < sizeof(buf); ++i) {
      if (buf[i] != pattern) {
        r.ok = false;
        r.firstBadSector = static_cast<uint32_t>(lba);
        r.firstBadOffset = i;
        r.actual = buf[i];
        r.sectorsProcessed = lba;
        r.elapsedMs = millis() - startMs;
        Serial.printf("[FAIL] mismatch LBA=%lu offset=%u expected=0x%02X actual=0x%02X\n",
                      static_cast<unsigned long>(r.firstBadSector),
                      r.firstBadOffset,
                      pattern,
                      r.actual);
        return r;
      }
    }

    r.sectorsProcessed = lba + 1;
    uint32_t percent = static_cast<uint32_t>(((lba + 1) * 100ULL) / sectors);
    if (lastPrinted == 0xFFFFFFFFu || percent >= lastPrinted + PROGRESS_STEP_PERCENT || lba + 1 == sectors) {
      lastPrinted = percent;
      uint32_t dummy = percent;
      printProgress("VERIFY", lba + 1, sectors, startMs, dummy);
    }
  }

  r.elapsedMs = millis() - startMs;
  Serial.printf("[PASS] full verify 0x%02X complete\n", pattern);
  return r;
}

bool runPattern(uint8_t pattern, uint64_t sectors) {
  PassResult w = writePattern(pattern, sectors);
  if (!w.ok) return false;

  PassResult v = verifyPattern(pattern, sectors);
  if (!v.ok) return false;

  Serial.printf("[PASS] pattern 0x%02X WRITE+READBACK verified across %llu sectors\n",
                pattern,
                static_cast<unsigned long long>(sectors));
  return true;
}
}

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
    return;
  }

  const uint64_t sectors = SD.numSectors();
  const uint32_t sectorSize = SD.sectorSize();
  const uint64_t bytes = sectors * static_cast<uint64_t>(sectorSize);

  Serial.printf("Card type  : %u\n", static_cast<unsigned>(SD.cardType()));
  Serial.printf("Sectors    : %llu\n", static_cast<unsigned long long>(sectors));
  Serial.printf("Sector size: %lu bytes\n", static_cast<unsigned long>(sectorSize));
  Serial.printf("Raw bytes  : %llu\n", static_cast<unsigned long long>(bytes));
  Serial.printf("Raw MiB    : %llu\n", static_cast<unsigned long long>(bytes / (1024ULL * 1024ULL)));

  if (sectorSize != 512 || sectors == 0 || sectors > 0xFFFFFFFFULL) {
    Serial.println("[FAIL] unsupported card geometry for this test implementation");
    SD.end();
    return;
  }

  Serial.println();
  Serial.println("DESTRUCTIVE ARMING REQUIRED");
  Serial.println("Type exactly: ERASE-ALL-SD");
  Serial.println("Then press Enter. Any other text aborts without writing.");
  Serial.print("> ");

  String arm = readLineBlocking();
  Serial.println(arm);
  if (arm != ARM_PHRASE) {
    Serial.println("[ABORT] destructive test not armed; no raw write started.");
    SD.end();
    sdSpi.end();
    return;
  }

  Serial.println();
  Serial.println("[ARMED] ALL DATA ON THIS CARD WILL NOW BE DESTROYED.");
  Serial.println("Do not remove power or the card during a write/verify phase unless intentionally aborting the test.");

  const uint32_t totalStart = millis();

  if (!runPattern(0x00, sectors)) {
    Serial.println("RESULT = FAIL DURING 0x00 PASS");
    return;
  }
  if (!runPattern(0xAA, sectors)) {
    Serial.println("RESULT = FAIL DURING 0xAA PASS");
    return;
  }
  if (!runPattern(0x55, sectors)) {
    Serial.println("RESULT = FAIL DURING 0x55 PASS");
    return;
  }

  const uint32_t totalElapsed = millis() - totalStart;
  SD.end();
  sdSpi.end();

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" SD FULL-MEDIA DESTRUCTIVE QUALIFICATION PASS");
  Serial.println(" 0x00 WRITE+VERIFY : PASS");
  Serial.println(" 0xAA WRITE+VERIFY : PASS");
  Serial.println(" 0x55 WRITE+VERIFY : PASS");
  Serial.printf(" Tested sectors    : %llu\n", static_cast<unsigned long long>(sectors));
  Serial.printf(" Total elapsed     : %lu s\n", static_cast<unsigned long>(totalElapsed / 1000UL));
  Serial.println(" Final media state : raw 0x55 pattern; NO filesystem remains");
  Serial.println("============================================================");
}

void loop() {
  delay(1000);
}
