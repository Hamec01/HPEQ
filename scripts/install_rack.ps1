# Invoked only by an explicit `build_vst.bat install` after the DAW is closed.
$ErrorActionPreference = 'Stop'
$rackRoot = Split-Path -Parent $PSScriptRoot
$rackBundle = Join-Path $rackRoot 'build/HPEQ_artefacts/Release/VST3/HamloRack.vst3'
if (-not (Test-Path -LiteralPath (Join-Path $rackBundle 'Contents/x86_64-win/HamloRack.vst3'))) { throw 'Build HamloRack first.' }
$rackDestination = [IO.Path]::GetFullPath((Join-Path $env:CommonProgramFiles 'VST3'))
$rackBackup = Join-Path $rackRoot ('out/rack-install-backup/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Force -Path $rackDestination,$rackBackup | Out-Null
if ((Get-Item -LiteralPath $rackDestination -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'VST3 destination must be a regular directory.' }
$rackOldBundles = @('HPEQ.vst3','HamloRack.vst3') | ForEach-Object {
    $rackTarget = [IO.Path]::GetFullPath((Join-Path $rackDestination $_))
    if ([IO.Path]::GetDirectoryName($rackTarget) -ne $rackDestination) { throw 'Invalid rack install path.' }
    $rackTarget
}
# Finish backups before replacing anything. Standalone Hamlo bundles are untouched.
foreach ($rackTarget in $rackOldBundles) {
    if ((Test-Path -LiteralPath $rackTarget) -and ((Get-Item -LiteralPath $rackTarget -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Rack bundle must not be a link: $rackTarget" }
    if (Test-Path -LiteralPath $rackTarget) { Copy-Item -LiteralPath $rackTarget -Destination $rackBackup -Recurse -ErrorAction Stop }
}
try {
    foreach ($rackTarget in $rackOldBundles) {
        if (Test-Path -LiteralPath $rackTarget) { Remove-Item -LiteralPath $rackTarget -Recurse -Force }
    }
    Copy-Item -LiteralPath $rackBundle -Destination $rackDestination -Recurse
} catch {
    $rackInstallError = $_
    foreach ($rackTarget in $rackOldBundles) {
        if (Test-Path -LiteralPath $rackTarget) { Remove-Item -LiteralPath $rackTarget -Recurse -Force }
        $rackSaved = Join-Path $rackBackup ([IO.Path]::GetFileName($rackTarget))
        if (Test-Path -LiteralPath $rackSaved) { Copy-Item -LiteralPath $rackSaved -Destination $rackDestination -Recurse }
    }
    throw $rackInstallError
}
Write-Output "Installed HamloRack. Previous rack bundles: $rackBackup"
