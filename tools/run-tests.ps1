param(
  [switch]$SmokeBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$TestRoot = Join-Path $RepoRoot "tools\tests"
$env:PYTHONDONTWRITEBYTECODE = "1"

if (Get-Command py -ErrorAction SilentlyContinue) {
  $PythonExe = "py"
  $PythonArgs = @("-X", "utf8")
} elseif (Get-Command python -ErrorAction SilentlyContinue) {
  $PythonExe = "python"
  $PythonArgs = @()
} else {
  Write-Error "Python was not found. Install Python or add it to PATH."
}

$Tests = Get-ChildItem -Path $TestRoot -Filter "test_*.py" | Sort-Object Name
if ($Tests.Count -eq 0) {
  Write-Error "No lightweight tests found in $TestRoot."
}

$Failed = @()

foreach ($Test in $Tests) {
  Write-Host "[RUN ] $($Test.Name)"
  & $PythonExe @PythonArgs $Test.FullName
  if ($LASTEXITCODE -eq 0) {
    Write-Host "[PASS] $($Test.Name)"
  } else {
    Write-Host "[FAIL] $($Test.Name)"
    $Failed += $Test.Name
  }
}

if ($SmokeBuild) {
  Write-Host "[RUN ] platformio run smoke build"
  $FirmwareRoot = Join-Path $RepoRoot "firmware\bikemb"
  Push-Location $FirmwareRoot
  try {
    & $PythonExe @PythonArgs -m platformio run
    if ($LASTEXITCODE -eq 0) {
      Write-Host "[PASS] platformio run smoke build"
    } else {
      Write-Host "[FAIL] platformio run smoke build"
      $Failed += "platformio run smoke build"
    }
  } finally {
    Pop-Location
  }
}

if ($Failed.Count -gt 0) {
  Write-Host ""
  Write-Host "BikeMB lightweight tests failed: $($Failed -join ', ')"
  exit 1
}

Write-Host ""
Write-Host "BikeMB lightweight tests passed: $($Tests.Count) contract test(s)."
if (-not $SmokeBuild) {
  Write-Host "Smoke build skipped. Run tools\run-tests.ps1 -SmokeBuild when you want platformio run."
}
