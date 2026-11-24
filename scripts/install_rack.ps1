# Invoked only for explicit installation, after closing the DAW.
param([string]$BundlePath = '')
$ErrorActionPreference = 'Stop'
if (!$BundlePath) { $BundlePath = Join-Path (Split-Path -Parent $PSScriptRoot) 'batvst/HamloRack.vst3' }
$BundlePath = [IO.Path]::GetFullPath($BundlePath)
if ([IO.Path]::GetFileName($BundlePath) -ne 'HamloRack.vst3') { throw 'Expected a HamloRack.vst3 bundle.' }
. (Join-Path $PSScriptRoot 'install_hamlo_plugins.ps1') -FunctionsOnly
$commonFiles = $env:CommonProgramW6432
if (!$commonFiles) { $commonFiles = [Environment]::GetFolderPath('CommonProgramFiles') }
Invoke-HamloInstaller ([IO.Path]::GetDirectoryName($BundlePath)) $commonFiles @('HamloRack') @('HPEQ')
