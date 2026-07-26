param(
    [string]$Source = "C:\Users\WINDOWS\.cache\codex-runtimes",
    [string]$Target = "D:\CodexCache\codex-runtimes"
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host ("[{0}] {1}" -f (Get-Date -Format "HH:mm:ss"), $Message)
}

$running = Get-Process -Name "codex", "Code" -ErrorAction SilentlyContinue
if ($running) {
    Write-Host "Codex or VSCode is still running. Close them first, then run this script again."
    $running | Select-Object ProcessName, Id, Path | Format-Table -AutoSize
    exit 1
}

$allowedSourceRoot = "C:\Users\WINDOWS\.cache\"
$allowedTargetRoot = "D:\CodexCache\"

if (-not $Source.StartsWith($allowedSourceRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing source outside allowed root: $Source"
}

if (-not $Target.StartsWith($allowedTargetRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing target outside allowed root: $Target"
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Target) | Out-Null

if (Test-Path -LiteralPath $Source) {
    $sourceItem = Get-Item -LiteralPath $Source -Force
    if ($sourceItem.LinkType) {
        Write-Step "Already linked: $Source -> $($sourceItem.Target -join ',')"
        exit 0
    }

    if (Test-Path -LiteralPath $Target) {
        Write-Step "Merging existing cache into $Target"
        Get-ChildItem -LiteralPath $Source -Force -ErrorAction SilentlyContinue |
            ForEach-Object { Move-Item -LiteralPath $_.FullName -Destination $Target -Force }
        Remove-Item -LiteralPath $Source -Force
    }
    else {
        Write-Step "Moving $Source to $Target"
        Move-Item -LiteralPath $Source -Destination $Target -Force
    }
}
else {
    Write-Step "Source cache does not exist; creating target directory."
    New-Item -ItemType Directory -Force -Path $Target | Out-Null
}

Write-Step "Creating junction: $Source -> $Target"
New-Item -ItemType Junction -Path $Source -Target $Target | Out-Null

Write-Step "Done."
