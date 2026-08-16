# 07_IOTest — external GPIO input validation

This Arduino example validates the six external GPIO inputs recovered from the Panlee V15 / 230208 factory `IO Test`:

- GPIO10
- GPIO11
- GPIO12
- GPIO13
- GPIO14
- GPIO21

## Safety

All six pins are configured as `INPUT_PULLDOWN`.

**Apply only 3.3 V. Never apply 5 V to ESP32-S3 GPIO.**

The sketch does not drive these GPIOs as outputs.

## Procedure

1. Flash `07_IOTest.ino`.
2. Open Serial Monitor at 115200 baud.
3. Connect board GND to the 3.3 V test source ground.
4. Touch/apply 3.3 V to exactly one target GPIO at a time.
5. Remove 3.3 V before moving to the next GPIO.
6. Repeat for GPIO10, 11, 12, 13, 14 and 21.

The program requires a stable one-hot state. If more than one target GPIO is HIGH, the sample is rejected.

Expected result for each line:

```text
[PASS] GPIO10 observed HIGH as a stable one-hot input
```

Final candidate result:

```text
EXTERNAL IO INPUT TEST PHYSICAL PASS CANDIDATE
GPIO10/11/12/13/14/21 all observed as stable one-hot inputs.
```

## Claim boundary

A successful run validates the Arduino input path and physical accessibility of the six recovered GPIO signals on the named specimen. It does not yet validate output-drive capability, external load capability, voltage tolerance above 3.3 V, the original production fixture, or other WT32-SC01-PLUS OEM revisions.
