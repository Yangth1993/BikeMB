# Design: round UI concept

The first implementation uses existing demo metrics and LVGL primitives only. It avoids bitmap assets and custom fonts so the page can run on the current board baseline.

The screen contains three full-size page containers. A demo page index cycles between them so the visual concept can be verified on hardware before input handling is finalized.

The UI keeps the confirmed direction:

- restrained black OLED home page
- cyan speed trend waveform page
- compact ride details page

