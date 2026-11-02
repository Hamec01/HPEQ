param(
    [string]$Pluginval = 'E:\HamloComp\build\tools\pluginval\pluginval.exe',
    [string]$Validator = 'E:\HamloComp\build\tools\vst3sdk-build\bin\Release\validator.exe',
    [string]$ReportDirectory = ''
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$plugin = Join-Path $repo 'build\HamloEcho_artefacts\Release\VST3\HamloEcho.vst3'
$reports = Join-Path $repo 'docs\reports\v2\validators'
if ($ReportDirectory) { $reports = [System.IO.Path]::GetFullPath($ReportDirectory) }
New-Item -ItemType Directory -Path $reports -Force | Out-Null
$summary = @()
foreach ($level in 5..10) {
    $levelDir = Join-Path $reports "pluginval_$level"
    New-Item -ItemType Directory -Path $levelDir -Force | Out-Null
    $arguments = @('--validate', $plugin, '--strictness-level', "$level", '--random-seed', '0x484d4563', '--timeout-ms', '120000', '--output-dir', $levelDir)
    if ($level -eq 10) { $arguments += @('--vst3validator', $Validator) }
    $job = Start-Process -FilePath $Pluginval -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $levelDir 'stdout.txt') -RedirectStandardError (Join-Path $levelDir 'stderr.txt')
    $summary += "pluginval $level exit=$($job.ExitCode)"
    $summary | Set-Content (Join-Path $reports 'summary.txt')
    if ($job.ExitCode -ne 0) { throw "pluginval $level failed: $($job.ExitCode)" }
}
$job = Start-Process -FilePath $Validator -ArgumentList @('-e', $plugin) -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $reports 'steinberg.txt') -RedirectStandardError (Join-Path $reports 'steinberg.err')
$summary += "Steinberg exit=$($job.ExitCode)"
$summary | Set-Content (Join-Path $reports 'summary.txt')
if ($job.ExitCode -ne 0) { throw "Steinberg failed: $($job.ExitCode)" }
$summary
