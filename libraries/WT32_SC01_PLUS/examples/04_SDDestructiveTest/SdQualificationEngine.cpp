#include "SdQualificationEngine.h"

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <unistd.h>

#include <esp_heap_caps.h>
#include <esp_vfs_fat.h>
#include <driver/sdspi_host.h>
#include <driver/spi_master.h>
#include <sdmmc_cmd.h>

#include <WT32_SC01_PLUS.h>

namespace {
constexpr char kMountPoint[] = "/sdqual";
constexpr char kProbeFile[] = "/sdqual/QUALIFY.TXT";
constexpr char kProbeText[] = "WT32-SC01-PLUS SD QUALIFICATION PASS\n";

spi_bus_config_t makeBusConfig() {
  spi_bus_config_t cfg{};
  cfg.mosi_io_num = wt32sc01plus::pins::SD_MOSI;
  cfg.miso_io_num = wt32sc01plus::pins::SD_MISO;
  cfg.sclk_io_num = wt32sc01plus::pins::SD_SCK;
  cfg.quadwp_io_num = -1;
  cfg.quadhd_io_num = -1;
  cfg.max_transfer_sz = SdQualificationEngine::kBlockBytes;
  return cfg;
}

sdspi_device_config_t makeSlotConfig(int hostId) {
  sdspi_device_config_t cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
  cfg.gpio_cs = static_cast<gpio_num_t>(wt32sc01plus::pins::SD_CS);
  cfg.host_id = static_cast<spi_host_device_t>(hostId);
  return cfg;
}
}  // namespace

bool SdQualificationEngine::begin(SdCardInfo &info, SdQualFailure &failure) {
  failure = SdQualFailure{};
  info_ = SdCardInfo{};

  buffer_ = heap_caps_malloc(kBlockBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!buffer_) {
    failure.phase = "ALLOC 32K";
    failure.error = ESP_ERR_NO_MEM;
    return false;
  }

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.max_freq_khz = kSpiHz / 1000;
  busHost_ = host.slot;

  spi_bus_config_t busCfg = makeBusConfig();
  esp_err_t err = spi_bus_initialize(static_cast<spi_host_device_t>(busHost_),
                                     &busCfg,
                                     SDSPI_DEFAULT_DMA);
  if (err != ESP_OK) {
    failure.phase = "SPI BUS INIT";
    failure.error = err;
    end();
    return false;
  }
  busInitialized_ = true;

  sdspi_device_config_t slotCfg = makeSlotConfig(busHost_);
  sdspi_dev_handle_t handle = -1;
  err = sdspi_host_init_device(&slotCfg, &handle);
  if (err != ESP_OK) {
    failure.phase = "SDSPI DEVICE";
    failure.error = err;
    end();
    return false;
  }
  deviceHandle_ = static_cast<int>(handle);
  deviceInitialized_ = true;

  host.slot = deviceHandle_;

  auto *card = static_cast<sdmmc_card_t *>(malloc(sizeof(sdmmc_card_t)));
  if (!card) {
    failure.phase = "CARD ALLOC";
    failure.error = ESP_ERR_NO_MEM;
    end();
    return false;
  }
  memset(card, 0, sizeof(*card));
  cardStorage_ = card;

  err = sdmmc_card_init(&host, card);
  if (err != ESP_OK) {
    failure.phase = "CARD INIT";
    failure.error = err;
    end();
    return false;
  }

  if (card->csd.sector_size != static_cast<int>(kSectorBytes) ||
      card->csd.capacity <= 0) {
    failure.phase = "CARD GEOMETRY";
    failure.error = ESP_ERR_INVALID_SIZE;
    end();
    return false;
  }

  info_.sectors = static_cast<uint64_t>(card->csd.capacity);
  info_.sectorSize = static_cast<uint32_t>(card->csd.sector_size);
  info_.bytes = info_.sectors * info_.sectorSize;
  strncpy(info_.name, card->cid.name, sizeof(info_.name) - 1);
  info_.name[sizeof(info_.name) - 1] = '\0';

  info = info_;
  return true;
}

