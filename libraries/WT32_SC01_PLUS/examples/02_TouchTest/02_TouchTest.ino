#include <WT32_SC01_PLUS.h>

WT32_SC01_PLUS board;

namespace {
constexpr uint16_t BLACK  = 0x0000;
constexpr uint16_t RED    = 0xF800;
constexpr uint16_t GREEN  = 0x07E0;
constexpr uint16_t YELLOW = 0xFFE0;

struct Target {
  const char *name;
  int x;
  int y;
};

constexpr Target targets[] = {
  {"TOP_LEFT",      40,  40},
  {"TOP_RIGHT",    439,  40},
  {"CENTER",       240, 160},
  {"BOTTOM_LEFT",   40, 279},
  {"BOTTOM_RIGHT", 439, 279},
};

constexpr int TARGET_HALF = 12;
constexpr int HIT_RADIUS = 45;
constexpr int STABLE_HITS_REQUIRED = 3;

void drawTarget(const Target &t, uint16_t color) {
  board.display().fillRect(t.x - TARGET_HALF, t.y - 2, TARGET_HALF * 2 + 1, 5, color);
  board.display().fillRect(t.x - 2, t.y - TARGET_HALF, 5, TARGET_HALF * 2 + 1, color);
}

bool waitForRelease(uint32_t timeoutMs = 3000) {
  const uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    WT32_SC01_PLUS_TouchPoint points[2];
    uint8_t count = 0;
    if (!board.touch().readPoints(points, 2, count)) return false;
    if (count == 0) return true;
    delay(20);
  }
  return false;
}

bool runTarget(const Target &t) {
  board.display().fillScreen(BLACK);
  drawTarget(t, RED);

  Serial.printf("[TARGET] %s lcd=(%d,%d)\n", t.name, t.x, t.y);
  Serial.println("Touch the red cross.");

  int stableHits = 0;
  const uint32_t start = millis();
  while (millis() - start < 15000) {
    WT32_SC01_PLUS_TouchPoint points[2];
    uint8_t count = 0;
    if (!board.touch().readPoints(points, 2, count)) {
      Serial.println("[FAIL] touch frame read error");
      return false;
    }

    if (count > 0) {
      int best = -1;
      int bestD2 = 0x7fffffff;

      for (uint8_t i = 0; i < count; ++i) {
        const int dx = static_cast<int>(points[i].x) - t.x;
        const int dy = static_cast<int>(points[i].y) - t.y;
        const int d2 = dx * dx + dy * dy;

        Serial.printf("[TOUCH] points=%u slot=%u raw=(%u,%u) mapped=(%u,%u) event=%u track=%u INT=%d\n",
                      count, i,
                      points[i].rawX, points[i].rawY,
                      points[i].x, points[i].y,
                      points[i].event, points[i].trackId,
                      board.touch().interruptLevel());

        if (d2 < bestD2) {
          bestD2 = d2;
          best = i;
        }
      }

      if (best >= 0) {
        const auto &p = points[best];
        board.display().fillRect(static_cast<int>(p.x) - 4,
                                 static_cast<int>(p.y) - 4,
                                 9, 9, YELLOW);

        if (bestD2 <= HIT_RADIUS * HIT_RADIUS) {
          ++stableHits;
          Serial.printf("[HIT] %s stable=%d/%d\n",
                        t.name, stableHits, STABLE_HITS_REQUIRED);
          if (stableHits >= STABLE_HITS_REQUIRED) {
            const int dx = static_cast<int>(p.x) - t.x;
            const int dy = static_cast<int>(p.y) - t.y;
            drawTarget(t, GREEN);
            Serial.printf("[PASS] %s dx=%d dy=%d\n", t.name, dx, dy);
            delay(250);
            return waitForRelease();
          }
        } else {
          stableHits = 0;
        }
      }
    } else {
      stableHits = 0;
    }

    delay(20);
  }

  Serial.printf("[FAIL] timeout waiting for %s\n", t.name);
  return false;
}
}

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 02_TouchTest");
  Serial.println(" FT6336U-compatible five-point landscape validation");
  Serial.println("============================================================");
  Serial.println("Touch bus        : SDA=6 SCL=5 INT=7 RST=4(shared)");
  Serial.println("I2C              : 0x38 @ 400 kHz on Wire1");
  Serial.println("Transform        : LCD_X=raw_Y, LCD_Y=319-raw_X");
  Serial.println("Frame mode       : coherent TD_STATUS + P1 + P2 read");
  Serial.println("Acceptance       : 3 consecutive hits per target");
  Serial.println();

  if (!board.begin()) {
    Serial.println("[FAIL] display/backlight initialization failed");
    while (true) delay(1000);
  }

  delay(250);

  if (!board.touch().begin()) {
    Serial.println("[FAIL] touch initialization/identity check failed");
    while (true) delay(1000);
  }

  Serial.printf("[PASS] touch identity: chip=0x%02X firmware=0x%02X focaltech=0x%02X\n",
                board.touch().chipCode(),
                board.touch().firmwareId(),
                board.touch().focalTechId());

  bool allPass = true;
  for (const auto &target : targets) {
    if (!runTarget(target)) {
      allPass = false;
      break;
    }
    delay(250);
  }

  board.display().fillScreen(allPass ? GREEN : RED);
  Serial.println();
  Serial.println("============================================================");
  if (allPass) {
    Serial.println(" TOUCH FIVE-POINT TEST PASS");
    Serial.println(" Arduino I2C + identity + coherent P1/P2 read + landscape transform passed.");
    Serial.println(" Reference specimen: Panlee V15 / 230208 PHYSICAL PASS.");
  } else {
    Serial.println(" TOUCH TEST FAILED / INVESTIGATE");
  }
  Serial.println("============================================================");
}

void loop() {
  delay(1000);
}
