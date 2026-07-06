param(
  [string]$Driver = "SDL2",
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

function Convert-ToMsysPath($WindowsPath) {
  $Resolved = (Resolve-Path $WindowsPath).Path
  $Drive = $Resolved.Substring(0, 1).ToLowerInvariant()
  $Tail = $Resolved.Substring(2).Replace("\", "/")
  return "/$Drive$Tail"
}

$MsysBash = "C:\msys64\usr\bin\bash.exe"
if (-not (Test-Path $MsysBash)) {
  Write-Error "MSYS2 bash was not found at $MsysBash. Install MSYS2 and retry."
}

Push-Location $SimulatorDir
try {
  $MsysSimulatorDir = Convert-ToMsysPath $SimulatorDir
  & $MsysBash -lc "export PATH=/mingw64/bin:`$PATH && cd '$MsysSimulatorDir' && make LV_DRIVER=$Driver CC=gcc"
  if ($LASTEXITCODE -ne 0) {
    throw "official LVGL simulator Makefile build failed with exit code ${LASTEXITCODE}"
  }

  $Executables = @()
  $Executables += Get-ChildItem -Path "build\bin" -Filter "*.exe" -ErrorAction SilentlyContinue
  $Executables += Get-ChildItem -Path "build\bin" -Filter "demo" -ErrorAction SilentlyContinue
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
