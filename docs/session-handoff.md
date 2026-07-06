# BikeMB Session Handoff

## Current Objective

Prepare the PC-side LVGL simulator flow so BikeMB UI can be reviewed before flashing firmware.

## Latest Stable Point

- Last saved project commit before this handoff work: `ff5a6a1 Add BikeMB LVGL demo baseline and tests`
- Current working focus: official LVGL PC simulator setup and Windows/MSYS2 runner scripts

## Verified

- GitHub SSH authentication works for `Yangth1993`.
- Official `lvgl/lv_port_pc_vscode` checkout exists locally under ignored path `tools/lv_port_pc_vscode/`.
- Simulator branch is `release/v8`.
- `lvgl` and `lv_drivers` submodules are present.
- MSYS2 SDL2 dependency was installed with `pacman -S --needed --noconfirm mingw-w64-x86_64-SDL2`.
- Official simulator built and launched as `tools/lv_port_pc_vscode/build/bin/demo.exe`.
- Lightweight contract tests pass with:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File tools\run-tests.ps1`

## Next Recommended Step

Connect BikeMB dashboard UI into the official LVGL simulator `ui/` mechanism, reusing the existing dashboard view/metrics structure instead of creating a custom simulator.

## Known Notes

- `tools/lv_port_pc_vscode/` is intentionally ignored and should not be committed.
- Official `release/v8` simulator uses Makefile, not root-level CMake.
- On Windows, build through MSYS2 bash with SDL2.
- Keep simulator integration small: first show the current dashboard on PC, then improve sharing between firmware and simulator.
