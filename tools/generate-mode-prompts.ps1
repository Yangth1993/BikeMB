param(
  [string]$VoiceName = "Microsoft Kangkang",
  [string]$OutDir = "build/generated-prompts"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutputDir = Join-Path $RepoRoot $OutDir
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$CsPath = Join-Path $env:TEMP "bikemb_tts_winrt.cs"
$ExePath = Join-Path $env:TEMP "bikemb_tts_winrt.exe"
@'
using System;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Media.SpeechSynthesis;

public class BikeMbTtsWinRt {
  public static int Main(string[] args) {
    return MainAsync(args).GetAwaiter().GetResult();
  }

  private static async Task<int> MainAsync(string[] args) {
    if (args.Length != 3) {
      Console.Error.WriteLine("usage: tts.exe <voice-display-name> <text> <output.wav>");
      return 2;
    }

    var synth = new SpeechSynthesizer();
    var voice = SpeechSynthesizer.AllVoices.FirstOrDefault(v => v.DisplayName == args[0]);
    if (voice == null) {
      Console.Error.WriteLine("voice not found: " + args[0]);
      return 3;
    }

    synth.Voice = voice;
    using (var stream = await synth.SynthesizeTextToStreamAsync(args[1]).AsTask()) {
      using (var input = stream.AsStreamForRead()) {
        using (var output = File.Create(args[2])) {
          await input.CopyToAsync(output);
        }
      }
    }
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
  throw "Failed to compile local TTS helper."
}

$Prompts = @(
  @{ Name = "Eco"; Phrase = "经济模式"; File = "eco.wav" },
  @{ Name = "Trail"; Phrase = "越野模式"; File = "trail.wav" },
  @{ Name = "Boost"; Phrase = "增强模式"; File = "boost.wav" }
)

foreach ($Prompt in $Prompts) {
  $WavPath = Join-Path $OutputDir $Prompt.File
  & $ExePath $VoiceName $Prompt.Phrase $WavPath
  if ($LASTEXITCODE -ne 0) {
    throw "Failed to generate $($Prompt.File) with $VoiceName."
  }
}

function Read-WavPcm16Mono([string]$Path) {
  $Bytes = [IO.File]::ReadAllBytes($Path)
  if ([Text.Encoding]::ASCII.GetString($Bytes, 0, 4) -ne "RIFF" -or
      [Text.Encoding]::ASCII.GetString($Bytes, 8, 4) -ne "WAVE") {
    throw "$Path is not a RIFF/WAVE file."
  }

  $Offset = 12
  $FormatOk = $false
  $DataOffset = -1
  $DataSize = 0
  while ($Offset + 8 -le $Bytes.Length) {
    $ChunkId = [Text.Encoding]::ASCII.GetString($Bytes, $Offset, 4)
    $ChunkSize = [BitConverter]::ToUInt32($Bytes, $Offset + 4)
    $ChunkData = $Offset + 8
    if ($ChunkId -eq "fmt ") {
      $AudioFormat = [BitConverter]::ToUInt16($Bytes, $ChunkData)
      $Channels = [BitConverter]::ToUInt16($Bytes, $ChunkData + 2)
      $SampleRate = [BitConverter]::ToUInt32($Bytes, $ChunkData + 4)
      $BitsPerSample = [BitConverter]::ToUInt16($Bytes, $ChunkData + 14)
      if ($AudioFormat -eq 1 -and $Channels -eq 1 -and $SampleRate -eq 16000 -and $BitsPerSample -eq 16) {
        $FormatOk = $true
      }
    } elseif ($ChunkId -eq "data") {
      $DataOffset = $ChunkData
      $DataSize = [int]$ChunkSize
    }
    $Offset = $ChunkData + [int]$ChunkSize
    if (($ChunkSize % 2) -eq 1) {
      $Offset += 1
    }
  }

  if (-not $FormatOk) {
    throw "$Path must be 16-bit mono PCM at 16000 Hz."
  }
  if ($DataOffset -lt 0 -or $DataSize -le 0) {
    throw "$Path has no data chunk."
  }

  $Samples = New-Object int16[] ($DataSize / 2)
  [Buffer]::BlockCopy($Bytes, $DataOffset, $Samples, 0, $DataSize)
  return $Samples
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
[void]$Source.AppendLine("// Generated by tools/generate-mode-prompts.ps1 from local Windows TTS voice Microsoft Kangkang.")
[void]$Source.AppendLine("// Source WAV format: 16-bit mono PCM, 16000 Hz.")
[void]$Source.AppendLine()
[void]$Source.AppendLine("#ifndef BIKE_MB_ENABLE_AUDIO_PROMPTS")
[void]$Source.AppendLine("#define BIKE_MB_ENABLE_AUDIO_PROMPTS 0")
[void]$Source.AppendLine("#endif")
[void]$Source.AppendLine()
[void]$Source.AppendLine("#if BIKE_MB_ENABLE_AUDIO_PROMPTS")
[void]$Source.AppendLine()
Write-CArray $Source "kBikeMbPromptEcoPcm" (Read-WavPcm16Mono (Join-Path $OutputDir "eco.wav"))
Write-CArray $Source "kBikeMbPromptTrailPcm" (Read-WavPcm16Mono (Join-Path $OutputDir "trail.wav"))
Write-CArray $Source "kBikeMbPromptBoostPcm" (Read-WavPcm16Mono (Join-Path $OutputDir "boost.wav"))
[void]$Source.AppendLine("#endif")
Set-Content -Encoding ASCII $SourcePath $Source.ToString()

Write-Host "Generated prompts with $VoiceName into $OutputDir"
