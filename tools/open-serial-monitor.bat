@echo off
setlocal

set PORT=%~1
if "%PORT%"=="" set PORT=COM5

set BAUD=%~2
if "%BAUD%"=="" set BAUD=115200

powershell.exe -NoExit -ExecutionPolicy Bypass -File "%~dp0serial-monitor.ps1" -Port "%PORT%" -Baud %BAUD%

