# BikeMB Software Architecture

This document captures the current BikeMB firmware architecture for quick agent and developer reference.

## Layer Diagram

```mermaid
flowchart TD
  HW["Hardware<br/>ESP32-S3<br/>ST77916 round LCD<br/>Backlight / GPIO / QSPI-SPI LCD"]

  BSP["Board Support<br/>BoardSupport_Init()<br/>ST77916_Init()<br/>Backlight_Init()"]

  LCD["LCD Driver<br/>Display_ST77916.cpp<br/>LCD_addWindow()<br/>RGB565 byte-swap<br/>esp_lcd_panel_draw_bitmap()"]

  LVGLPORT["LVGL Port<br/>lvgl_port.cpp<br/>lv_init()<br/>static lv_disp_drv_t<br/>internal RAM double buffers<br/>FlushCallback()<br/>LvglPort_Tick()<br/>LvglPort_Run()"]

  LVGL["LVGL Core<br/>lv_timer_handler()<br/>draw buffer<br/>invalid areas<br/>widget rendering"]

  APP["Application Scheduler<br/>main.cpp<br/>Arduino setup()/loop()<br/>DashboardApp_Tick()<br/>stable 33ms dashboard cadence"]

  METRICS["Demo Metrics<br/>demo_metrics.cpp<br/>FPS counter<br/>GUI render-budget load<br/>heap / psram<br/>orb position"]

  LOGGING["Serial / Diagnostics<br/>startup logs only in normal dashboard<br/>no periodic Serial.printf in hot path<br/>display diagnostic guarded by BIKE_MB_RUN_DISPLAY_DIAGNOSTIC"]

  VIEWWRAP["Firmware UI Adapter<br/>dashboard_view.cpp<br/>DemoMetrics to BikeMbDashboardMetrics"]

  UICORE["Shared UI Core<br/>dashboard_view_core.c/h<br/>dashboard widgets<br/>change-guarded labels/bars<br/>round background + non-clipping content layer"]

  SIMTOOLS["PC Simulator Tools<br/>setup-lvgl-simulator.ps1<br/>open-lvgl-simulator.ps1<br/>sync-bikemb-simulator-ui.ps1"]

  SIM["Official LVGL Simulator<br/>tools/lv_port_pc_vscode/ ignored<br/>ui/bikemb_dashboard.c<br/>ui/dashboard_view_core.c"]

  TESTS["Lightweight Contract Tests<br/>tools/tests/<br/>driver / LVGL port / simulator / performance contracts"]

  HW --> BSP --> LCD --> LVGLPORT --> LVGL
  APP --> LVGLPORT
  APP --> METRICS --> VIEWWRAP --> UICORE --> LVGL
  APP --> LOGGING
  LVGLPORT --> LCD
  SIMTOOLS --> SIM --> UICORE
  TESTS -. validates .-> LCD
  TESTS -. validates .-> LVGLPORT
  TESTS -. validates .-> UICORE
  TESTS -. validates .-> SIMTOOLS
```

## Runtime Flow

```mermaid
sequenceDiagram
  participant Loop as Arduino loop()
  participant App as DashboardApp
  participant Metrics as DemoMetrics
  participant View as DashboardView/Core
  participant LVGL as LVGL
  participant LCD as LCD_addWindow/ST77916

  Loop->>LVGL: LvglPort_Tick(deltaMs)
  Loop->>App: DashboardApp_Tick(now)
  App->>Metrics: DemoMetrics_Update(renderWorkMs)
  Metrics-->>App: fps/cpu/heap/orb
  App->>View: DashboardView_Update(metrics)
  View->>LVGL: update changed labels/bars + orb position
  Loop->>LVGL: LvglPort_Run()
  LVGL->>LCD: FlushCallback(area, color)
  LCD-->>LVGL: lv_disp_flush_ready()
  LVGL-->>Loop: renderWorkMs
  Loop->>App: DashboardApp_SetRenderWorkMs(renderWorkMs)
```

## Notes

- The LCD driver currently uses a synchronous flush contract. Do not push refresh cadence aggressively until the LCD transfer path is measured or made asynchronous.
- The dashboard CPU number is a GUI render-budget estimate, not a FreeRTOS system-wide CPU utilization metric.
- The dashboard hot path must not emit periodic serial logs. USB serial backpressure can stall the main loop and make FPS collapse after the first logging interval.
- The proven board cadence is currently the stable 33ms dashboard update interval. 16ms/60Hz needs async flush or measured LCD transfer evidence first.
- `dashboard_view_core.c/h` is shared by firmware and the official PC LVGL simulator.
- `tools/lv_port_pc_vscode/` is an ignored official checkout and should not be committed.

## How To View

This file is Markdown with Mermaid diagrams.

Recommended viewers:

- VS Code or Cursor: open this file, then use Markdown Preview.
- GitHub: Mermaid diagrams render automatically when the file is pushed.
- Typora or Obsidian: both can render Markdown; enable Mermaid support if it is disabled.

If a viewer shows the Mermaid code block as plain text, install or enable Mermaid preview support for that editor.
