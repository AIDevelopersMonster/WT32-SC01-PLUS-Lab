#include "WT32_SC01_PLUS.h"

#include <ESP_I2S.h>
#include <math.h>

namespace {
I2SClass s_i2s;

constexpr size_t kFramesPerBlock = 128;
int16_t s_stereoBlock[kFramesPerBlock * 2];

bool writeAll(const void *data, size_t bytes) {
    const uint8_t *p = static_cast<const uint8_t *>(data);
    size_t remaining = bytes;
    while (remaining > 0) {
        const size_t written = s_i2s.write(p, remaining);
        if (written == 0) return false;
        p += written;
        remaining -= written;
    }
    return true;
}
} // namespace

bool WT32_SC01_PLUS_Audio::begin(uint32_t sampleRate) {
    if (ready_) return true;

    s_i2s.setPins(
        wt32sc01plus::pins::AUDIO_BCLK,
        wt32sc01plus::pins::AUDIO_LRCK,
        wt32sc01plus::pins::AUDIO_DOUT,
        -1,
        -1);

    if (!s_i2s.begin(
            I2S_MODE_STD,
            sampleRate,
            I2S_DATA_BIT_WIDTH_16BIT,
            I2S_SLOT_MODE_STEREO)) {
        return false;
    }

    sampleRate_ = sampleRate;
    ready_ = true;
    return silence(30);
}

void WT32_SC01_PLUS_Audio::end() {
    if (!ready_) return;
    silence(20);
    s_i2s.end();
    ready_ = false;
    sampleRate_ = 0;
}

bool WT32_SC01_PLUS_Audio::silence(uint32_t durationMs) {
    if (!ready_ || sampleRate_ == 0) return false;

    memset(s_stereoBlock, 0, sizeof(s_stereoBlock));
    uint64_t framesLeft = (static_cast<uint64_t>(sampleRate_) * durationMs) / 1000ULL;

    while (framesLeft > 0) {
        const size_t frames = framesLeft > kFramesPerBlock
                                ? kFramesPerBlock
                                : static_cast<size_t>(framesLeft);
        if (!writeAll(s_stereoBlock, frames * 2 * sizeof(int16_t))) return false;
        framesLeft -= frames;
        yield();
    }
    return true;
}

bool WT32_SC01_PLUS_Audio::tone(uint32_t frequencyHz,
                                uint32_t durationMs,
                                uint8_t amplitudePercent,
                                Stream *diagnostics) {
    if (!ready_ || sampleRate_ == 0 || frequencyHz == 0) return false;
    if (amplitudePercent > 100) amplitudePercent = 100;

    const float amplitude = 32767.0f * (static_cast<float>(amplitudePercent) / 100.0f);
    const float phaseStep = 2.0f * PI * static_cast<float>(frequencyHz) /
                            static_cast<float>(sampleRate_);
    float phase = 0.0f;
    uint64_t framesLeft = (static_cast<uint64_t>(sampleRate_) * durationMs) / 1000ULL;
    uint32_t nextReport = millis() + 1000U;

    while (framesLeft > 0) {
        const size_t frames = framesLeft > kFramesPerBlock
                                ? kFramesPerBlock
                                : static_cast<size_t>(framesLeft);

        for (size_t i = 0; i < frames; ++i) {
            const int16_t sample = static_cast<int16_t>(sinf(phase) * amplitude);
            s_stereoBlock[i * 2] = sample;
            s_stereoBlock[i * 2 + 1] = sample;
            phase += phaseStep;
            if (phase >= 2.0f * PI) phase -= 2.0f * PI;
        }

        if (!writeAll(s_stereoBlock, frames * 2 * sizeof(int16_t))) return false;
        framesLeft -= frames;

        if (diagnostics != nullptr && static_cast<int32_t>(millis() - nextReport) >= 0) {
            diagnostics->print("[AUDIO HEARTBEAT] ms=");
            diagnostics->print(millis());
            diagnostics->print(" free_heap=");
            diagnostics->println(ESP.getFreeHeap());
            nextReport += 1000U;
        }
        yield();
    }

    return silence(30);
}
