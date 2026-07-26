param(
    [switch]$PlanOnly,
    [switch]$MoveCaches,
    [switch]$CleanOldTemp,
    [switch]$IncludeOpenAILocalRuntime,
    [int]$OldTempDays = 7
)

$ErrorActionPreference = "Stop"

if (-not $PlanOnly -and -not $MoveCaches -and -not $CleanOldTemp -and -not $IncludeOpenAILocalRuntime) {
    $PlanOnly = $true
}

$UserProfile = "C:\Users\WINDOWS"
$TempRoot = Join-Path $UserProfile "AppData\Local\Temp"

$MoveMappings = @(
    @{
        Name = "Codex runtime cache"
        Source = Join-Path $UserProfile ".cache\codex-runtimes"
        Target = "D:\CodexCache\codex-runtimes"
        RequiresClosed = @("codex", "Code")
        Optional = $false
    },
    @{
        Name = "PlatformIO home"
        Source = Join-Path $UserProfile ".platformio"
        Target = "D:\DevCache\platformio"
        RequiresClosed = @()
        Optional = $false
    },
    @{
        Name = "npm cache"
        Source = Join-Path $UserProfile "AppData\Local\npm-cache"
        Target = "D:\DevCache\npm-cache"
        RequiresClosed = @()
        Optional = $false
    },
    @{
        Name = "pip cache"
        Source = Join-Path $UserProfile "AppData\Local\pip\Cache"
        Target = "D:\DevCache\pip-cache"
        RequiresClosed = @()
        Optional = $false
    },
    @{
        Name = "Thunder Network temp"
        Source = Join-Path $TempRoot "Thunder Network"
        Target = "D:\CDriveSlimming\TempMoved\Thunder Network"
        RequiresClosed = @("Thunder", "ThunderBrowser", "DownloadSDKServer", "XLServicePlatform")
        Optional = $false
    },
    @{
        Name = "OpenAI Codex local runtime"
        Source = Join-Path $UserProfile "AppData\Local\OpenAI\Codex"
        Target = "D:\CodexCache\local-openai-codex"
        RequiresClosed = @("codex", "Code")
        Optional = $true
    }
)

$AllowedMoves = @(
    @{ SourceRoot = Join-Path $UserProfile ".cache"; TargetRoot = "D:\CodexCache" },
    @{ SourceRoot = Join-Path $UserProfile ".platformio"; TargetRoot = "D:\DevCache" },
    @{ SourceRoot = Join-Path $UserProfile "AppData\Local\npm-cache"; TargetRoot = "D:\DevCache" },
    @{ SourceRoot = Join-Path $UserProfile "AppData\Local\pip\Cache"; TargetRoot = "D:\DevCache" },
    @{ SourceRoot = Join-Path $TempRoot "Thunder Network"; TargetRoot = "D:\CDriveSlimming\TempMoved" },
    @{ SourceRoot = Join-Path $UserProfile "AppData\Local\OpenAI\Codex"; TargetRoot = "D:\CodexCache" }
)

function Write-Step {
    param([string]$Message)
    Write-Host ("[{0}] {1}" -f (Get-Date -Format "HH:mm:ss"), $Message)
}

function Get-DirectorySizeBytes {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        return 0
    }

    $sum = (Get-ChildItem -LiteralPath $Path -Recurse -Force -File -ErrorAction SilentlyContinue |
        Measure-Object Length -Sum).Sum
    if ($null -eq $sum) {
        return 0
    }
    return [int64]$sum
}

function Format-Size {
    param([int64]$Bytes)
    if ($Bytes -ge 1GB) {
        return ("{0:N2} GB" -f ($Bytes / 1GB))
    }
    return ("{0:N1} MB" -f ($Bytes / 1MB))
}

function Assert-MoveIsAllowed {
    param(
        [string]$Source,
        [string]$Target
    )

    $allowed = $false
    foreach ($rule in $AllowedMoves) {
        if ($Source.StartsWith($rule.SourceRoot, [System.StringComparison]::OrdinalIgnoreCase) -and
            $Target.StartsWith($rule.TargetRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            $allowed = $true
            break
        }
    }

    if (-not $allowed) {
        throw "Refusing move outside allowed cache roots: $Source -> $Target"
    }
}

function Get-RunningBlockingProcesses {
    param([string[]]$Names)
    if ($Names.Count -eq 0) {
        return @()
    }

    return @(Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $Names -contains $_.ProcessName } |
        Select-Object ProcessName, Id, Path)
}

