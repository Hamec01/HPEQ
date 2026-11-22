@echo off
rem Builds current standalone Hamlo VST3 plug-ins and HamloRack (HPEQ target).
rem
rem Usage:
rem   build_hamlo_plugins.bat                    Interactive menu (choose one or all)
rem   build_hamlo_plugins.bat all                Build all 11 plug-ins
rem   build_hamlo_plugins.bat <name>             Build a single plug-in (e.g. HamloVerb, echo, rev, hpeq)
rem   build_hamlo_plugins.bat install [name|all] Build and copy to C:\Program Files\Common Files\VST3
rem                                              (requires administrator rights)

setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set "ROOT=%~dp0"
set "JUCE_DIR=%ROOT%build\_deps\juce-src"
set "OUTPUT_DIR=%ROOT%batvst"
set "VST3_TARGET=C:\Program Files\Common Files\VST3"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake was not found. Install CMake or run from Developer Command Prompt.
    goto :fail
)

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if errorlevel 1 goto :fail

set "ARG1=%~1"
set "ARG2=%~2"
set "DO_INSTALL=0"
set "TARGET_CHOICE="

if /i "%ARG1%"=="install" (
    set "DO_INSTALL=1"
    set "TARGET_CHOICE=%ARG2%"
    if "%TARGET_CHOICE%"=="" set "TARGET_CHOICE=all"
    goto :check_admin
)

if not "%ARG1%"=="" (
    if /i "%ARG2%"=="install" set "DO_INSTALL=1"
    set "TARGET_CHOICE=%ARG1%"
    goto :process_choice
)

:interactive_menu
cls
echo ============================================================
echo           Hamlo & HamloRack VST3 Build System (Release)
echo ============================================================
echo.
echo   [0]  Build ALL (HamloRack + 10 Hamlo plug-ins)
echo.
echo   [1]  HamloRack            [7]  HamloEcho
echo   [2]  HamloComp            [8]  HamloZastava
echo   [3]  HamloUdarnik         [9]  HamloShirota
echo   [4]  HamloRedWall         [10] HamloFader
echo   [5]  HamloMag             [11] HamloRevolution
echo   [6]  HamloVerb
echo.
echo   [I]  Build and Install ALL to VST3 folder (Admin required)
echo   [Q]  Quit
echo.
echo ============================================================
set /p "USER_INPUT=Select option [0-11, I, Q] (default=0): "

if "%USER_INPUT%"=="" set "USER_INPUT=0"
if /i "%USER_INPUT%"=="q" exit /b 0

if /i "%USER_INPUT%"=="i" (
    set "DO_INSTALL=1"
    set "TARGET_CHOICE=all"
    goto :check_admin
)

set "TARGET_CHOICE=%USER_INPUT%"

:process_choice
rem Normalize numeric menu choices
if "%TARGET_CHOICE%"=="0"  set "TARGET_CHOICE=all"
if "%TARGET_CHOICE%"=="1"  set "TARGET_CHOICE=HPEQ"
if "%TARGET_CHOICE%"=="2"  set "TARGET_CHOICE=HamloComp"
if "%TARGET_CHOICE%"=="3"  set "TARGET_CHOICE=HamloUdarnik"
if "%TARGET_CHOICE%"=="4"  set "TARGET_CHOICE=HamloRedWall"
if "%TARGET_CHOICE%"=="5"  set "TARGET_CHOICE=HamloMag"
if "%TARGET_CHOICE%"=="6"  set "TARGET_CHOICE=HamloVerb"
if "%TARGET_CHOICE%"=="7"  set "TARGET_CHOICE=HamloEcho"
if "%TARGET_CHOICE%"=="8"  set "TARGET_CHOICE=HamloZastava"
if "%TARGET_CHOICE%"=="9"  set "TARGET_CHOICE=HamloShirota"
if "%TARGET_CHOICE%"=="10" set "TARGET_CHOICE=HamloFader"
if "%TARGET_CHOICE%"=="11" set "TARGET_CHOICE=HamloRevolution"

