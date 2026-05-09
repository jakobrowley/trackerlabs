@echo off
setlocal EnableDelayedExpansion

REM Auto-elevate to administrator if not already
NET SESSION >nul 2>&1
if %errorLevel% NEQ 0 (
    echo Requesting administrator privileges...
    powershell -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

echo.
echo ============================================================
echo   Installing TrackerLabs for After Effects
echo ============================================================
echo.

set "DEST=%ProgramFiles%\Adobe\Common\Plug-ins\7.0\MediaCore"
set "SRC=%~dp0plugin_files"

if not exist "%SRC%\TrackerLabs.aex" (
    echo [ERROR] plugin_files\TrackerLabs.aex not found.
    echo Make sure you UNZIPPED the entire folder, not just opened the .bat.
    echo.
    pause
    exit /b 1
)

if not exist "%DEST%" (
    echo Creating Adobe MediaCore plug-ins folder...
    mkdir "%DEST%" 2>nul
    if not exist "%DEST%" (
        echo [ERROR] Could not create:
        echo   %DEST%
        echo Make sure After Effects is installed, and that you ran this as
        echo administrator (right-click ^> Run as administrator).
        echo.
        pause
        exit /b 1
    )
)

echo Copying TrackerLabs.aex to After Effects plug-ins folder...
copy /Y "%SRC%\TrackerLabs.aex" "%DEST%\TrackerLabs.aex" >nul
if not exist "%DEST%\TrackerLabs.aex" (
    echo [ERROR] File copy failed.
    echo.
    pause
    exit /b 1
)

REM Strip Mark-of-the-Web so SmartScreen doesn't block the loaded plugin
echo Removing internet zone markers...
powershell -Command "Unblock-File -Path '%DEST%\TrackerLabs.aex' -ErrorAction SilentlyContinue" >nul 2>&1

echo.
echo ============================================================
echo   Install complete!
echo ============================================================
echo.
echo Next steps:
echo   1. Close After Effects if it is currently open
echo   2. Open After Effects
echo   3. Apply TrackerLabs (Effect ^> TrackerLabs) to a layer
echo   4. Enter your license key when prompted
echo.
pause
