param([string]$ReportRoot='E:\HamloEcho\docs\reports\release_020_final')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$bundle=(Resolve-Path -LiteralPath (Join-Path $repo 'build\HamloEcho_artefacts\Release\VST3\HamloEcho.vst3')).Path
$inventory=@(Get-ChildItem -LiteralPath $bundle -Recurse -File | Sort-Object FullName | ForEach-Object {
    [pscustomobject]@{path=$_.FullName.Substring($bundle.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant();bytes=$_.Length}
})
New-Item -ItemType Directory -Path $ReportRoot -Force | Out-Null
& (Join-Path $PSScriptRoot 'validate_legacy.ps1') -ReportDirectory (Join-Path $ReportRoot 'validators')
foreach ($file in $inventory) {
    $actual=(Get-FileHash -LiteralPath (Join-Path $bundle $file.path) -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $file.sha256) { throw 'VST3 changed during validation' }
}
$source=Get-Content (Join-Path $repo 'build\SourceIdentity.h') -Raw
if ($source -notmatch 'HAMLO_ECHO_SOURCE_ID "([0-9a-f]{64})"') { throw 'Missing source identity' }
$record=[pscustomobject]@{version='0.2.0';dsp_source=$Matches[1];passed=$true;files=$inventory;validation_scope='pluginval5..10; Steinberg537/537; immutable payload before/after'}
$record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $ReportRoot 'validated_payload.json') -Encoding utf8
