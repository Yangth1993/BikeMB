# BikeMB interaction model

This document defines the UI interaction contract for the 360 x 360 round LCD.
It is a design and LVGL implementation guide, not a firmware driver spec.

## Scope

Current scope:

- Dashboard page switching.
- Physical button and touch behavior.
- AI page state control at the UI layer.
- Waiting, error, disabled, and offline feedback.
- LVGL-friendly component boundaries.

Out of scope for this document:

- Motor control.
- Phone app navigation.
- Cloud account flows.
- Wi-Fi password or token entry.
- Always-on wake word behavior.

## Assumptions

- Target screen is a `360 x 360` round LCD.
- P0 must remain usable without network, AI, voice, or phone app.
- At least one physical button is available for page switching.
- Touch is available through CST816, but riding use must not depend on precise touch.
- LVGL is the implementation UI framework.

## Input priority

Physical button behavior has priority over touch because it is safer while riding.

| Input | Primary use | Rule |
| --- | --- | --- |
| Short button press | Switch dashboard page | Cycle through dashboard pages only |
| Long button press | Reserved | Do not assign a critical P0 action until hardware behavior is verified |
| Horizontal swipe | Switch dashboard page | Same order as button cycling |
| Swipe up | Open settings list | Available from any dashboard page |
| Swipe down / back area | Return | Return one level up |
| Center tap on AI page | Start, cancel, or stop AI interaction | Depends on current AI state |

## Page model

Top-level UI states:

| State | Purpose |
| --- | --- |
| `Dashboard` | Riding-first information pages |
| `SettingsList` | Short list of non-riding settings entries |
| `SettingsDetail` | One settings detail page |

Dashboard page order:

1. `Home`
2. `AiAssistant`
3. `RideDetails`

The UI must remember `last_dashboard_page` so returning from settings restores the page the rider came from.

## Button behavior

Short press:

- In `Dashboard`, move to the next dashboard page.
- In `SettingsList`, move selection to the next row only if row selection is implemented.
- In `SettingsDetail`, return to `SettingsList` only if there is no separate back button.

Long press:

- Reserved for later confirmation.
- Do not use for reset, erase, network pairing, or destructive actions in P0.

## Touch behavior

Touch zones should be coarse and forgiving:

| Zone | Suggested bounds | Use |
| --- | --- | --- |
| Center action | Approx. 120-240 px x 100-260 px | AI start/cancel/stop |
| Top status | Approx. y 0-72 px | Status display only |
| Bottom navigation | Approx. y 288-360 px | Page dots, time, back affordance |
| Full page horizontal swipe | Whole circular visible area | Dashboard page switch |
| Full page vertical swipe | Whole circular visible area | Settings enter/return |

Avoid small icon-only touch targets. If an icon is visually small, its active hit area should still be at least about `44 x 44 px`.

## Gesture conflict rules

- Dashboard supports horizontal paging and swipe up to settings.
- Settings pages do not support horizontal dashboard paging.
- AI center tap must not trigger page switching.
- A swipe should win over a tap when movement passes the configured gesture threshold.
- Avoid nested scroll areas on the round screen.

## Feedback rules

Every accepted input needs visible feedback within one UI frame or the next LVGL tick:

- Button page switch: update page dot and animate/crossfade page content.
- Touch press: dim, brighten, or pulse the touched action region.
- AI start: switch text from idle to listening immediately.
- Cancel/stop: show a short neutral transition, then return to idle or offline.
- Invalid action: show no destructive effect; use muted text or a brief disabled pulse.

## LVGL implementation notes

Use small, explicit state machines:

- `ui_route_state`: `Dashboard`, `SettingsList`, `SettingsDetail`.
- `dashboard_page`: `Home`, `AiAssistant`, `RideDetails`.
- `ai_visual_state`: UI-facing AI state only.

Keep page rendering separate from input handling:

- Input code changes state.
- Page code renders from state.
- AI, network, audio, and storage backends must not directly mutate LVGL widgets.
