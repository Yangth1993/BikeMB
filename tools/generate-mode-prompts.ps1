param(
  [string]$SourceDir = "generated_audio",
  [string]$WorkDir = "build/generated-prompts"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$SourceRoot = Join-Path $RepoRoot $SourceDir
$WorkRoot = Join-Path $RepoRoot $WorkDir
New-Item -ItemType Directory -Force -Path $WorkRoot | Out-Null

if (-not (Test-Path $SourceRoot)) {
  throw "Prompt source directory not found: $SourceRoot"
}

function Build-Transcoder {
  $CsPath = Join-Path $env:TEMP "bikemb_audio_transcode.cs"
  $ExePath = Join-Path $env:TEMP "bikemb_audio_transcode.exe"
  if (Test-Path $ExePath) {
    return $ExePath
  }

@'
using System;
using System.IO;
using System.Threading.Tasks;
using Windows.Media.MediaProperties;
using Windows.Media.Transcoding;
using Windows.Storage;

public class BikeMbAudioTranscode {
  public static int Main(string[] args) {
    return MainAsync(args).GetAwaiter().GetResult();
  }

  private static async Task<int> MainAsync(string[] args) {
    if (args.Length != 2) {
      Console.Error.WriteLine("usage: transcode.exe <input-audio> <output-wav>");
      return 2;
    }

    var input = await StorageFile.GetFileFromPathAsync(args[0]).AsTask();
    var outputFolder = await StorageFolder.GetFolderFromPathAsync(Path.GetDirectoryName(args[1])).AsTask();
    var output = await outputFolder.CreateFileAsync(Path.GetFileName(args[1]), CreationCollisionOption.ReplaceExisting).AsTask();
    var profile = MediaEncodingProfile.CreateWav(AudioEncodingQuality.High);
    var transcoder = new MediaTranscoder();
    var prepared = await transcoder.PrepareFileTranscodeAsync(input, output, profile).AsTask();
    if (!prepared.CanTranscode) {
      Console.Error.WriteLine(prepared.FailureReason.ToString());
      return 3;
    }
    await prepared.TranscodeAsync().AsTask();
    return 0;
  }
}
'@ | Set-Content -Encoding UTF8 $CsPath

  $Csc = "C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
  $Winmd = "C:\Program Files (x86)\Windows Kits\10\UnionMetadata\10.0.26100.0\Windows.winmd"
  & $Csc /nologo `
    /r:C:\Windows\Microsoft.NET\Framework64\v4.0.30319\System.Runtime.dll `
    /r:C:\Windows\Microsoft.NET\Framework64\v4.0.30319\System.Runtime.WindowsRuntime.dll `
    /r:$Winmd `
    /out:$ExePath `
    $CsPath
  if ($LASTEXITCODE -ne 0) {
    throw "Failed to compile Windows Media Foundation transcode helper."
  }

  return $ExePath
}

function ConvertTo-PcmWav([string]$InputPath, [string]$OutputPath) {
  $Transcoder = Build-Transcoder
  & $Transcoder $InputPath $OutputPath
  if ($LASTEXITCODE -ne 0) {
    throw "Failed to convert source audio to PCM WAV: $InputPath"
  }
}

function Get-WavChunks([byte[]]$Bytes) {
  if ([Text.Encoding]::ASCII.GetString($Bytes, 0, 4) -ne "RIFF" -or
      [Text.Encoding]::ASCII.GetString($Bytes, 8, 4) -ne "WAVE") {
    throw "Input is not a RIFF/WAVE file."
  }

  $Chunks = @{}
  $Offset = 12
  while ($Offset + 8 -le $Bytes.Length) {
    $ChunkId = [Text.Encoding]::ASCII.GetString($Bytes, $Offset, 4)
    $ChunkSize = [BitConverter]::ToUInt32($Bytes, $Offset + 4)
    $Chunks[$ChunkId] = @{ Offset = $Offset + 8; Size = [int]$ChunkSize }
    $Offset += 8 + [int]$ChunkSize
    if (($ChunkSize % 2) -eq 1) {
      $Offset += 1
    }
  }
  return $Chunks
}

function Read-WavPcmMono16k([string]$Path) {
  $Bytes = [IO.File]::ReadAllBytes($Path)
  $Chunks = Get-WavChunks $Bytes
  if (-not $Chunks.ContainsKey("fmt ") -or -not $Chunks.ContainsKey("data")) {
    throw "$Path must contain fmt and data chunks."
  }

  $FmtOffset = $Chunks["fmt "].Offset
  $AudioFormat = [BitConverter]::ToUInt16($Bytes, $FmtOffset)
  $Channels = [BitConverter]::ToUInt16($Bytes, $FmtOffset + 2)
  $SampleRate = [BitConverter]::ToUInt32($Bytes, $FmtOffset + 4)
  $BitsPerSample = [BitConverter]::ToUInt16($Bytes, $FmtOffset + 14)
  if ($AudioFormat -ne 1 -or $BitsPerSample -ne 16 -or $Channels -lt 1) {
    throw "$Path must be 16-bit PCM WAV before resampling."
  }

  $DataOffset = $Chunks["data"].Offset
  $DataSize = $Chunks["data"].Size
  $FrameCount = [int]($DataSize / (2 * $Channels))
  $Mono = New-Object double[] $FrameCount
  for ($Frame = 0; $Frame -lt $FrameCount; ++$Frame) {
    $Sum = 0
    for ($Ch = 0; $Ch -lt $Channels; ++$Ch) {
      $SampleOffset = $DataOffset + (($Frame * $Channels + $Ch) * 2)
      $Sum += [BitConverter]::ToInt16($Bytes, $SampleOffset)
    }
    $Mono[$Frame] = $Sum / $Channels
  }

  $TargetRate = 16000
  $TargetCount = [Math]::Max(1, [int][Math]::Round($FrameCount * $TargetRate / [double]$SampleRate))
  $Output = New-Object int16[] $TargetCount
  for ($i = 0; $i -lt $TargetCount; ++$i) {
    $SourcePos = $i * [double]$SampleRate / $TargetRate
    $Index = [int][Math]::Floor($SourcePos)
    if ($Index -ge $FrameCount - 1) {
      $Value = $Mono[$FrameCount - 1]
    } else {
      $Frac = $SourcePos - $Index
      $Value = $Mono[$Index] * (1.0 - $Frac) + $Mono[$Index + 1] * $Frac
    }
    $Value = [Math]::Max([int16]::MinValue, [Math]::Min([int16]::MaxValue, [Math]::Round($Value)))
    $Output[$i] = [int16]$Value
  }

  return $Output
}

function Write-CArray([System.Text.StringBuilder]$Builder, [string]$Name, [int16[]]$Samples) {
  [void]$Builder.AppendLine("const int16_t $Name[] = {")
  for ($i = 0; $i -lt $Samples.Length; $i += 12) {
    $End = [Math]::Min($i + 11, $Samples.Length - 1)
    $Line = ($i..$End | ForEach-Object { $Samples[$_].ToString() }) -join ", "
    [void]$Builder.AppendLine("    $Line,")
  }
  [void]$Builder.AppendLine("};")
  [void]$Builder.AppendLine("const uint32_t ${Name}SampleCount = $($Samples.Length);")
  [void]$Builder.AppendLine()
}

$Prompts = @(
  @{ Name = "Eco"; Source = "eco.mp3"; Array = "kBikeMbPromptEcoPcm" },
  @{ Name = "Trail"; Source = "trail.mp3"; Array = "kBikeMbPromptTrailPcm" },
  @{ Name = "Boost"; Source = "boost.mp3"; Array = "kBikeMbPromptBoostPcm" }
)

$PromptSamples = @{}
foreach ($Prompt in $Prompts) {
  $InputPath = Join-Path $SourceRoot $Prompt.Source
  if (-not (Test-Path $InputPath)) {
    throw "Prompt source file not found: $InputPath"
  }

  $WavPath = Join-Path $WorkRoot ($Prompt.Name.ToLowerInvariant() + ".wav")
  ConvertTo-PcmWav (Resolve-Path $InputPath).Path $WavPath
  $PromptSamples[$Prompt.Array] = Read-WavPcmMono16k $WavPath
}

$HeaderPath = Join-Path $RepoRoot "firmware/bikemb/src/audio/audio_prompt_assets.h"
$SourcePath = Join-Path $RepoRoot "firmware/bikemb/src/audio/audio_prompt_assets.cpp"

$Header = @"
#pragma once

#include <stdint.h>

extern const int16_t kBikeMbPromptEcoPcm[];
extern const uint32_t kBikeMbPromptEcoPcmSampleCount;
extern const int16_t kBikeMbPromptTrailPcm[];
extern const uint32_t kBikeMbPromptTrailPcmSampleCount;
extern const int16_t kBikeMbPromptBoostPcm[];
extern const uint32_t kBikeMbPromptBoostPcmSampleCount;
"@
Set-Content -Encoding ASCII $HeaderPath $Header

$Source = [System.Text.StringBuilder]::new()
[void]$Source.AppendLine('#include "audio_prompt_assets.h"')
[void]$Source.AppendLine()
[void]$Source.AppendLine("// Generated by tools/generate-mode-prompts.ps1 from generated_audio source files.")
[void]$Source.AppendLine("// Source audio is converted to 16-bit mono PCM, 16000 Hz.")
[void]$Source.AppendLine()
[void]$Source.AppendLine("#ifndef BIKE_MB_ENABLE_AUDIO_PROMPTS")
[void]$Source.AppendLine("#define BIKE_MB_ENABLE_AUDIO_PROMPTS 0")
[void]$Source.AppendLine("#endif")
[void]$Source.AppendLine()
[void]$Source.AppendLine("#if BIKE_MB_ENABLE_AUDIO_PROMPTS")
[void]$Source.AppendLine()
Write-CArray $Source "kBikeMbPromptEcoPcm" $PromptSamples["kBikeMbPromptEcoPcm"]
Write-CArray $Source "kBikeMbPromptTrailPcm" $PromptSamples["kBikeMbPromptTrailPcm"]
Write-CArray $Source "kBikeMbPromptBoostPcm" $PromptSamples["kBikeMbPromptBoostPcm"]
[void]$Source.AppendLine("#endif")
Set-Content -Encoding ASCII $SourcePath $Source.ToString()

Write-Host "Generated prompt assets from $SourceRoot"
