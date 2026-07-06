param(
  [switch]$Build
)

$ErrorActionPreference = "Stop"

$SimulatorDir = Join-Path $PSScriptRoot "lv_port_pc_vscode"

if (-not (Test-Path $SimulatorDir)) {
  Write-Error "LVGL simulator is missing. Run tools\setup-lvgl-simulator.ps1 first."
}

if (-not $Build) {
  Write-Host "LVGL simulator checkout:"
  Write-Host "  $SimulatorDir"
  Write-Host ""
  Write-Host "Build and open it with:"
  Write-Host "  tools\open-lvgl-simulator.ps1 -Build"
  exit 0
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
  Write-Error "cmake was not found. Install CMake and retry."
}

Push-Location $SimulatorDir
try {
  cmake -S . -B build
  cmake --build build

  $Executables = Get-ChildItem -Path "build" -Recurse -Filter "*.exe" | Sort-Object LastWriteTime -Descending
  if ($Executables.Count -eq 0) {
    Write-Host "Build finished, but no .exe was found under $SimulatorDir\build."
    exit 0
  }

  $App = $Executables[0].FullName
  Write-Host "Opening LVGL simulator:"
  Write-Host "  $App"
  Start-Process -FilePath $App
} finally {
  Pop-Location
}