void SdQualificationEngine::reportProgress(SdProgressCallback callback,
                                           void *context,
                                           const char *phase,
                                           uint8_t pattern,
                                           uint8_t stage,
                                           uint64_t done,
                                           uint64_t total,
                                           uint32_t startMs,
                                           uint32_t &lastPercent) {
  if (!callback || total == 0) return;

  uint32_t percent = static_cast<uint32_t>((done * 100ULL) / total);
  if (percent > 100) percent = 100;
  if (done != total && lastPercent != 0xFFFFFFFFu && percent <= lastPercent) return;
  lastPercent = percent;

  const uint32_t elapsedMs = millis() - startMs;
  const double elapsedSec = elapsedMs ? elapsedMs / 1000.0 : 0.001;
  const double doneMiB = static_cast<double>(done) * kSectorBytes / (1024.0 * 1024.0);
  const double rate = doneMiB / elapsedSec;

  uint32_t eta = 0;
  if (rate > 0.0 && done < total) {
    const double remainingMiB = static_cast<double>(total - done) * kSectorBytes /
                                (1024.0 * 1024.0);
    eta = static_cast<uint32_t>(remainingMiB / rate);
  }

  callback(context, phase, pattern, stage, kTotalStages, done, total, rate, eta);
}

bool SdQualificationEngine::writePattern(uint8_t pattern,
                                         uint8_t stage,
                                         SdProgressCallback callback,
                                         void *context,
                                         SdQualFailure &failure) {
  auto *card = static_cast<sdmmc_card_t *>(cardStorage_);
  auto *buffer = static_cast<uint8_t *>(buffer_);
  memset(buffer, pattern, kBlockBytes);

  const uint32_t startMs = millis();
  uint32_t lastPercent = 0xFFFFFFFFu;

  for (uint64_t lba = 0; lba < info_.sectors;) {
    const uint32_t count = static_cast<uint32_t>(
        std::min<uint64_t>(kBlockSectors, info_.sectors - lba));

    const esp_err_t err = sdmmc_write_sectors(card,
                                               buffer,
                                               static_cast<size_t>(lba),
                                               count);
    if (err != ESP_OK) {
      failure.phase = "WRITE";
      failure.pattern = pattern;
      failure.lba = lba;
      failure.error = err;
      return false;
    }

    lba += count;
    reportProgress(callback, context, "WRITE", pattern, stage,
                   lba, info_.sectors, startMs, lastPercent);
    yield();
  }

  return true;
}

bool SdQualificationEngine::verifyPattern(uint8_t pattern,
                                          uint8_t stage,
                                          SdProgressCallback callback,
                                          void *context,
                                          SdQualFailure &failure) {
  auto *card = static_cast<sdmmc_card_t *>(cardStorage_);
  auto *buffer = static_cast<uint8_t *>(buffer_);

  const uint32_t startMs = millis();
  uint32_t lastPercent = 0xFFFFFFFFu;

  for (uint64_t lba = 0; lba < info_.sectors;) {
    const uint32_t count = static_cast<uint32_t>(
        std::min<uint64_t>(kBlockSectors, info_.sectors - lba));

    const esp_err_t err = sdmmc_read_sectors(card,
                                              buffer,
                                              static_cast<size_t>(lba),
                                              count);
    if (err != ESP_OK) {
      failure.phase = "VERIFY READ";
      failure.pattern = pattern;
      failure.lba = lba;
      failure.error = err;
      return false;
    }

    const size_t bytesToCheck = static_cast<size_t>(count) * kSectorBytes;
    for (size_t i = 0; i < bytesToCheck; ++i) {
      if (buffer[i] != pattern) {
        failure.phase = "VERIFY DATA";
        failure.pattern = pattern;
        failure.lba = lba + (i / kSectorBytes);
        failure.offset = static_cast<uint16_t>(i % kSectorBytes);
        failure.expected = pattern;
        failure.actual = buffer[i];
        failure.error = ESP_FAIL;
        return false;
      }
    }

    lba += count;
    reportProgress(callback, context, "VERIFY", pattern, stage,
                   lba, info_.sectors, startMs, lastPercent);
    yield();
  }

  return true;
}

void SdQualificationEngine::deinitRaw() {
  if (deviceInitialized_) {
    sdspi_host_remove_device(static_cast<sdspi_dev_handle_t>(deviceHandle_));
    deviceInitialized_ = false;
    deviceHandle_ = -1;
  }

  if (busInitialized_) {
    spi_bus_free(static_cast<spi_host_device_t>(busHost_));
    busInitialized_ = false;
    busHost_ = -1;
  }

  if (cardStorage_) {
    free(cardStorage_);
    cardStorage_ = nullptr;
  }
}

