# 11_RainbowTouch

Interactive Arduino BSP example for the Panlee WT32-SC01-PLUS reference profile.

The sketch initializes the library display/backlight and touch subsystems, then lets the user paint on the screen with a persistent coordinate-dependent RGB565 trail.

The color field is intentionally simple:

```text
R = map(X, left -> right, 0 -> 255)
G = map(Y, top -> bottom, 0 -> 255)
B = map(X + Y, near -> far, 255 -> 0)
```

No LovyanGFX configuration, LCD/touch GPIO table, HSV conversion, palette, or lookup table is required in the application sketch.

## Physical validation

Status: **PHYSICAL PASS**

Validated on:

```text
Panlee WT32-SC01-PLUS
ZX3D50CE08S-V15-USRC / 230208
```

Observed:

- successful compile and upload;
- display and touch initialization;
- finger tracking across the display;
- persistent drawing trail;
- continuous position-dependent rainbow-like color change.

## Video

YouTube Shorts — physical upload and demonstration:

https://youtube.com/shorts/Rl50IJfhhrM

## Origin of the example

This example was created after physically testing and studying the independent Sukesh Akhilesh WT32-SC01-PLUS LovyanGFX touch-drawing Gist. The upstream project was used as a compatibility/reference point; this implementation is independently written against the `WT32_SC01_PLUS` BSP.

Upstream physical-demonstration video:

https://youtube.com/shorts/5CkP_Jh4ofo

## Release

Because this example lives inside the Arduino library `examples/` tree, it is intended to be included in the next packaged `WT32_SC01_PLUS` Arduino release ZIP.
