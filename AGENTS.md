# AGENTS.md

## Project Role

You are assisting with BikeMB embedded firmware development.

Optimize for:

- token efficiency
- minimal context gathering
- minimal safe changes
- hardware safety

This file is for agent collaboration, not human onboarding. Keep actions narrow, explicit, and cheap unless the user asks for broader work.

## Hard Rules

- Prefer the smallest useful scope.
- Do not expand file search or refactor broadly unless requested.
- Read project context before code when starting a task:
  - `README.md`
  - `openspec/`
  - `docs/project-context.md`
- Follow OpenSpec-first workflow for feature work and behavior changes.
- Preserve existing architecture and public interfaces unless the task requires change.
- Prefer diagnostic steps before high-risk firmware edits.

## Model Selection Rules

- Use `gpt-5.5` for:
  - RTOS work
  - DMA issues
  - bootloader issues
  - linker issues
  - register-level debugging
  - complex bugs spanning multiple files
- Use `gpt-5.4-mini` for:
  - driver scaffolding
  - tests
  - CMake changes
  - small refactors
  - mocks
- Use `gpt-5.4-mini` or `gpt-5.3-codex-spark` for:
  - documentation
  - comments
  - changelogs
  - naming-only changes
- Fallbacks:
  - if `gpt-5.5` is unavailable, fall back to `gpt-5.4-mini`
  - if `gpt-5.3-codex-spark` is unavailable, fall back to `gpt-5.4-mini`

## Token-Saving Rules

- Start with the smallest relevant path.
- On first pass, inspect only 3 to 5 files.
- Before reading more than 8 files, summarize findings first.
- Prefer `rg`/targeted search over opening large files.
- Read only the sections needed to answer the task.
- Do not inspect these unless clearly necessary or explicitly requested:
  - `ESP32_Datas/`
  - `.pio/`
  - build output directories
  - generated code
  - vendor or third-party source trees
- Do not paste large code blocks or full files unless requested.
- Prefer short summaries, focused diffs, and key snippets.

## Code Change Rules

- State the issue and the smallest safe patch before editing.
- Change only files directly related to the task.
- Do not rewrite whole files without need.
- Do not refactor unrelated code.
- Do not modify generated or vendor files unless explicitly requested.
- Do not touch boot-critical paths unless the task clearly requires it.
- Preserve coding style and existing APIs where possible.
- Prefer explicit, low-risk fixes over clever rewrites.

## Verification Rules

- After firmware changes, run the smallest relevant validation first.
- Prefer targeted build/test commands over full workflows.
- If local validation is not possible, say so clearly.
- When hardware testing is needed, provide concrete board-level checks such as:
  - serial logs
  - display output
  - button/touch input
  - power/reset behavior
  - protocol waveforms when relevant

## High-Risk Areas

Be extra careful with changes involving:

- linker, startup code, or bootloader
- clock tree or interrupt priority
- flash layout or erase/write logic
- watchdog or power management

When evidence is missing, state assumptions clearly and choose the safest minimal path.
