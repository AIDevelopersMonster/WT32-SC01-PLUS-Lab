# 00_identity_probe

First executable hardware test. It should report ESP chip identity, revision, flash size, PSRAM availability/size, reset reason, heap and SDK/core version without touching unknown display/touch GPIO.

This is intentionally the safest first test for an incompletely identified OEM board.
