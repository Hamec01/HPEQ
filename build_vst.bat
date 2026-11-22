@echo off
rem Builds the HamloRack VST3 plug-in (Release; HPEQ-compatible identity).
rem   build_vst.bat           build only
rem   build_vst.bat install   build, back up old rack bundles, then install HamloRack
rem                           (run as administrator; close the DAW first)
setlocal
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake was not found. Install CMake or open a Developer Command Prompt.
    goto :fail
)

if not exist "build\CMakeCache.txt" (
    echo Configuring the build folder...
    cmake --preset vs2026-x64
    if errorlevel 1 goto :fail
)

echo.
echo Building HamloRack (Release)...
cmake --build --preset release --parallel --target HPEQ_VST3
if errorlevel 1 goto :fail

set "HPEQ_VST=%~dp0build\HPEQ_artefacts\Release\VST3\HamloRack.vst3"
if not exist "%HPEQ_VST%\Contents\x86_64-win\HamloRack.vst3" goto :fail
echo.
echo Done:
echo   %HPEQ_VST%

if not exist "%~dp0batvst" mkdir "%~dp0batvst"
xcopy "%HPEQ_VST%" "%~dp0batvst\HamloRack.vst3\" /E /I /Y /Q >nul
if errorlevel 1 goto :fail

if /i not "%~1"=="install" goto :ok

echo.
net session >nul 2>nul
if errorlevel 1 (
    echo "install" needs administrator rights: right-click build_vst.bat - Run as administrator,
    echo or run it from an administrator command prompt.
    goto :fail
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\install_rack.ps1"
if errorlevel 1 goto :fail
echo Installed. Start the DAW and rescan for HamloRack.

:ok
echo.
pause
exit /b 0

:fail
echo.
echo FAILED - see the messages above.
pause
exit /b 1
