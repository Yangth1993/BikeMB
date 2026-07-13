# BikeMB Software Architecture

This document captures the current BikeMB firmware architecture for quick agent and developer reference.

## Layered Firmware Architecture

```mermaid
flowchart BT
  subgraph L0["L0 Hardware"]
    HW_MCU["ESP32-S3R8<br/>dual core MCU<br/>16MB flash / 8MB PSRAM board profile"]
    HW_LCD["ST77916 round LCD<br/>360 x 360<br/>SPI/QSPI panel path"]
    HW_IO["GPIO / backlight / reset / power / I2C expander"]
  end

  subgraph L1["L1 ESP-IDF HAL + FreeRTOS"]
    IDF["ESP-IDF framework<br/>app_main()<br/>ESP_LOGx"]
    RTOS["FreeRTOS primitives<br/>task / queue / tick delay"]
    IDF_LCD["esp_lcd<br/>panel IO + draw bitmap"]
    IDF_TIME["esp_timer_get_time()<br/>heap_caps APIs"]
  end

  subgraph L1B["Fallback Runtime"]
    ARDUINO["Arduino env retained<br/>setup() / loop()<br/>known stable fallback"]
  end

  subgraph L2["L2 BSP"]
    BSP["BoardSupport_Init()<br/>board bring-up boundary"]
    BSP_LCD["ST77916_Init()<br/>Backlight_Init()"]
    BSP_DIAG["DisplayDiagnostics_Run()<br/>BIKE_MB_RUN_DISPLAY_DIAGNOSTIC"]
  end

  subgraph L3["L3 Drivers"]
    LCDDRV["Display_ST77916<br/>LCD_addWindow()<br/>RGB565 byte-swap<br/>sync flush contract"]
    PANEL["esp_lcd_st77916<br/>panel command sequence"]
    I2C["I2C_Driver + TCA9554PWR<br/>Arduino and ESP-IDF paths"]
  end

  subgraph L4["L4 Platform + LVGL Port"]
    PLATFORM["bike_platform.h<br/>millis/micros/delay/log/heap/GPIO wrappers"]
    LVGLPORT["lvgl_port<br/>lv_init()<br/>static lv_disp_drv_t<br/>draw buffers"]
    FLUSH["FlushCallback()<br/>LCD_addWindow()<br/>lv_disp_flush_ready()"]
    LVGL["LVGL 8.4<br/>widgets / styles / fonts<br/>single UI owner rule"]
  end

  subgraph L5["L5 BikeMB Runtime"]
    RUNTIME["BikeRuntime_Init()<br/>BikeRuntime_Start()"]
    QUEUE["Bike Event Queue<br/>fixed FreeRTOS queue<br/>drop low-priority ticks when full"]
    EVENTS["BikeEvent<br/>SystemTick / DashboardTick<br/>RenderStatsUpdate / DiagnosticRequest"]
    TICKTASK["RuntimeTickTask<br/>posts periodic events"]
  end

  subgraph L6["L6 Services"]
    UISVC["UiService<br/>only task allowed to call LVGL<br/>owns LvglPort + DashboardApp"]
    METRICSVC["MetricsService<br/>demo metrics + render stats aggregation"]
  end

  subgraph L7["L7 App / UI"]
    APP["DashboardApp<br/>event/model update boundary"]
    VIEW["dashboard_view.cpp<br/>firmware adapter"]
    UICORE["dashboard_view_core.c/h<br/>shared dashboard UI core"]
  end

  subgraph DEV["Development + Verification"]
    SIM["Official LVGL PC simulator<br/>ui/ mechanism"]
    TESTS["tools/tests<br/>contract tests for driver, LVGL, runtime, config"]
    PIO["PlatformIO<br/>Arduino fallback env<br/>ESP-IDF migration env"]
    BUILDS["repo-local cache/build<br/>.pio-home<br/>build/pio-bikemb<br/>build/pio-libdeps"]
  end

  HW_MCU --> IDF
  HW_MCU --> ARDUINO
  HW_LCD --> IDF_LCD
  HW_IO --> BSP

  IDF --> RTOS
  IDF --> IDF_TIME
  RTOS --> RUNTIME
  IDF_LCD --> LCDDRV
  IDF_TIME --> PLATFORM
  ARDUINO --> PLATFORM

  PLATFORM --> BSP
  BSP --> BSP_LCD
  BSP --> BSP_DIAG
  BSP_LCD --> LCDDRV
  I2C --> BSP
  LCDDRV --> PANEL

  PANEL --> FLUSH
  LCDDRV --> FLUSH
  FLUSH --> LVGLPORT
  LVGLPORT --> LVGL
  PLATFORM --> LVGLPORT

  RUNTIME --> QUEUE
  TICKTASK --> QUEUE
  EVENTS --> QUEUE
  QUEUE --> UISVC
  QUEUE --> METRICSVC

  UISVC --> LVGLPORT
  UISVC --> APP
  METRICSVC --> APP
  APP --> VIEW
  VIEW --> UICORE
  LVGL --> UICORE

  SIM --> UICORE
  TESTS -. validates .-> LCDDRV
  TESTS -. validates .-> LVGLPORT
  TESTS -. validates .-> RUNTIME
  TESTS -. validates .-> PIO
  PIO --> BUILDS
```

## Runtime Flow

```mermaid
sequenceDiagram
  participant Main as app_main()
  participant Runtime as BikeRuntime
  participant Queue as Bike Event Queue
  participant UI as UiService task
  participant Metrics as MetricsService
  participant App as DashboardApp
  participant LVGL as LVGL
  participant LCD as LCD_addWindow/ST77916

  Main->>Runtime: BikeRuntime_Init()
  Runtime->>Runtime: BoardSupport_Init()
  Main->>Runtime: BikeRuntime_Start()
  Runtime->>UI: xTaskCreatePinnedToCore()
  Runtime->>Runtime: start RuntimeTickTask
  Runtime->>Queue: post DashboardTick / SystemTick
  UI->>Queue: consume UI events
  UI->>App: DashboardApp_Tick(timestampMs)
  App->>Metrics: MetricsService_UpdateDashboard(renderWorkMs)
  Metrics-->>App: fps/cpu/heap/orb model
  App->>LVGL: update shared dashboard view
  UI->>LVGL: LvglPort_Tick(deltaMs)
  UI->>LVGL: LvglPort_Run()
  LVGL->>LCD: FlushCallback(area, color)
  LCD-->>LVGL: lv_disp_flush_ready()
  UI->>App: DashboardApp_SetRenderWorkMs(renderWorkMs)
```

## Current Migration State

- The Arduino PlatformIO env remains the default and the stable hardware fallback.
- The ESP-IDF PlatformIO env builds and provides the new `app_main()` Runtime/Event Bus/Service skeleton.
- LVGL remains single-owner: only `UiService` should call LVGL APIs in the ESP-IDF runtime.
- The LCD path still uses the stable synchronous flush contract: `FlushCallback()` -> `LCD_addWindow()` -> `lv_disp_flush_ready()`.
- The PC simulator continues to share `dashboard_view_core.c/h` through the official LVGL simulator `ui/` mechanism.
- Before flashing the ESP-IDF env, finish the IDF board sdkconfig pass for flash size, PSRAM, CPU frequency, and LVGL examples/demos pruning.

## How To View

This file is Markdown with Mermaid diagrams.

- VS Code or Cursor: open this file, then use Markdown Preview.
- GitHub: Mermaid diagrams render automatically when the file is pushed.
- Typora or Obsidian: enable Mermaid support if it is disabled.
