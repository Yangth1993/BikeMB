$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$FontConv = Join-Path $RepoRoot "node_modules\.bin\lv_font_conv.cmd"
$FontSource = Join-Path $RepoRoot "node_modules\@fontsource\rajdhani\files\rajdhani-latin-700-normal.woff"
$OutputDir = Join-Path $RepoRoot "src\firmware\bikemb\src\app\assets"
$Fonts = @(
  @{
    Output = Join-Path $OutputDir "dashboard_font_speed_140.c"
    Size = 140
    Symbol = "bike_mb_font_speed_140"
  },
  @{
    Output = Join-Path $OutputDir "dashboard_font_speed_decimal_96.c"
    Size = 96
    Symbol = "bike_mb_font_speed_decimal_96"
  },
  @{
    Output = Join-Path $OutputDir "dashboard_font_output_80.c"
    Size = 80
    Symbol = "bike_mb_font_output_80"
  }
)

if (-not (Test-Path $FontConv)) {
  Write-Error "lv_font_conv is missing. Run: cmd /c npm install"
}

if (-not (Test-Path $FontSource)) {
  Write-Error "Rajdhani font source is missing: $FontSource"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

foreach ($Font in $Fonts) {
  & $FontConv `
    --font $FontSource `
    --symbols "0123456789." `
    --size $Font.Size `
    --bpp 4 `
    --format lvgl `
    --lv-include "lvgl.h" `
    --lv-font-name $Font.Symbol `
    --output $Font.Output

  if ($LASTEXITCODE -ne 0) {
    throw "lv_font_conv failed with exit code $LASTEXITCODE"
  }

  $Generated = Get-Content -Raw -LiteralPath $Font.Output
  $Generated = $Generated.Replace($RepoRoot, "<repo>")
  [System.IO.File]::WriteAllText($Font.Output, $Generated, [System.Text.UTF8Encoding]::new($false))

  Write-Host "Generated LVGL speed font: $($Font.Output)"
}
