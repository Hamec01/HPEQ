[CmdletBinding()]
param([string]$Label = 'validators')
$ErrorActionPreference = 'Stop'
$repoPath = Split-Path $PSScriptRoot -Parent
$bundlePath = Join-Path $repoPath 'build/HamloRevolution_artefacts/Release/VST3/HamloRevolution.vst3'
$dllPath = Join-Path $bundlePath 'Contents/x86_64-win/HamloRevolution.vst3'
$validationPath = Join-Path $repoPath ('docs/reports/r9/' + $Label)
if (Test-Path -LiteralPath $validationPath) { throw 'Refusing to overwrite validator evidence' }
New-Item -ItemType Directory -Path $validationPath | Out-Null
$pluginvalPath = 'E:/HamloComp/build/tools/pluginval/pluginval.exe'
$steinbergPath = 'E:/HamloComp/build/tools/vst3sdk-build/bin/Release/validator.exe'
$testedHash = (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash.ToLowerInvariant()
$summary = @()
foreach ($level in 5..10) {
    $levelPath = Join-Path $validationPath ('pluginval_' + $level)
    New-Item -ItemType Directory -Path $levelPath | Out-Null
    $arguments = @('--strictness-level', "$level", '--random-seed', '0x46414631', '--timeout-ms', '120000', '--output-dir', ('"' + $levelPath + '"'))
    if ($level -eq 10) { $arguments += @('--vst3validator', ('"' + $steinbergPath + '"')) }
    $arguments += @('--validate', ('"' + $bundlePath + '"'))
    $process = Start-Process -FilePath $pluginvalPath -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $levelPath 'stdout.txt') -RedirectStandardError (Join-Path $levelPath 'stderr.txt')
    $summary += "pluginval $level exit=$($process.ExitCode)"
    $summary | Set-Content -LiteralPath (Join-Path $validationPath 'summary.txt') -Encoding utf8
    Write-Output $summary[-1]
    if ($process.ExitCode -ne 0) { throw "pluginval $level failed; retained raw output" }
}
$process = Start-Process -FilePath $steinbergPath -ArgumentList @('-e', ('"' + $bundlePath + '"')) -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $validationPath 'steinberg.txt') -RedirectStandardError (Join-Path $validationPath 'steinberg.err')
$summary += "Steinberg exit=$($process.ExitCode)"
$summary | Set-Content -LiteralPath (Join-Path $validationPath 'summary.txt') -Encoding utf8
if ($process.ExitCode -ne 0) { throw 'Steinberg failed; retained raw output' }
if ((Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $testedHash) { throw 'Binary changed during validation' }
@{ dll_sha256 = $testedHash; results = $summary } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $validationPath 'identity.json') -Encoding utf8
Write-Output $summary[-1]
