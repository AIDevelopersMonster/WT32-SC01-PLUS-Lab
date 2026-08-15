#include <WT32_SC01_PLUS.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

namespace {
SPIClass sdSpi(FSPI);

const char *cardTypeName(uint8_t type) {
  switch (type) {
    case CARD_MMC: return "MMC";
    case CARD_SD: return "SDSC";
    case CARD_SDHC: return "SDHC";
    case CARD_NONE: return "NONE";
    default: return "UNKNOWN";
  }
}

bool readSample(File &file, size_t maxBytes) {
  if (!file || file.isDirectory()) return false;

  const size_t total = file.size();
  const size_t want = total < maxBytes ? total : maxBytes;
  size_t readCount = 0;
  uint32_t checksum = 2166136261u;

  while (readCount < want && file.available()) {
    int v = file.read();
    if (v < 0) break;
    checksum ^= static_cast<uint8_t>(v);
    checksum *= 16777619u;
    ++readCount;
  }

  Serial.printf("      sample_read=%u bytes checksum=0x%08lX\n",
                static_cast<unsigned>(readCount),
                static_cast<unsigned long>(checksum));
  return readCount == want;
}

bool listAndRead(fs::FS &fs, const char *dirname, uint8_t maxFiles) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.println("[FAIL] unable to open root directory");
    return false;
  }

  Serial.println("[DIRECTORY] root entries");
  uint8_t entries = 0;
  uint8_t filesRead = 0;

  File file = root.openNextFile();
  while (file && entries < maxFiles) {
    ++entries;
    if (file.isDirectory()) {
      Serial.printf("  DIR : %s\n", file.name());
    } else {
      Serial.printf("  FILE: %s size=%llu\n",
                    file.name(),
                    static_cast<unsigned long long>(file.size()));
      if (filesRead < 3) {
        if (!readSample(file, 256)) {
          Serial.println("[FAIL] file sample read failed");
          file.close();
          root.close();
          return false;
        }
        ++filesRead;
      }
    }
    file.close();
    file = root.openNextFile();
  }

  root.close();
  Serial.printf("[PASS] directory enumeration entries=%u files_sampled=%u\n", entries, filesRead);
  return true;
}
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 03_StorageTest");
  Serial.println(" READ-ONLY SD/SDSPI + FAT mount validation");
  Serial.println("============================================================");
  Serial.printf("Pins             : SCK=%d MISO=%d MOSI=%d CS=%d\n",
                wt32sc01plus::pins::SD_SCK,
                wt32sc01plus::pins::SD_MISO,
                wt32sc01plus::pins::SD_MOSI,
                wt32sc01plus::pins::SD_CS);
  Serial.printf("SPI clock        : %lu Hz\n",
                static_cast<unsigned long>(wt32sc01plus::pins::SD_SPI_HZ));
  Serial.println("Filesystem       : Arduino SD / FAT mount");
  Serial.println("Safety           : READ ONLY - no create/write/append/rename/delete/format");
  Serial.println("Display/Touch    : intentionally NOT initialized");
  Serial.println("Audio/Wi-Fi/LVGL : intentionally NOT initialized");
  Serial.println();

  pinMode(wt32sc01plus::pins::SD_CS, OUTPUT);
  digitalWrite(wt32sc01plus::pins::SD_CS, HIGH);

  sdSpi.begin(wt32sc01plus::pins::SD_SCK,
              wt32sc01plus::pins::SD_MISO,
              wt32sc01plus::pins::SD_MOSI,
              wt32sc01plus::pins::SD_CS);

  Serial.println("[SD] Mounting card...");
  if (!SD.begin(wt32sc01plus::pins::SD_CS,
                sdSpi,
                wt32sc01plus::pins::SD_SPI_HZ,
                "/sd",
                5,
                false)) {
    Serial.println("[FAIL] SD mount failed");
    Serial.println("RESULT = INVESTIGATE");
    return;
  }

  const uint8_t type = SD.cardType();
  if (type == CARD_NONE) {
    Serial.println("[FAIL] no SD card detected after mount");
    SD.end();
    return;
  }

  Serial.printf("[PASS] card type   : %s\n", cardTypeName(type));
  Serial.printf("[INFO] card size   : %llu MiB\n",
                static_cast<unsigned long long>(SD.cardSize() / (1024ULL * 1024ULL)));
  Serial.printf("[INFO] sectors     : %u\n", static_cast<unsigned>(SD.numSectors()));
  Serial.printf("[INFO] sector size : %u bytes\n", static_cast<unsigned>(SD.sectorSize()));
  Serial.printf("[INFO] FAT total   : %llu MiB\n",
                static_cast<unsigned long long>(SD.totalBytes() / (1024ULL * 1024ULL)));
  Serial.printf("[INFO] FAT used    : %llu MiB\n",
                static_cast<unsigned long long>(SD.usedBytes() / (1024ULL * 1024ULL)));

  uint8_t sector0[512] = {0};
  Serial.println("[RAW] Reading sector 0...");
  if (!SD.readRAW(sector0, 0)) {
    Serial.println("[FAIL] raw sector-0 read failed");
    SD.end();
    return;
  }

  const uint16_t signature = static_cast<uint16_t>(sector0[510]) |
                             (static_cast<uint16_t>(sector0[511]) << 8);
  Serial.printf("[PASS] sector 0 read, signature=0x%04X\n", signature);

  if (!listAndRead(SD, "/", 20)) {
    SD.end();
    return;
  }

  SD.end();
  sdSpi.end();

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" SD READ-ONLY TEST PASS CANDIDATE");
  Serial.println(" SDSPI mount + metadata + raw sector read + FAT directory read passed.");
  Serial.println(" No write operation was requested by this sketch.");
  Serial.println("============================================================");
}

void loop() {
  delay(1000);
}
