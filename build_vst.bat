@echo off
rem Builds the separate Hamlo module VST3 plug-ins (Release); HPEQ itself is not built here.
rem   build_vst.bat           build only
rem   build_vst.bat install   build, then copy every .vst3 to C:\Program Files\Common Files\VST3
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
echo Building the Hamlo modules (Release)...
cmake --build --preset release --parallel --target HamloComp_VST3 HamloVerb_VST3 HamloEcho_VST3 HamloMag_VST3 HamloRedWall_VST3 HamloUdarnik_VST3 HamloZastava_VST3 HamloShirota_VST3
if errorlevel 1 goto :fail

set "MODULES=%~dp0build\HamloModules\Release"

echo.
echo Done:
for /d %%M in ("%MODULES%\*.vst3") do echo   %%M

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
for /d %%M in ("%MODULES%\*.vst3") do (
    xcopy "%%M" "%TARGET%\%%~nxM\" /E /I /Y /Q >nul
    if errorlevel 1 goto :fail
)
echo Installed. Start the DAW and rescan plug-ins if new modules do not appear.

:ok
echo.
pause
exit /b 0

:fail
echo.
echo FAILED - see the messages above.
pause
exit /b 1
