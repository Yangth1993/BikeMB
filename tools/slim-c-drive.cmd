@echo off
setlocal

set "SCRIPT=%~dp0slim-c-drive.ps1"

if not exist "%SCRIPT%" (
    echo Cannot find script:
    echo %SCRIPT%
    pause
    exit /b 1
)

if "%~1"=="" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" -PlanOnly
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*
)

if errorlevel 1 (
    echo.
    echo C drive slimming script failed. Check the messages above.
    pause
    exit /b %errorlevel%
)

pause
