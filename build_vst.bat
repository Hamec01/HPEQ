@echo off
rem Builds the HPEQ VST3 plug-in (Release).
rem   build_vst.bat           build only
rem   build_vst.bat install   build, then copy HPEQ.vst3 to C:\Program Files\Common Files\VST3
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
echo Building HPEQ (Release)...
cmake --build --preset release --parallel --target HPEQ_VST3
if errorlevel 1 goto :fail

set "HPEQ_VST=%~dp0build\HPEQ_artefacts\Release\VST3\HPEQ.vst3"
echo.
echo Done:
echo   %HPEQ_VST%

if /i not "%~1"=="install" goto :ok

echo.
net session >nul 2>nul
if errorlevel 1 (
    echo "install" needs administrator rights: right-click build_vst.bat - Run as administrator,
    echo or run it from an administrator command prompt.
    goto :fail
)
set "TARGET=C:\Program Files\Common Files\VST3"
echo Copying to %TARGET% ...
xcopy "%HPEQ_VST%" "%TARGET%\HPEQ.vst3\" /E /I /Y /Q >nul
if errorlevel 1 goto :fail
echo Installed. Start the DAW (rescan plug-ins if HPEQ does not appear).

:ok
echo.
pause
exit /b 0

:fail
echo.
echo FAILED - see the messages above.
pause
exit /b 1
