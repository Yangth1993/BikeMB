$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Node20 = Join-Path $RepoRoot "node_modules\node\bin\node.exe"
$TsNodeRegister = "ts-node/register/transpile-only"
$TsNodeRegisterFile = Join-Path $RepoRoot "node_modules\ts-node\register\transpile-only.js"
$ImageConv = Join-Path $RepoRoot "node_modules\lv_img_conv\lib\cli.ts"
$SourceDir = Join-Path $RepoRoot "src\assets\source\ui"
$OutputDir = Join-Path $RepoRoot "src\firmware\bikemb\src\app\assets"
$Images = @(
  @{
    Source = Join-Path $SourceDir "home_assist_glow.png"
    Output = Join-Path $OutputDir "dashboard_img_home_assist_glow.c"
    Symbol = "bike_mb_img_home_assist_glow"
  },
  @{
    Source = Join-Path $SourceDir "home_bezel.png"
    Output = Join-Path $OutputDir "dashboard_img_home_bezel.c"
    Symbol = "bike_mb_img_home_bezel"
  }
)

if (-not (Test-Path $Node20) -or -not (Test-Path $ImageConv) -or -not (Test-Path $TsNodeRegisterFile)) {
  Write-Error "The pinned LVGL image converter runtime is missing. Run: cmd /c npm install"
}

foreach ($Image in $Images) {
  if (-not (Test-Path $Image.Source)) {
    Write-Error "Source image is missing: $($Image.Source)"
  }
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$env:TS_NODE_COMPILER_OPTIONS = '{"module":"CommonJS","moduleResolution":"Node"}'
$env:TS_NODE_SKIP_IGNORE = "true"

foreach ($Image in $Images) {
  Push-Location $RepoRoot
  try {
    & $Node20 `
      -r $TsNodeRegister `
      -e "require(process.argv[1])" `
      $ImageConv `
      $Image.Source `
      --force `
      --color-format CF_ALPHA_8_BIT `
      --output-format c `
      --output-file $Image.Output `
      --image-name $Image.Symbol
  } finally {
    Pop-Location
  }

  if ($LASTEXITCODE -ne 0) {
    throw "lv_img_conv failed with exit code $LASTEXITCODE"
  }

  Write-Host "Generated LVGL image: $($Image.Output)"
}
