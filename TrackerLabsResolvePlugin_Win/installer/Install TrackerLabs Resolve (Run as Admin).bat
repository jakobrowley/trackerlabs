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
echo   Installing TrackerLabs for DaVinci Resolve
echo ============================================================
echo.

set "DEST=%ProgramFiles%\Common Files\OFX\Plugins\TrackerLabsPlugin.ofx.bundle"
set "SRC=%~dp0plugin_files"

if not exist "%SRC%" (
    echo [ERROR] plugin_files folder not found at:
    echo   %SRC%
    echo.
    echo Make sure you UNZIPPED the entire folder, not just opened the .bat.
    echo.
    pause
    exit /b 1
)

if not exist "%SRC%\Contents\Win64\TrackerLabsPlugin.ofx" (
    echo [ERROR] TrackerLabsPlugin.ofx is missing from plugin_files.
    echo Re-download the installer.
    echo.
    pause
    exit /b 1
)

echo Removing any previous TrackerLabs install...
if exist "%DEST%" (
    rmdir /S /Q "%DEST%"
    if exist "%DEST%" (
        echo [ERROR] Could not remove previous install at:
        echo   %DEST%
        echo Close DaVinci Resolve completely and try again.
        echo.
        pause
        exit /b 1
    )
)

echo Creating plugin folder...
mkdir "%DEST%" 2>nul
if not exist "%DEST%" (
    echo [ERROR] Could not create:
    echo   %DEST%
    echo Right-click this .bat and choose "Run as administrator".
    echo.
    pause
    exit /b 1
)

echo Copying plugin files...
xcopy /E /Y /Q /I "%SRC%\*" "%DEST%\" >nul
if not exist "%DEST%\Contents\Win64\TrackerLabsPlugin.ofx" (
    echo [ERROR] File copy failed - plugin binary missing after install.
    echo.
    pause
    exit /b 1
)

REM Strip Mark-of-the-Web so SmartScreen doesn't block the loaded DLL
echo Removing internet zone markers...
powershell -Command "Get-ChildItem -Path '%DEST%' -Recurse -Force | Unblock-File -ErrorAction SilentlyContinue" >nul 2>&1

REM Clear DaVinci Resolve's OFX plugin cache. Without this, a previous failed
REM load of an older TrackerLabs version stays cached as "skip" and Resolve
REM never re-scans the new install.
echo Clearing DaVinci Resolve OFX plugin cache...
set "CACHE1=%APPDATA%\Blackmagic Design\DaVinci Resolve\OFXPluginCacheV2.xml"
set "CACHE2=%APPDATA%\Blackmagic Design\DaVinci Resolve\Support\OFXPluginCacheV2.xml"
set "CACHE3=%LOCALAPPDATA%\Blackmagic Design\DaVinci Resolve\OFXPluginCacheV2.xml"
if exist "%CACHE1%" del /Q "%CACHE1%" >nul 2>&1
if exist "%CACHE2%" del /Q "%CACHE2%" >nul 2>&1
if exist "%CACHE3%" del /Q "%CACHE3%" >nul 2>&1

echo.
echo ============================================================
echo   Install complete!
echo ============================================================
echo.
echo Next steps:
echo   1. Fully close DaVinci Resolve if it is currently open
echo   2. Open DaVinci Resolve
echo   3. Open any project, go to the Color page
echo   4. In the OpenFX panel: Filters ^> TinyTapes ^> TrackerLabs
echo   5. Drag onto a clip, then enter your license key
echo.
pause
