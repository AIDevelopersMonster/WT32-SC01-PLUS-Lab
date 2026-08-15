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
  Serial.printf("[TONE] %-14s %lu Hz, %lu ms, amplitude=%u%%\n",
                name,
                static_cast<unsigned long>(frequencyHz),
                static_cast<unsigned long>(durationMs),
                amplitudePercent);
  Serial.flush();

  const bool ok = board.audio().tone(
      frequencyHz, durationMs, amplitudePercent, &Serial);

  Serial.printf("[TONE] %-14s %s\n", name, ok ? "DONE" : "FAILED");
  Serial.flush();
  return ok;
}

static bool gap(uint32_t durationMs) {
  return board.audio().silence(durationMs);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 05_AudioTest");
  Serial.println(" HIGH-POWER I2S + Serial/reset stability diagnostic");
  Serial.println("============================================================");

  const esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("Reset reason : %d (%s)\n", static_cast<int>(reason), resetReasonName(reason));
  Serial.printf("Free heap    : %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Min free heap: %u bytes\n", ESP.getMinFreeHeap());
  Serial.println("Audio pins   : BCLK=GPIO36 LRCK=GPIO35 DOUT=GPIO37");
  Serial.println("Pin status   : EXPERIMENTAL on Panlee V15 until full-power test passes");
  Serial.println("Display      : intentionally NOT initialized");
  Serial.println("Wi-Fi/LVGL/SD: intentionally NOT initialized");
  Serial.println();
  Serial.println("CAUTION:");
  Serial.println("  This revision reaches full-scale PCM and is intentionally loud.");
  Serial.println("  Stop/reset if the speaker sounds mechanically distressed or overheats.");
  Serial.println();
  Serial.println("WATCH FOR:");
  Serial.println("  1) clean increase in loudness through the amplitude staircase");
  Serial.println("  2) continuous Serial output during high-power playback");
  Serial.println("  3) no reboot / panic / watchdog / brownout");
  Serial.println("  4) no COM-port disappearance during loud I2S activity");
  Serial.println("  5) no instability during repeated 100% load transients");
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

  // Loudness staircase. Each step is separated by silence so power/load
  // changes are easy to hear and supply-transient effects are easier to spot.
  if (!runTone("LEVEL_20", 1000, 1200, 20)) goto audio_failed;
  if (!gap(400)) goto audio_failed;
  if (!runTone("LEVEL_35", 1000, 1200, 35)) goto audio_failed;
  if (!gap(400)) goto audio_failed;
  if (!runTone("LEVEL_50", 1000, 1200, 50)) goto audio_failed;
  if (!gap(400)) goto audio_failed;
  if (!runTone("LEVEL_65", 1000, 1200, 65)) goto audio_failed;
  if (!gap(400)) goto audio_failed;
  if (!runTone("LEVEL_80", 1000, 1200, 80)) goto audio_failed;
  if (!gap(400)) goto audio_failed;
  if (!runTone("LEVEL_95", 1000, 1200, 95)) goto audio_failed;
  if (!gap(500)) goto audio_failed;
  if (!runTone("LEVEL_100", 1000, 1500, 100)) goto audio_failed;
  if (!gap(800)) goto audio_failed;

  // Sustained near-full-scale load to expose power-rail, thermal, USB and
  // scheduler problems without keeping the speaker at exact full scale for long.
  Serial.println("[STRESS-A] 1000 Hz for 15 seconds at 90% amplitude");
  Serial.println("[STRESS-A] Serial heartbeat should continue approximately once per second");
  Serial.flush();
  if (!runTone("SUSTAINED_90", 1000, 15000, 90)) goto audio_failed;
  if (!gap(1000)) goto audio_failed;

  // Repeated full-scale bursts stress current transients and rail recovery.
  Serial.println("[STRESS-B] Five full-scale bursts: 100% / 1 second, 300 ms silence");
  Serial.flush();
  for (int i = 1; i <= 5; ++i) {
    char label[20];
    snprintf(label, sizeof(label), "FULL_BURST_%d", i);
    if (!runTone(label, 1000, 1000, 100)) goto audio_failed;
    if (!gap(300)) goto audio_failed;
    heartbeat("FULL_SCALE_GAP");
  }

  Serial.println("[AUDIO] Deinitializing I2S...");
  board.audio().end();
  Serial.println("[PASS] I2S deinitialized");

  quietHeartbeat(5000, "POST_I2S");

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" HIGH-POWER AUDIO STABILITY TEST COMPLETE");
  Serial.println(" If loudness increased normally and COM/Serial never disappeared:");
  Serial.println(" RESULT = OPERATOR FULL-POWER AUDIO PASS CANDIDATE");
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
