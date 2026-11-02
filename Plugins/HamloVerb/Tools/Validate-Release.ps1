[CmdletBinding()]
param(
    [string]$PluginvalPath = 'E:\HamloComp\build\tools\pluginval\pluginval.exe',
    [string]$ValidatorPath = 'E:\HamloComp\build\tools\vst3sdk-build\bin\Release\validator.exe'
)
$ErrorActionPreference = 'Stop'
$verbRoot = Split-Path $PSScriptRoot -Parent
$verbBundle = Join-Path $verbRoot 'build\HamloVerb_artefacts\Release\VST3\HamloVerb.vst3'
$verbBinary = Join-Path $verbBundle 'Contents\x86_64-win\HamloVerb.vst3'
$beforeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $verbBinary).Hash
foreach ($level in 5..10) {
    $verbLogDir = Join-Path $verbRoot "docs\reports\pluginval\v2\strictness_$level"
    New-Item -ItemType Directory -Force $verbLogDir | Out-Null
    $verbArguments = @('--strictness-level', $level, '--random-seed', '0x51e84f', '--output-dir', $verbLogDir)
    if ($level -eq 10) { $verbArguments += @('--vst3validator', $ValidatorPath) }
    $verbArguments += @('--validate', $verbBundle)
    $verbProcess = Start-Process -FilePath $PluginvalPath -ArgumentList $verbArguments -WindowStyle Hidden -Wait -PassThru
    if ($verbProcess.ExitCode -ne 0) { throw "pluginval $level failed: $($verbProcess.ExitCode)" }
    Write-Output "pluginval $level passed"
}
$verbValidatorDir = Join-Path $verbRoot 'docs\reports\vst3validator'
New-Item -ItemType Directory -Force $verbValidatorDir | Out-Null
& $ValidatorPath -e $verbBundle 2>&1 | Out-File -Encoding utf8 (Join-Path $verbValidatorDir 'validator_v2.txt')
if ($LASTEXITCODE -ne 0) { throw "Steinberg validator failed: $LASTEXITCODE" }
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $verbBinary).Hash -ne $beforeHash) { throw 'Binary changed during validation' }
Set-Content -Encoding ascii -LiteralPath (Join-Path $verbRoot 'docs\reports\v2\validated_binary.sha256') -Value ($beforeHash.ToLower() + '  HamloVerb.vst3')
Write-Output 'Steinberg extensive validation passed'
