$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

& (Join-Path $RepoRoot "tools\generate-lvgl-fonts.ps1")
& (Join-Path $RepoRoot "tools\generate-lvgl-images.ps1")

Write-Host "BikeMB LVGL visual assets generated."
