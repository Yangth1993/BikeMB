param(
  [switch]$Update
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SimulatorDir = Join-Path $PSScriptRoot "lv_port_pc_vscode"
$SimulatorRepo = "https://github.com/lvgl/lv_port_pc_vscode.git"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
  Write-Error "git was not found. Install git and retry."
}

if (Test-Path $SimulatorDir) {
  $GitDir = Join-Path $SimulatorDir ".git"
  if (-not (Test-Path $GitDir)) {
    Write-Error "Found $SimulatorDir, but it is not a git checkout. Move it aside before retrying."
  }

  if ($Update) {
    Push-Location $SimulatorDir
    try {
      git pull --ff-only
      git submodule update --init --recursive --depth 1
    } finally {
      Pop-Location
    }
  } else {
    Write-Host "LVGL simulator already exists: $SimulatorDir"
    Write-Host "Use tools\setup-lvgl-simulator.ps1 -Update to update it."
  }
  exit 0
}

Write-Host "Downloading official LVGL PC simulator:"
Write-Host "  $SimulatorRepo"
git clone --depth 1 --recurse-submodules --shallow-submodules $SimulatorRepo $SimulatorDir

Write-Host ""
Write-Host "LVGL simulator ready: $SimulatorDir"
Write-Host "Next: tools\open-lvgl-simulator.ps1 -Build"
