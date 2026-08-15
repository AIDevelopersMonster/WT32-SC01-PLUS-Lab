#include <WT32_SC01_PLUS.h>
#include <esp_system.h>

WT32_SC01_PLUS board;

static const char *resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXTERNAL";
    case ESP_RST_SW:        return "SOFTWARE";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "OTHER_WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "UNKNOWN";
  }
}

static void heartbeat(const char *phase) {
  Serial.printf("[HEARTBEAT] phase=%s ms=%lu free_heap=%u min_free_heap=%u\n",
                phase,
                static_cast<unsigned long>(millis()),
                ESP.getFreeHeap(),
                ESP.getMinFreeHeap());
  Serial.flush();
}

static void quietHeartbeat(uint32_t durationMs, const char *phase) {
  const uint32_t started = millis();
  while (millis() - started < durationMs) {
    heartbeat(phase);
    delay(500);
  }
}

static bool runTone(const char *name,
                    uint32_t frequencyHz,
                    uint32_t durationMs,
                    uint8_t amplitudePercent) {
  Serial.printf("[TONE] %-10s %lu Hz, %lu ms, amplitude=%u%%\n",
                name,
                static_cast<unsigned long>(frequencyHz),
                static_cast<unsigned long>(durationMs),
                amplitudePercent);
  Serial.flush();

  const bool ok = board.audio().tone(
      frequencyHz, durationMs, amplitudePercent, &Serial);

  Serial.printf("[TONE] %-10s %s\n", name, ok ? "DONE" : "FAILED");
  Serial.flush();
  return ok;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 05_AudioTest");
  Serial.println(" Isolated I2S + Serial/reset stability diagnostic");
  Serial.println("============================================================");

  const esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("Reset reason : %d (%s)\n", static_cast<int>(reason), resetReasonName(reason));
  Serial.printf("Free heap    : %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Min free heap: %u bytes\n", ESP.getMinFreeHeap());
  Serial.println("Audio pins   : BCLK=GPIO36 LRCK=GPIO35 DOUT=GPIO37");
  Serial.println("Pin status   : EXPERIMENTAL on Panlee V15 until this test passes");
  Serial.println("Display      : intentionally NOT initialized");
  Serial.println("Wi-Fi/LVGL/SD: intentionally NOT initialized");
  Serial.println();
  Serial.println("WATCH FOR:");
  Serial.println("  1) audible clean tone bursts");
  Serial.println("  2) continuous Serial output");
  Serial.println("  3) no reboot / panic / watchdog / brownout");
  Serial.println("  4) no COM-port disappearance during I2S activity");
  Serial.println();

  quietHeartbeat(3000, "PRE_I2S");

  Serial.println("[AUDIO] Initializing I2S at 44100 Hz / 16-bit stereo...");
  Serial.flush();
  if (!board.audio().begin(44100)) {
    Serial.println("[FAIL] I2S initialization failed. Test stopped; no reboot requested.");
    while (true) {
      heartbeat("I2S_INIT_FAILED");
      delay(1000);
    }
  }

  Serial.println("[PASS] I2S initialized");
  heartbeat("I2S_READY");

  // Conservative amplitude ramp. The test deliberately avoids factory MP3.
  if (!runTone("LOW",    440,  700, 3)) goto audio_failed;
  if (!board.audio().silence(500)) goto audio_failed;
  if (!runTone("MID",    880,  700, 6)) goto audio_failed;
  if (!board.audio().silence(500)) goto audio_failed;
  if (!runTone("NORMAL", 1000, 1000, 10)) goto audio_failed;
  if (!board.audio().silence(700)) goto audio_failed;

  Serial.println("[STRESS] 1000 Hz for 10 seconds at 12% amplitude");
  Serial.println("[STRESS] Serial heartbeat should continue approximately once per second");
  Serial.flush();
  if (!runTone("STRESS", 1000, 10000, 12)) goto audio_failed;

  Serial.println("[AUDIO] Deinitializing I2S...");
  board.audio().end();
  Serial.println("[PASS] I2S deinitialized");

  quietHeartbeat(5000, "POST_I2S");

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" AUDIO STABILITY TEST COMPLETE");
  Serial.println(" If tones were audible and COM/Serial never disappeared:");
  Serial.println(" RESULT = OPERATOR AUDIO PASS CANDIDATE");
  Serial.println(" Keep Serial Monitor open; idle heartbeat continues below.");
  Serial.println("============================================================");
  Serial.flush();
  return;

 audio_failed:
  Serial.println("[FAIL] PCM write/silence failed. Deinitializing I2S without reboot.");
  board.audio().end();
  Serial.flush();
}

void loop() {
  heartbeat("IDLE_AFTER_TEST");
  delay(2000);
}
