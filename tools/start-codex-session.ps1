param(
    [string]$V2rayNPath = "D:\v2rayN-windows-64\v2rayN.exe",
    [string]$CodexPath = "",
    [string]$ChromePath = "",
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host ("[{0}] {1}" -f (Get-Date -Format "HH:mm:ss"), $Message)
}

function Resolve-ExistingPath {
    param(
        [string]$Name,
        [string[]]$Candidates
    )

    foreach ($candidate in $Candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }

        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "Cannot find $Name. Pass -${Name}Path with the full executable path."
}

function Resolve-CodexPath {
    if (-not [string]::IsNullOrWhiteSpace($CodexPath) -and (Test-Path -LiteralPath $CodexPath)) {
        return (Resolve-Path -LiteralPath $CodexPath).Path
    }

    $command = Get-Command codex.exe -ErrorAction SilentlyContinue
    if ($command -and (Test-Path -LiteralPath $command.Source)) {
        return $command.Source
    }

    $windowsApps = "C:\Program Files\WindowsApps"
    $matches = @()
    if (Test-Path -LiteralPath $windowsApps) {
        $matches = Get-ChildItem -Path $windowsApps -Directory -Filter "OpenAI.Codex_*" -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName "app\resources\codex.exe" } |
            Where-Object { Test-Path -LiteralPath $_ }
    }

    if ($matches.Count -gt 0) {
        return $matches[0]
    }

    throw "Cannot find Codex. Pass -CodexPath with the full executable path."
}

function Start-CodexIfNeeded {
    $existing = Get-Process -Name "codex" -ErrorAction SilentlyContinue
    if ($existing) {
        Write-Step "Codex is already running."
        return
    }

    if ($DryRun) {
        Write-Step "Dry run: would start Codex with app alias, AppsFolder ID, or explicit -CodexPath."
        return
    }

    Write-Step "Starting Codex with app alias."
    try {
        Start-Process -FilePath "codex.exe"
        return
    }
    catch {
    }

    Write-Step "Codex app alias failed; trying AppsFolder ID."
    try {
        Start-Process -FilePath "explorer.exe" -ArgumentList "shell:AppsFolder\OpenAI.Codex_2p2nqsd0c76g0!App"
        return
    }
    catch {
    }

    $resolvedPath = Resolve-CodexPath
    if ($resolvedPath -like "C:\Program Files\WindowsApps\*") {
        throw "Cannot start Codex directly from WindowsApps because Windows denies direct execution. Enable the Codex app alias or pin a shortcut, then run this script again."
    }

    Write-Step "Starting Codex from explicit path."
    Start-Process -FilePath $resolvedPath
}

function Resolve-ChromePath {
    if (-not [string]::IsNullOrWhiteSpace($ChromePath) -and (Test-Path -LiteralPath $ChromePath)) {
        return (Resolve-Path -LiteralPath $ChromePath).Path
    }

    $command = Get-Command chrome.exe -ErrorAction SilentlyContinue
    if ($command -and (Test-Path -LiteralPath $command.Source)) {
        return $command.Source
    }

    $registryCandidates = @(
        "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\chrome.exe",
        "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\chrome.exe"
    )

    foreach ($registryPath in $registryCandidates) {
        try {
            $value = (Get-ItemProperty -Path $registryPath -ErrorAction Stop)."(default)"
            if ($value -and (Test-Path -LiteralPath $value)) {
                return $value
            }
        }
        catch {
        }
    }

    return Resolve-ExistingPath -Name "Chrome" -Candidates @(
        "C:\Program Files\Google\Chrome\Application\chrome.exe",
        "C:\Program Files (x86)\Google\Chrome\Application\chrome.exe"
    )
}

function Start-AppIfNeeded {
    param(
        [string]$Name,
        [string]$Path,
        [string]$ProcessName,
        [string[]]$ArgumentList = @(),
        [System.Diagnostics.ProcessWindowStyle]$WindowStyle = [System.Diagnostics.ProcessWindowStyle]::Normal
    )

    $existing = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue
    if ($existing) {
        Write-Step "$Name is already running."
        return
    }

    if ($DryRun) {
        Write-Step "Dry run: would start $Name from $Path."
        return
    }

    Write-Step "Starting $Name."
    if ($ArgumentList.Count -gt 0) {
        Start-Process -FilePath $Path -ArgumentList $ArgumentList -WindowStyle $WindowStyle
    }
    else {
        Start-Process -FilePath $Path -WindowStyle $WindowStyle
    }
}

Write-Step "Resolving application paths."
$resolvedV2rayNPath = Resolve-ExistingPath -Name "V2rayN" -Candidates @($V2rayNPath)
$resolvedChromePath = Resolve-ChromePath

Write-Step "v2rayN: $resolvedV2rayNPath"
Write-Step "Chrome: $resolvedChromePath"

Start-AppIfNeeded -Name "v2rayN" -Path $resolvedV2rayNPath -ProcessName "v2rayN" -WindowStyle Minimized
Write-Step "Skipping network wait."

Start-CodexIfNeeded

$chromeUrls = @("https://chatgpt.com/", "https://github.com/")
if ($DryRun) {
    Write-Step "Dry run: would open Chrome URLs: $($chromeUrls -join ', ')"
}
else {
    Write-Step "Opening ChatGPT and GitHub in Chrome."
    Start-Process -FilePath $resolvedChromePath -ArgumentList $chromeUrls
}

Write-Step "Startup sequence complete."
