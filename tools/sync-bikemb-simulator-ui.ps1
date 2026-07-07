$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SimulatorDir = Join-Path $PSScriptRoot "lv_port_pc_vscode"
$SourceUiDir = Join-Path $RepoRoot "simulator\bikemb_ui"
$SharedUiDir = Join-Path $RepoRoot "firmware\bikemb\src\app"
$TargetUiDir = Join-Path $SimulatorDir "ui"
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
Copy-Item -Force -LiteralPath (Join-Path $SourceUiDir "bikemb_dashboard.c") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SourceUiDir "bikemb_dashboard.h") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SharedUiDir "dashboard_view_core.c") -Destination $TargetUiDir
Copy-Item -Force -LiteralPath (Join-Path $SharedUiDir "dashboard_view_core.h") -Destination $TargetUiDir

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