rem Normalize aliases
if /i "%TARGET_CHOICE%"=="rack"       set "TARGET_CHOICE=HPEQ"
if /i "%TARGET_CHOICE%"=="hamlorack"  set "TARGET_CHOICE=HPEQ"
if /i "%TARGET_CHOICE%"=="comp"       set "TARGET_CHOICE=HamloComp"
if /i "%TARGET_CHOICE%"=="udarnik"    set "TARGET_CHOICE=HamloUdarnik"
if /i "%TARGET_CHOICE%"=="redwall"    set "TARGET_CHOICE=HamloRedWall"
if /i "%TARGET_CHOICE%"=="mag"        set "TARGET_CHOICE=HamloMag"
if /i "%TARGET_CHOICE%"=="verb"       set "TARGET_CHOICE=HamloVerb"
if /i "%TARGET_CHOICE%"=="echo"       set "TARGET_CHOICE=HamloEcho"
if /i "%TARGET_CHOICE%"=="zastava"    set "TARGET_CHOICE=HamloZastava"
if /i "%TARGET_CHOICE%"=="shirota"    set "TARGET_CHOICE=HamloShirota"
if /i "%TARGET_CHOICE%"=="fader"      set "TARGET_CHOICE=HamloFader"
if /i "%TARGET_CHOICE%"=="rev"        set "TARGET_CHOICE=HamloRevolution"
if /i "%TARGET_CHOICE%"=="revolution" set "TARGET_CHOICE=HamloRevolution"

if "%DO_INSTALL%"=="1" goto :check_admin
goto :dispatch_build

:check_admin
net session >nul 2>nul
if errorlevel 1 (
    echo.
    echo [ERROR] Installation to %VST3_TARGET% requires Administrator privileges.
    echo Right-click this script and select "Run as administrator".
    goto :fail
)

:dispatch_build
rem Ensure JUCE is ready if building a plugin other than HPEQ
if /i not "%TARGET_CHOICE%"=="HPEQ" (
    if not exist "%JUCE_DIR%\CMakeLists.txt" (
        echo JUCE was not found at: %JUCE_DIR%
        echo Building HPEQ first to download JUCE...
        call :build_hpeq
        if errorlevel 1 goto :fail
    )
)

if /i "%TARGET_CHOICE%"=="all" goto :build_all
if /i "%TARGET_CHOICE%"=="HPEQ" goto :single_hpeq
if /i "%TARGET_CHOICE%"=="HamloComp" goto :single_comp
if /i "%TARGET_CHOICE%"=="HamloUdarnik" goto :single_udarnik
if /i "%TARGET_CHOICE%"=="HamloRedWall" goto :single_redwall
if /i "%TARGET_CHOICE%"=="HamloMag" goto :single_mag
if /i "%TARGET_CHOICE%"=="HamloVerb" goto :single_verb
if /i "%TARGET_CHOICE%"=="HamloEcho" goto :single_echo
if /i "%TARGET_CHOICE%"=="HamloZastava" goto :single_zastava
if /i "%TARGET_CHOICE%"=="HamloShirota" goto :single_shirota
if /i "%TARGET_CHOICE%"=="HamloFader" goto :single_fader
if /i "%TARGET_CHOICE%"=="HamloRevolution" goto :single_revolution

echo [ERROR] Unknown plugin choice: %TARGET_CHOICE%
goto :fail

:single_hpeq
call :build_hpeq
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HPEQ
if errorlevel 1 goto :fail
goto :done_summary

:single_comp
call :build_plugin HamloComp HAMLOCOMP_JUCE_DIR
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HamloComp
if errorlevel 1 goto :fail
goto :done_summary

:single_udarnik
call :build_plugin HamloUdarnik HAMLOUDARNIK_JUCE_DIR
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HamloUdarnik
if errorlevel 1 goto :fail
goto :done_summary

