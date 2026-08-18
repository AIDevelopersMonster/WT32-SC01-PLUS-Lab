#include <WT32_SC01_PLUS.h>

WT32_SC01_PLUS board;

void setup() {
  Serial.begin(115200);

  if (!board.begin()) {
    Serial.println("WT32-SC01-PLUS init failed");
    while (true) {
      delay(1000);
    }
  }

  board.printBoardInfo(Serial);
  board.backlight().set(80);
  board.display().drawTestPattern();
}

void loop() {
  WT32_SC01_PLUS_TouchPoint point;

  if (board.touch().read(point) && point.touched) {
    Serial.printf("touch: x=%u y=%u rawX=%u rawY=%u\n",
                  point.x, point.y, point.rawX, point.rawY);

    const int marker = 5;
    board.display().fillRect((int)point.x - marker,
                             (int)point.y - marker,
                             marker * 2 + 1,
                             marker * 2 + 1,
                             0xFFFF);
  }

  delay(10);
}
