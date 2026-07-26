@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "SCRIPT=%SCRIPT_DIR%tools\start-codex-session.ps1"

if not exist "%SCRIPT%" (
    echo Cannot find startup script:
    echo %SCRIPT%
    pause
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*

if errorlevel 1 (
    echo.
    echo Startup failed. Check the messages above.
    pause
    exit /b %errorlevel%
)

endlocal
