@echo off
rem Build HamloRack only. Optional: install or check.
setlocal EnableExtensions DisableDelayedExpansion
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build_hamlo.ps1" -RackOnly %*
set "HAMLO_RESULT=%errorlevel%"
echo.
if not defined HAMLO_NO_PAUSE pause
exit /b %HAMLO_RESULT%
