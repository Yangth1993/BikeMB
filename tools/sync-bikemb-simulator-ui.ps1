$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SimulatorDir = Join-Path $PSScriptRoot "lv_port_pc_vscode"
$SourceUiDir = Join-Path $RepoRoot "tools\simulator\bikemb_ui"
$SharedUiDir = Join-Path $RepoRoot "src\firmware\bikemb\src\app"
$SharedAssetsDir = Join-Path $SharedUiDir "assets"
$TargetUiDir = Join-Path $SimulatorDir "ui"
$TargetAssetsDir = Join-Path $TargetUiDir "assets"
$TargetDriversDir = Join-Path $SimulatorDir "drivers"
$MainPath = Join-Path $SimulatorDir "main\src\main.c"

if (-not (Test-Path $SimulatorDir)) {
  Write-Error "LVGL simulator is missing. Run tools\setup-lvgl-simulator.ps1 first."
}

if (-not (Test-Path $SourceUiDir)) {
  Write-Error "BikeMB simulator UI source is missing: $SourceUiDir"
}

if (-not (Test-Path $MainPath)) {
  Write-Error "Official simulator main.c was not found: $MainPath"
}

New-Item -ItemType Directory -Force -Path $TargetUiDir | Out-Null
New-Item -ItemType Directory -Force -Path $TargetAssetsDir | Out-Null
New-Item -ItemType Directory -Force -Path $TargetDriversDir | Out-Null
Remove-Item -Force -ErrorAction SilentlyContinue -LiteralPath `
  (Join-Path $TargetAssetsDir "dashboard_font_speed_88.c"), `
  (Join-Path $TargetAssetsDir "dashboard_font_speed_112.c")
Copy-Item -Force -LiteralPath (Join-Path $SourceUiDir "bikemb_dashboard.c") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SourceUiDir "bikemb_dashboard.h") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SharedUiDir "dashboard_view_core.c") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SharedUiDir "dashboard_view_core.h") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SharedUiDir "dashboard_pages.c") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SharedUiDir "dashboard_pages.h") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SharedUiDir "dashboard_ui_style.c") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SharedUiDir "dashboard_ui_style.h") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SharedAssetsDir "dashboard_assets.h") -Destination $TargetAssetsDir
Copy-Item -Force -LiteralPath (Join-Path $SharedAssetsDir "dashboard_font_speed_140.c") -Destination $TargetAssetsDir
Copy-Item -Force -LiteralPath (Join-Path $SharedAssetsDir "dashboard_font_speed_decimal_96.c") -Destination $TargetAssetsDir
Copy-Item -Force -LiteralPath (Join-Path $SharedAssetsDir "dashboard_font_output_80.c") -Destination $TargetAssetsDir
Copy-Item -Force -LiteralPath (Join-Path $SharedAssetsDir "dashboard_img_home_assist_glow.c") -Destination $TargetAssetsDir
Copy-Item -Force -LiteralPath (Join-Path $SharedAssetsDir "dashboard_img_home_bezel.c") -Destination $TargetAssetsDir

$TouchStub = @"
#pragma once

typedef enum Cst816Gesture {
  CST816_GESTURE_NONE = 0x00,
  CST816_GESTURE_SWIPE_UP = 0x01,
  CST816_GESTURE_SWIPE_DOWN = 0x02,
  CST816_GESTURE_SWIPE_LEFT = 0x03,
  CST816_GESTURE_SWIPE_RIGHT = 0x04,
  CST816_GESTURE_SINGLE_CLICK = 0x05,
  CST816_GESTURE_DOUBLE_CLICK = 0x0B,
  CST816_GESTURE_LONG_PRESS = 0x0C,
} Cst816Gesture;

static inline Cst816Gesture TouchCst816_ConsumeGesture(void) {
  return CST816_GESTURE_NONE;
}
"@
Set-Content -LiteralPath (Join-Path $TargetDriversDir "Touch_CST816.h") -Value $TouchStub -Encoding ASCII

$Main = Get-Content -Raw -LiteralPath $MainPath

if ($Main -notmatch '#include "ui/bikemb_dashboard.h"') {
  $Main = $Main -replace '#include "lvgl/demos/lv_demos.h"', "#include `"lvgl/demos/lv_demos.h`"`r`n#include `"ui/bikemb_dashboard.h`""
}

if ($Main -notmatch '#ifndef BIKEMB_SIMULATOR_UI') {
  $Main = $Main -replace '#include "ui/bikemb_dashboard.h"', "#include `"ui/bikemb_dashboard.h`"`r`n#ifndef BIKEMB_SIMULATOR_UI`r`n#define BIKEMB_SIMULATOR_UI 1`r`n#endif"
}

$EntryBlock = @"
#if BIKEMB_SIMULATOR_UI
  bikemb_dashboard_create();
#else
  lv_demo_widgets();
#endif
"@

if ($Main -notmatch 'bikemb_dashboard_create\(\);') {
  $Main = $Main -replace '(?m)^\s*lv_demo_widgets\(\);', $EntryBlock
}

Set-Content -LiteralPath $MainPath -Value $Main -Encoding utf8
Write-Host "BikeMB simulator UI synced into official LVGL simulator checkout."
