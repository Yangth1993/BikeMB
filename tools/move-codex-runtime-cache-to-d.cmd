@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0move-codex-runtime-cache-to-d.ps1" %*
if errorlevel 1 (
    echo.
    echo Migration failed. Check the messages above.
    pause
    exit /b %errorlevel%
)
pause
