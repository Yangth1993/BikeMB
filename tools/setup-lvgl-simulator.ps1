param(
  [string]$Branch = "release/v8",
  [switch]$UseSsh,
  [switch]$Update
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SimulatorDir = Join-Path $PSScriptRoot "lv_port_pc_vscode"
$SimulatorHttpsRepo = "https://github.com/lvgl/lv_port_pc_vscode.git"
$SimulatorSshRepo = "git@github.com:lvgl/lv_port_pc_vscode.git"
$SimulatorRepo = if ($UseSsh) { $SimulatorSshRepo } else { $SimulatorHttpsRepo }

function Invoke-CheckedGit {
  git @args
  if ($LASTEXITCODE -ne 0) {
    throw "git command failed with exit code ${LASTEXITCODE}: git $($args -join ' ')"
  }
}

function Set-OfficialSubmoduleUrls {
  if ($UseSsh) {
    Invoke-CheckedGit submodule set-url lv_drivers git@github.com:lvgl/lv_drivers.git
    Invoke-CheckedGit submodule set-url lvgl git@github.com:lvgl/lvgl.git
  }
}

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
      Invoke-CheckedGit pull --ff-only
      Set-OfficialSubmoduleUrls
      Invoke-CheckedGit submodule update --init --recursive --depth 1
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
Write-Host "  branch: $Branch"
Invoke-CheckedGit clone --depth 1 --branch $Branch $SimulatorRepo $SimulatorDir

Push-Location $SimulatorDir
try {
  Set-OfficialSubmoduleUrls
  Invoke-CheckedGit submodule update --init --recursive --depth 1
} finally {
  Pop-Location
}

Write-Host ""
Write-Host "LVGL simulator ready: $SimulatorDir"
Write-Host "Next: tools\open-lvgl-simulator.ps1 -Build"