bool SdQualificationEngine::restoreEmptyFat(SdProgressCallback callback,
                                            void *context,
                                            SdQualFailure &failure) {
  if (callback) {
    callback(context, "FORMAT FAT", 0, 7, kTotalStages,
             0, info_.sectors, 0.0, 0);
  }

  deinitRaw();

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.max_freq_khz = kSpiHz / 1000;

  spi_bus_config_t busCfg = makeBusConfig();
  esp_err_t err = spi_bus_initialize(static_cast<spi_host_device_t>(host.slot),
                                     &busCfg,
                                     SDSPI_DEFAULT_DMA);
  if (err != ESP_OK) {
    failure.phase = "FORMAT BUS";
    failure.error = err;
    return false;
  }

  sdspi_device_config_t slotCfg = makeSlotConfig(host.slot);
  esp_vfs_fat_sdmmc_mount_config_t mountCfg{};
  mountCfg.format_if_mount_failed = true;
  mountCfg.max_files = 4;
  mountCfg.allocation_unit_size = 16 * 1024;

  sdmmc_card_t *fatCard = nullptr;
  err = esp_vfs_fat_sdspi_mount(kMountPoint,
                                &host,
                                &slotCfg,
                                &mountCfg,
                                &fatCard);
  if (err != ESP_OK) {
    failure.phase = "FORMAT FAT";
    failure.error = err;
    spi_bus_free(static_cast<spi_host_device_t>(host.slot));
    return false;
  }

  bool ok = true;
  FILE *f = fopen(kProbeFile, "wb");
  if (!f) {
    ok = false;
    failure.phase = "FORMAT WRITE";
    failure.error = ESP_FAIL;
  } else {
    const size_t expected = strlen(kProbeText);
    if (fwrite(kProbeText, 1, expected, f) != expected) {
      ok = false;
      failure.phase = "FORMAT WRITE";
      failure.error = ESP_FAIL;
    }
    fclose(f);
  }

  if (ok) {
    char probe[sizeof(kProbeText)] = {0};
    f = fopen(kProbeFile, "rb");
    if (!f) {
      ok = false;
      failure.phase = "FORMAT READ";
      failure.error = ESP_FAIL;
    } else {
      const size_t expected = strlen(kProbeText);
      const size_t got = fread(probe, 1, expected, f);
      fclose(f);
      if (got != expected || memcmp(probe, kProbeText, expected) != 0) {
        ok = false;
        failure.phase = "FORMAT VERIFY";
        failure.error = ESP_FAIL;
      }
    }
  }

  if (ok && unlink(kProbeFile) != 0) {
    ok = false;
    failure.phase = "FORMAT DELETE";
    failure.error = ESP_FAIL;
  }

  esp_vfs_fat_sdcard_unmount(kMountPoint, fatCard);
  spi_bus_free(static_cast<spi_host_device_t>(host.slot));

  if (ok && callback) {
    callback(context, "FORMAT FAT", 0, 7, kTotalStages,
             info_.sectors, info_.sectors, 0.0, 0);
  }

  return ok;
}

bool SdQualificationEngine::run(SdProgressCallback callback,
                                void *context,
                                SdQualFailure &failure) {
  failure = SdQualFailure{};

  if (!cardStorage_ || !buffer_) {
    failure.phase = "ENGINE NOT READY";
    failure.error = ESP_ERR_INVALID_STATE;
    return false;
  }

  if (!writePattern(0x00, 1, callback, context, failure)) return false;
  if (!verifyPattern(0x00, 2, callback, context, failure)) return false;
  if (!writePattern(0xAA, 3, callback, context, failure)) return false;
  if (!verifyPattern(0xAA, 4, callback, context, failure)) return false;
  if (!writePattern(0x55, 5, callback, context, failure)) return false;
  if (!verifyPattern(0x55, 6, callback, context, failure)) return false;
  if (!restoreEmptyFat(callback, context, failure)) return false;

  return true;
}

void SdQualificationEngine::end() {
  deinitRaw();
  if (buffer_) {
    heap_caps_free(buffer_);
    buffer_ = nullptr;
  }
}
