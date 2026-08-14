# FactoryCTL v0.1 - safe JTAG factory-test runner

`factoryctl.py` converts the reverse-engineered factory-test addresses for the verified Panlee `ZX3D50CE08S-V15-USRC` specimen (`230208`) into reproducible host-side commands.

This is deliberately a conservative first release. It contains no flash erase/write command and no eFuse write command. Before any PC/register/RAM manipulation, a run command resets and halts the target and checks two code signatures from the verified factory image. A signature mismatch aborts the operation; v0.1 has no force override.

Known full-flash SHA-256:

```text
3772c1bf7d6d2b713973212ddf5c671e3c844a13a8464f675343d9aed4e7f044
```

## Prerequisites

Start OpenOCD for the ESP32-S3 built-in USB Serial/JTAG interface in a separate terminal. The current lab setup uses the Espressif `board/esp32s3-builtin.cfg` configuration and the OpenOCD telnet port `4444`.

Run commands from the repository root:

```powershell
py tools/factory-test/factoryctl.py list
py tools/factory-test/factoryctl.py status
py tools/factory-test/factoryctl.py run display
py tools/factory-test/factoryctl.py run touch
py tools/factory-test/factoryctl.py run audio
py tools/factory-test/factoryctl.py run sd
py tools/factory-test/factoryctl.py show-ok
```

An optional known factory dump can be hashed in `status` mode:

```powershell
py tools/factory-test/factoryctl.py --firmware-bin C:\path\factory-flash-16mb.bin status
```

## Safety behavior

- Flash programming is absent by design.
- eFuse programming is absent by design.
- `run io` is refused because the recovered GPIO10/11/12/13/14/21 stage needs the external one-hot fixture.
- `run usb` is refused because the current built-in USB-JTAG transport occupies GPIO19/GPIO20, which the factory USB stage intentionally remuxes.
- `run full` is refused in v0.1.
- `run sd` requires an explicit confirmation because the factory code may create or replace `/sdcard/hello.txt` and `/sdcard/foo.txt`.
- A completed isolated `show-ok` is not proof that IO or USB passed.

## JTAG factory entry used by v0.1

The runner uses the already verified volatile factory-entry method:

```text
reset halt
verify factory code signatures
break at 0x4200690E
resume and wait for the receive gate
set a10 = 1
write 0xFF to the selector output byte at [a1]
break at 0x42007060
resume and wait for factory-runner initialization
```

No flash bytes are changed by this procedure.

## Stage boundaries

```text
AUDIO    0x42007060 -> 0x42007063
DISPLAY  0x42007063 -> 0x42007066
TOUCH    0x42007066 -> 0x42007069
IO       0x42007069 -> 0x4200706C
SD       0x4200706C -> 0x4200706F
USB      0x4200706F -> 0x42007072
OK       0x42007072 -> 0x42007075
```

For DISPLAY, TOUCH, SD and OK, FactoryCTL stops on the next recovered runner boundary and reports that the factory stage returned.

Audio is asynchronous. v0.1 first confirms that the launcher returned, then resumes into the interactive LCD stage for a configurable observation window so the scheduler can continue running the audio task. Do not touch the panel during that window. Physical audio completion remains an operator observation rather than an automatically certified result.

## Important status boundary

The addresses and signatures are evidence-backed for the verified factory firmware, but FactoryCTL v0.1 itself still requires end-to-end validation on the physical reference specimen. Until that validation is recorded, treat this release as `IMPLEMENTED / PHYSICAL_TOOL_VALIDATION_PENDING`.
