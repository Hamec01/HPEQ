@echo off
rem Install the ten standalone bundles from batvst; Windows elevation is requested when needed.
setlocal EnableExtensions DisableDelayedExpansion
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\install_hamlo_plugins.ps1" %*
set "HAMLO_RESULT=%errorlevel%"
echo.
if not defined HAMLO_NO_PAUSE pause
exit /b %HAMLO_RESULT%
