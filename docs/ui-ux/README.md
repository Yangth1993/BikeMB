# BikeMB UI/UX docs

This directory stores round-screen UI structure, interaction rules, visual guidelines, AI state expression, and LVGL implementation guidance for BikeMB.

## Primary docs

- [interaction-model.md](interaction-model.md): physical button, touch, gesture, feedback, and UI state model.
- [screen-flows.md](screen-flows.md): boot flow, dashboard page order, settings flow, waiting flow, and error flow.
- [ai-assistant-ui.md](ai-assistant-ui.md): AI assistant page layout, state table, boundaries, and LVGL component guidance.
- [visual-guidelines.md](visual-guidelines.md): round-screen layout zones, hierarchy, color, type, readability, and component rules.

## Reference and pipeline docs

- [ui-redesign-avinox-ux.md](ui-redesign-avinox-ux.md): current Avinox-inspired round-screen UI/UX design notes.
- [lvgl-pixel-replica-pipeline.md](lvgl-pixel-replica-pipeline.md): design-to-LVGL asset and simulator verification pipeline.

## Working rule

For UI changes, update the smallest relevant primary doc first. Use the reference and pipeline docs only when a change affects visual direction, image assets, font assets, or simulator verification.
