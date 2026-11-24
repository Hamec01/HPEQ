@echo off
rem Build all, a selected plug-in, or check commands without compiling.
setlocal EnableExtensions DisableDelayedExpansion
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build_hamlo.ps1" %*
set "HAMLO_RESULT=%errorlevel%"
echo.
if not defined HAMLO_NO_PAUSE pause
exit /b %HAMLO_RESULT%
