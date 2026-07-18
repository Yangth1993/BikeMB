# Design: round UI concept

The first implementation uses existing demo metrics and a hybrid LVGL visual layer. Dynamic data and interaction remain native LVGL widgets. Complex static decoration is exported as transparent PNG and converted to an LVGL C image, while large speed digits use a generated subset font.

The screen contains three full-size page containers. A demo page index cycles between them so the visual concept can be verified on hardware before input handling is finalized.

The UI keeps the confirmed direction:

- restrained black OLED home page
- cyan speed trend waveform page
- compact ride details page

The visual asset pipeline uses the official `lv_font_conv` and `lv_img_conv` projects. The image converter runs in a pinned Node 20 tool runtime because its native `canvas` dependency is not compatible with the workstation's Node 24 runtime. Generated assets are shared by the simulator and firmware builds.
