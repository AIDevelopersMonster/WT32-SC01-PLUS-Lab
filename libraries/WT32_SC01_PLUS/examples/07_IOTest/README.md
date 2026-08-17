# 07_IOTest — external GPIO input validation

**Test status:** `PHYSICAL TEST VALIDATED`  
**Reference specimen:** Panlee `ZX3D50CE08S-V15-USRC / 230208`  
**Validation date:** 2026-08-16

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

The Extended I/O connector `+` rail is not used as the stimulus source because it is +5 V. A known-safe 3.3 V source must be used instead.

## Test logic

The program samples all six GPIOs and accepts a channel only when exactly one target GPIO is HIGH for the required stable interval.

Examples:

```text
000000  -> no active input
000001  -> GPIO10 only
000010  -> GPIO11 only
000100  -> GPIO12 only
001000  -> GPIO13 only
010000  -> GPIO14 only
100000  -> GPIO21 only
```

If more than one target GPIO is HIGH simultaneously, the state is rejected rather than counted as a PASS.

## Physical validation result

The test was exercised manually on the reference board using a 3.3 V stimulus. All six channels were independently detected and associated with the correct one-hot mask:

```text
GPIO10 : PASS
GPIO11 : PASS
GPIO12 : PASS
GPIO13 : PASS
GPIO14 : PASS
GPIO21 : PASS
```

Observed representative output included:

```text
[PASS] GPIO10 observed HIGH as a stable one-hot input
[PASS] GPIO11 observed HIGH as a stable one-hot input
[PASS] GPIO12 observed HIGH as a stable one-hot input
[PASS] GPIO13 observed HIGH as a stable one-hot input
[PASS] GPIO14 observed HIGH as a stable one-hot input
[PASS] GPIO21 observed HIGH as a stable one-hot input
```

## Evidence combination

The physical validation evidence is intentionally combined from two manual probing runs.

During the first run, GPIO13, GPIO14, GPIO21, GPIO10 and GPIO11 were observed as PASS. A brownout reset then occurred while manually moving small 1.25 mm connector probes.

After reboot, the test again ran normally and independently confirmed GPIO13, GPIO12, GPIO11 and GPIO10. No GPIO detection or test-logic failure was observed.

Because every one of the six channels produced an explicit stable one-hot PASS event, the diagnostic itself is accepted as:

```text
07_IOTest: PHYSICAL TEST VALIDATED
```

The brownout is recorded as an incidental manual-fixture event, not as evidence of a failed GPIO channel or failed detection algorithm.

## What is validated

The physical run validates that this Arduino diagnostic:

- reads GPIO10/11/12/13/14/21 using the intended pin mapping;
- distinguishes each channel from the other five;
- maps each input to the expected one-hot bit;
- rejects zero-input and multi-input states as non-PASS states;
- applies `INPUT_PULLDOWN` successfully on the reference board;
- records stable HIGH events correctly on real hardware.

This is primarily validation of the **Arduino diagnostic test and BSP pin mapping on real hardware**, not a new proof of the factory production fixture.

## Recommended future fixture

For repeated or production-style testing, use a small 1.25 mm breakout/fixture instead of manual probing. This reduces the chance of accidental rail contact or power disturbance and allows one continuous six-channel run.

## Claim boundary

A successful run does not establish:

- output-drive capability of these pins;
- external load capability;
- 5 V tolerance;
- electrical characteristics of the original factory fixture;
- identical mapping on other WT32-SC01-PLUS OEM revisions.