function Move-ToJunction {
    param(
        [hashtable]$Map
    )

    if ($Map.Optional -and -not $IncludeOpenAILocalRuntime) {
        Write-Step "Skip optional: $($Map.Name)"
        return
    }

    $source = $Map.Source
    $target = $Map.Target
    Assert-MoveIsAllowed -Source $source -Target $target

    if (-not (Test-Path -LiteralPath $source)) {
        Write-Step "Missing source, creating junction target only: $source"
        New-Item -ItemType Directory -Force -Path $target | Out-Null
        New-Item -ItemType Junction -Path $source -Target $target | Out-Null
        return
    }

    $sourceItem = Get-Item -LiteralPath $source -Force
    if ($sourceItem.LinkType) {
        Write-Step "Already linked: $source -> $($sourceItem.Target -join ',')"
        return
    }

    $blockers = Get-RunningBlockingProcesses -Names $Map.RequiresClosed
    if ($blockers.Count -gt 0) {
        Write-Warning "Skip $($Map.Name): close these processes first."
        $blockers | Format-Table -AutoSize
        return
    }

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null

    if (Test-Path -LiteralPath $target) {
        Write-Step "Merging $source into existing $target"
        Get-ChildItem -LiteralPath $source -Force -ErrorAction SilentlyContinue |
            ForEach-Object { Move-Item -LiteralPath $_.FullName -Destination $target -Force -ErrorAction Stop }
        Remove-Item -LiteralPath $source -Force
    }
    else {
        Write-Step "Moving $source to $target"
        Move-Item -LiteralPath $source -Destination $target -Force
    }

    Write-Step "Creating junction: $source -> $target"
    New-Item -ItemType Junction -Path $source -Target $target | Out-Null
}

function Get-OldTempCandidates {
    $cutoff = (Get-Date).AddDays(-1 * $OldTempDays)
    $patterns = @("*.tmp", "patch_*.exe", "nsi*.tmp", "ns*.tmp")
    $items = @()

    foreach ($pattern in $patterns) {
        $items += @(Get-ChildItem -LiteralPath $TempRoot -Force -ErrorAction SilentlyContinue -Filter $pattern |
            Where-Object { $_.LastWriteTime -lt $cutoff })
    }

    return @($items | Sort-Object FullName -Unique)
}

function Remove-OldTempCandidates {
    $candidates = Get-OldTempCandidates
    foreach ($item in $candidates) {
        try {
            Write-Step "Deleting old temp item: $($item.FullName)"
            Remove-Item -LiteralPath $item.FullName -Recurse -Force -ErrorAction Stop
        }
        catch {
            Write-Warning "Skipped locked or inaccessible temp item: $($item.FullName)"
        }
    }
}

function Show-DriveReport {
    [System.IO.DriveInfo]::GetDrives() |
        Where-Object { $_.DriveType -eq "Fixed" } |
        Select-Object Name,
            @{Name = "TotalGB"; Expression = { [math]::Round($_.TotalSize / 1GB, 2) }},
            @{Name = "FreeGB"; Expression = { [math]::Round($_.AvailableFreeSpace / 1GB, 2) }} |
        Format-Table -AutoSize
}

function Show-Plan {
    Write-Step "Drive space"
    Show-DriveReport

    Write-Step "Move plan"
    $rows = @()
    foreach ($map in $MoveMappings) {
        if ($map.Optional -and -not $IncludeOpenAILocalRuntime) {
            continue
        }

        $item = $null
        $linkType = ""
        $targetText = ""
        if (Test-Path -LiteralPath $map.Source) {
            $item = Get-Item -LiteralPath $map.Source -Force
            $linkType = $item.LinkType
            $targetText = ($item.Target -join ",")
        }

        $rows += [pscustomobject]@{
            Name = $map.Name
            Source = $map.Source
            Target = $map.Target
            LinkType = $linkType
            CurrentTarget = $targetText
            Size = Format-Size -Bytes (Get-DirectorySizeBytes -Path $map.Source)
        }
    }
    $rows | Format-Table -AutoSize

    Write-Step "Old temp cleanup candidates older than $OldTempDays days"
    $tempRows = @()
    foreach ($item in Get-OldTempCandidates) {
        $size = if ($item.PSIsContainer) { Get-DirectorySizeBytes -Path $item.FullName } else { $item.Length }
        $tempRows += [pscustomobject]@{
            Name = $item.Name
            LastWriteTime = $item.LastWriteTime
            Size = Format-Size -Bytes $size
        }
    }

    if ($tempRows.Count -eq 0) {
        Write-Host "No old temp candidates found."
    }
    else {
        $tempRows | Sort-Object LastWriteTime | Format-Table -AutoSize
    }
}

Write-Step "C drive slimming script started."

if ($PlanOnly) {
    Show-Plan
    Write-Step "PlanOnly finished. No files were moved or deleted."
    exit 0
}

Write-Step "Before"
Show-DriveReport

if ($MoveCaches) {
    foreach ($map in $MoveMappings) {
        Move-ToJunction -Map $map
    }
}

if ($CleanOldTemp) {
    Remove-OldTempCandidates
}

Write-Step "After"
Show-DriveReport

Write-Step "Done."