:single_redwall
call :build_plugin HamloRedWall HAMLOREDWALL_JUCE_DIR
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HamloRedWall
if errorlevel 1 goto :fail
goto :done_summary

:single_mag
call :build_plugin HamloMag HAMLOMAG_JUCE_DIR
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HamloMag
if errorlevel 1 goto :fail
goto :done_summary

:single_verb
call :build_plugin HamloVerb HAMLO_VERB_JUCE_DIR
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HamloVerb
if errorlevel 1 goto :fail
goto :done_summary

:single_echo
call :build_plugin HamloEcho HAMLO_ECHO_JUCE_DIR
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HamloEcho
if errorlevel 1 goto :fail
goto :done_summary

:single_zastava
call :build_plugin HamloZastava HAMLO_ZASTAVA_JUCE_DIR
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HamloZastava
if errorlevel 1 goto :fail
goto :done_summary

:single_shirota
call :build_plugin HamloShirota HAMLO_SHIROTA_JUCE_DIR
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HamloShirota
if errorlevel 1 goto :fail
goto :done_summary

:single_fader
call :build_plugin HamloFader HAMLO_FADER_JUCE_DIR
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HamloFader
if errorlevel 1 goto :fail
goto :done_summary

:single_revolution
call :build_plugin HamloRevolution HAMLO_REVOLUTION_JUCE_DIR
if errorlevel 1 goto :fail
if "%DO_INSTALL%"=="1" call :install_plugin HamloRevolution
if errorlevel 1 goto :fail
goto :done_summary

:build_all
call :build_hpeq
if errorlevel 1 goto :fail
call :build_plugin HamloComp HAMLOCOMP_JUCE_DIR
if errorlevel 1 goto :fail
call :build_plugin HamloUdarnik HAMLOUDARNIK_JUCE_DIR
if errorlevel 1 goto :fail
call :build_plugin HamloRedWall HAMLOREDWALL_JUCE_DIR
if errorlevel 1 goto :fail
call :build_plugin HamloMag HAMLOMAG_JUCE_DIR
if errorlevel 1 goto :fail
call :build_plugin HamloVerb HAMLO_VERB_JUCE_DIR
if errorlevel 1 goto :fail
call :build_plugin HamloEcho HAMLO_ECHO_JUCE_DIR
if errorlevel 1 goto :fail
call :build_plugin HamloZastava HAMLO_ZASTAVA_JUCE_DIR
if errorlevel 1 goto :fail
call :build_plugin HamloShirota HAMLO_SHIROTA_JUCE_DIR
if errorlevel 1 goto :fail
call :build_plugin HamloFader HAMLO_FADER_JUCE_DIR
if errorlevel 1 goto :fail
call :build_plugin HamloRevolution HAMLO_REVOLUTION_JUCE_DIR
if errorlevel 1 goto :fail

if "%DO_INSTALL%"=="1" (
    echo.
    echo Installing all plug-ins to %VST3_TARGET% ...
    call :install_plugin HPEQ
    if errorlevel 1 goto :fail
    call :install_plugin HamloComp
    if errorlevel 1 goto :fail
    call :install_plugin HamloUdarnik
    if errorlevel 1 goto :fail
    call :install_plugin HamloRedWall
    if errorlevel 1 goto :fail
    call :install_plugin HamloMag
    if errorlevel 1 goto :fail
    call :install_plugin HamloVerb
    if errorlevel 1 goto :fail
    call :install_plugin HamloEcho
    if errorlevel 1 goto :fail
    call :install_plugin HamloZastava
    if errorlevel 1 goto :fail
    call :install_plugin HamloShirota
    if errorlevel 1 goto :fail
    call :install_plugin HamloFader
    if errorlevel 1 goto :fail
    call :install_plugin HamloRevolution
    if errorlevel 1 goto :fail
)

