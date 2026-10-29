# Run only after the final preset bank and VST3 have been built.
[CmdletBinding()]
param(
    [string]$PluginvalPath = 'E:\HamloComp\build\tools\pluginval\pluginval.exe',
    [string]$ValidatorPath = 'E:\HamloComp\build\tools\vst3sdk-build\bin\Release\validator.exe'
)
$ErrorActionPreference = 'Stop'
$magRoot = Split-Path $PSScriptRoot -Parent
$magBundle = Join-Path $magRoot 'build\HamloMag_artefacts\Release\VST3\HamloMag.vst3'
foreach ($level in 5..10) {
    $magLogDir = Join-Path $magRoot "docs\reports\pluginval\v2\strictness_$level"
    New-Item -ItemType Directory -Force $magLogDir | Out-Null
    $magArguments = @('--strictness-level', $level, '--random-seed', '0x51e84f', '--output-dir', $magLogDir)
    if ($level -eq 10) { $magArguments += @('--vst3validator', $ValidatorPath) }
    $magArguments += @('--validate', $magBundle)
    $magProcess = Start-Process -FilePath $PluginvalPath -ArgumentList $magArguments -WindowStyle Hidden -Wait -PassThru
    if ($magProcess.ExitCode -ne 0) { throw "pluginval $level failed: $($magProcess.ExitCode)" }
    Write-Output "pluginval $level passed"
}
$magValidatorDir = Join-Path $magRoot 'docs\reports\vst3validator'
New-Item -ItemType Directory -Force $magValidatorDir | Out-Null
& $ValidatorPath -e $magBundle 2>&1 | Out-File -Encoding utf8 (Join-Path $magValidatorDir 'validator_v2.txt')
if ($LASTEXITCODE -ne 0) { throw "Steinberg validator failed: $LASTEXITCODE" }
Write-Output 'Steinberg extensive validation passed'
