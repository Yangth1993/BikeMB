# BikeMB visual guidelines

This document defines visual rules for the BikeMB round-screen UI.

## Design principles

- Riding readability first.
- One primary number per riding page.
- Dark OLED-style background by default.
- Strong hierarchy through scale, contrast, and spacing.
- Mode color is an accent, not the whole interface.
- Every visual element should be practical in LVGL.

## Round-screen layout

The physical screen is `360 x 360`, but the corners are not usable on a round display.

Recommended zones:

| Zone | Approx. bounds | Use |
| --- | --- | --- |
| Safe center | x 44-316, y 44-316 | Primary information and actions |
| Top status | y 20-72 | Battery, mode, network |
| Primary value | y 96-210 | Speed or main state |
| Bottom status | y 260-330 | Time, page dots, secondary value |
| Edge decoration | Near circular edge | Arcs, glow, separators only |

Do not place critical text near the circular edge.

## Visual hierarchy

| Level | Use | Rule |
| --- | --- | --- |
| Primary | Current speed or main AI state | Largest object on page |
| Secondary | Time, battery, trip value | Readable at a glance |
| Tertiary | Labels, page dots, units | Muted but legible |
| Decoration | Glow, arcs, separators | Never competes with data |

If two elements compete for attention, reduce the non-riding element first.

## Color tokens

Base palette:

| Token | Value | Use |
| --- | --- | --- |
| `COLOR_BG` | `#050707` | Screen background |
| `COLOR_PANEL` | `#101315` | Cards and grouped surfaces |
| `COLOR_TEXT` | `#F4F7F8` | Primary text |
| `COLOR_TEXT_MUTED` | `#8D969D` | Secondary labels |
| `COLOR_LINE` | `#30363A` | Separators and quiet arcs |
| `COLOR_ERROR` | `#F04E3E` | Error state |

Mode accents:

| Mode | Token | Value |
| --- | --- | --- |
| `ECO` | `COLOR_ECO` | `#23D66B` |
| `TRAIL` | `COLOR_TRAIL` | `#F3B53F` |
| `AUTO` | `COLOR_AUTO` | `#238CFF` |
| `BOOST` | `COLOR_BOOST` | `#F04E3E` |

Rules:

- Use one mode accent per page.
- Do not mix multiple saturated mode colors on the same page.
- Error red must include text or icon meaning; do not rely on color alone.
- Keep primary text high contrast against the dark background.

## Typography

Recommended display scale for the 360 px screen:

| Style | Size | Use |
| --- | --- | --- |
| `DisplayXL` | 88-104 px | Home speed integer |
| `DisplayL` | 46-64 px | AI state or detail page main value |
| `Body` | 18-26 px | Labels, units, row names |
| `Meta` | 18-24 px | Time, battery, page hints |

Rules:

- Use tabular numbers for speed, time, distance, and battery.
- Avoid long text labels.
- Prefer short English labels until final localization is defined.
- Do not use text smaller than 18 px for information needed while riding.

## Components

LVGL-friendly component set:

| Component | LVGL primitive | Use |
| --- | --- | --- |
| Primary value | `lv_label` | Speed, AI state, main metric |
| Assist or progress arc | `lv_arc` | Mode accent, progress, AI waiting |
| Metric row/card | `lv_obj` + `lv_label` | Ride details and settings rows |
| Page dots | Small `lv_obj` circles | Dashboard page position |
| Divider | `lv_line` or styled object | Quiet separation |
| Icon | Recolorable `lv_img` | Battery, network, settings |
| Simple chart | `lv_line` or `lv_chart` | Only when data exists |

Avoid:

- Bitmap text.
- Large full-screen PNGs that contain dynamic data.
- Tiny tap targets.
- Decorative animations unrelated to state.
- High-frequency animations that reduce frame stability.

## Riding readability checks

Before accepting a screen:

- The main value is readable in one glance.
- The main value remains readable with motion blur or vibration.
- Battery and mode are visible without opening settings.
- The page still makes sense if AI/network data is offline.
- No critical label sits on the circular edge.
- Page switching feedback is visible.

## Waiting and error expression

Waiting:

- Use small motion: dots, pulse, or arc.
- Keep text short: `Waiting`, `Sending`, `Thinking`.
- Allow navigation away.

Error:

- Localize the error to the affected area.
- Use `Failed`, `Offline`, or `No data` on screen.
- Put detailed diagnostics in logs or non-riding detail screens.
- Use full-screen error only if the UI cannot render the dashboard.

## Asset rules

- Dynamic text must stay in LVGL labels.
- Dynamic battery, network, AI state, speed, and distance must not be baked into images.
- Monochrome PNG icons may be recolored by mode.
- Export image assets with ASCII lowercase filenames.
- Keep source design frames at exactly `360 x 360 px`.