:done_summary
echo.
echo ============================================================
echo BUILD SUCCEEDED!
echo VST3 folder: %OUTPUT_DIR%
if "%DO_INSTALL%"=="1" echo Installed to:  %VST3_TARGET%
echo ============================================================
echo.
pause
exit /b 0

:fail
echo.
echo ============================================================
echo [FAILED] See errors above.
echo ============================================================
echo.
pause
exit /b 1

:build_hpeq
set "HPEQ_BUILD=%ROOT%build"
set "HPEQ_BINARY=%HPEQ_BUILD%\HPEQ_artefacts\Release\VST3\HamloRack.vst3\Contents\x86_64-win\HamloRack.vst3"

echo.
echo ============================================================
echo Building HamloRack (VST3 only) ...
echo ============================================================

if not exist "%HPEQ_BUILD%\CMakeCache.txt" (
    cmake -S "%ROOT%" -B "%HPEQ_BUILD%" -G "Visual Studio 18 2026" -A x64
    if errorlevel 1 exit /b 1
)

cmake --build "%HPEQ_BUILD%" --config Release --parallel --target "HPEQ_VST3"
if errorlevel 1 exit /b 1

if not exist "%HPEQ_BINARY%" (
    echo Expected VST3 binary was not created:
    echo   %HPEQ_BINARY%
    exit /b 1
)

echo Copying HamloRack.vst3 to %OUTPUT_DIR% ...
xcopy "%HPEQ_BUILD%\HPEQ_artefacts\Release\VST3\HamloRack.vst3" "%OUTPUT_DIR%\HamloRack.vst3\" /E /I /Y /Q >nul
if errorlevel 1 exit /b 1

echo Ready: %OUTPUT_DIR%\HamloRack.vst3
exit /b 0

:build_plugin
set "PLUGIN=%~1"
set "JUCE_VARIABLE=%~2"
set "PLUGIN_SOURCE=%ROOT%Plugins\%PLUGIN%"
set "PLUGIN_BUILD=%PLUGIN_SOURCE%\build"
set "PLUGIN_BINARY=%PLUGIN_BUILD%\%PLUGIN%_artefacts\Release\VST3\%PLUGIN%.vst3\Contents\x86_64-win\%PLUGIN%.vst3"

echo.
echo ============================================================
echo Building %PLUGIN% (VST3 only) ...
echo ============================================================

if not exist "%PLUGIN_BUILD%\CMakeCache.txt" (
    cmake -S "%PLUGIN_SOURCE%" -B "%PLUGIN_BUILD%" -G "Visual Studio 18 2026" -A x64 "-D%JUCE_VARIABLE%=%JUCE_DIR%"
    if errorlevel 1 exit /b 1
)

cmake --build "%PLUGIN_BUILD%" --config Release --parallel --target "%PLUGIN%_VST3"
if errorlevel 1 exit /b 1

if not exist "%PLUGIN_BINARY%" (
    echo Expected VST3 binary was not created:
    echo   %PLUGIN_BINARY%
    exit /b 1
)

echo Copying %PLUGIN%.vst3 to %OUTPUT_DIR% ...
xcopy "%PLUGIN_BUILD%\%PLUGIN%_artefacts\Release\VST3\%PLUGIN%.vst3" "%OUTPUT_DIR%\%PLUGIN%.vst3\" /E /I /Y /Q >nul
if errorlevel 1 exit /b 1

echo Ready: %OUTPUT_DIR%\%PLUGIN%.vst3
exit /b 0

:install_plugin
set "PLUGIN=%~1"
if /i "%PLUGIN%"=="HPEQ" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%scripts\install_rack.ps1"
    exit /b !errorlevel!
)
set "PLUGIN_VST=%OUTPUT_DIR%\%PLUGIN%.vst3"
echo Installing %PLUGIN%.vst3 ...
xcopy "%PLUGIN_VST%" "%VST3_TARGET%\%PLUGIN%.vst3\" /E /I /Y /Q >nul
if errorlevel 1 exit /b 1
exit /b 0
