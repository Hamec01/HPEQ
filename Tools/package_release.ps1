param([string]$ReportRoot='E:\HamloEcho\docs\reports\release_020_final')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
Set-Location $repo
$library=Get-Content docs/reports/presets/verification.json -Raw | ConvertFrom-Json
$validated=Get-Content (Join-Path $ReportRoot 'validated_payload.json') -Raw | ConvertFrom-Json
if ($library.passed -ne 200 -or $library.failed -ne 0 -or $library.unique -ne 200 -or !$validated.passed -or $library.dsp_source -ne $validated.dsp_source) { throw 'Release acceptance is incomplete or stale' }
$tests=Get-Content build/shipping_tests.log -Raw
if ($tests -notmatch 'HamloEcho_Tests: (\d+) expectations passed, 0 failed' -or [int]$Matches[1] -lt 1198) { throw 'Final unit checks missing' }
$legacy=Get-Content (Join-Path $ReportRoot 'legacy_160.json') -Raw | ConvertFrom-Json
if ($legacy.failures -ne 0 -or $legacy.cases.Count -ne 160) { throw 'Final M0 checks missing' }
$currentSource=(& C:/Users/Ham_h/AppData/Local/Programs/Python/Python311/python.exe -I -c "import runpy; print(runpy.run_path('Tools/calibrate_score.py')['current_dsp']())").Trim()
if ($LASTEXITCODE -ne 0 -or $currentSource -ne $validated.dsp_source) { throw 'Source changed after validation' }
$bundle=(Resolve-Path -LiteralPath 'build\HamloEcho_artefacts\Release\VST3\HamloEcho.vst3').Path
foreach ($file in $validated.files) {
    if ((Get-FileHash -LiteralPath (Join-Path $bundle $file.path) -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.sha256) { throw 'Validated payload mismatch' }
}
$dll=$validated.files | Where-Object { $_.path -eq 'Contents/x86_64-win/HamloEcho.vst3' }
if (!$dll) { throw 'Missing validated plugin binary' }
Copy-Item -LiteralPath docs/USER_GUIDE.md -Destination docs/release/USER-GUIDE.txt -Force
& powershell -NoProfile -ExecutionPolicy Bypass -File Installer/Build-Installer.ps1 -SkipBuild -CompilerPath C:/DRUMENGINE/build/tools/InnoSetup/ISCC.exe
if ($LASTEXITCODE -ne 0) { throw 'Installer build failed' }
foreach ($file in $validated.files) {
    if ((Get-FileHash -LiteralPath (Join-Path $bundle $file.path) -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.sha256) { throw 'Compiler input payload changed' }
}
$stage=Join-Path $repo ('out\packages\0.2.0-'+$dll.sha256.Substring(0,12))
$stagedBundle=Join-Path $stage 'HamloEcho.vst3'
New-Item -ItemType Directory -Path $stagedBundle -Force | Out-Null
foreach ($file in $validated.files) {
    $destination=Join-Path $stagedBundle $file.path
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $bundle $file.path) -Destination $destination -Force
}
$documentation=Join-Path $stagedBundle 'Documentation'
New-Item -ItemType Directory -Path $documentation -Force | Out-Null
foreach ($name in 'README.txt','USER-GUIDE.txt','THIRD-PARTY-NOTICES.txt') { Copy-Item -LiteralPath (Join-Path $repo "docs\release\$name") -Destination $documentation -Force }
$licence=Join-Path $documentation 'ThirdParty\JUCE'
New-Item -ItemType Directory -Path $licence -Force | Out-Null
Copy-Item -LiteralPath build/JUCE-LICENSE.md -Destination (Join-Path $licence 'LICENSE.md') -Force
$zip=Join-Path $repo 'dist\HamloEcho-0.2.0-win64-vst3.zip'
Compress-Archive -LiteralPath $stagedBundle -DestinationPath $zip -CompressionLevel Optimal -Force
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[IO.Compression.ZipFile]::OpenRead($zip)
try {
    foreach ($file in Get-ChildItem -LiteralPath $stagedBundle -Recurse -File) {
        $relative=$file.FullName.Substring($stagedBundle.Length+1).Replace('\','/')
        $entry=$archive.Entries | Where-Object { $_.FullName.Replace('\','/') -eq "HamloEcho.vst3/$relative" }
        if (!$entry) { throw "ZIP omits $relative" }
        $stream=$entry.Open();$hash=[Security.Cryptography.SHA256]::Create()
        try { $actual=[BitConverter]::ToString($hash.ComputeHash($stream)).Replace('-','').ToLowerInvariant() } finally { $hash.Dispose();$stream.Dispose() }
        if ($actual -ne (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()) { throw "ZIP payload mismatch $relative" }
    }
} finally { $archive.Dispose() }
$examples=Join-Path $repo 'dist\HamloEcho-0.2.0-audio-examples.zip'
if (!(Test-Path -LiteralPath out/presets/demos/echo.bb.drums.06/identity.json)) { throw 'Prepared demonstrations are missing' }
foreach ($id in 'echo.bb.drums.06','echo.trap.vocal.09','echo.techno.synth.06','echo.dnb.fx.03') {
    $demo=Get-Content (Join-Path $repo "out\presets\demos\$id\identity.json") -Raw | ConvertFrom-Json
    if ($demo.dsp_source -ne $validated.dsp_source -or !$demo.verification.passed) { throw 'Stale or failed demonstration' }
}
Compress-Archive -LiteralPath (Join-Path $repo 'out\presets\demos') -DestinationPath $examples -CompressionLevel Optimal -Force
$artifacts=@()
foreach ($file in @((Join-Path $repo 'dist\HamloEcho_VST3_Setup_0.2.0.exe'),$zip,$examples)) {
    $info=Get-Item -LiteralPath $file;$hash=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $($info.Name)" | Set-Content -LiteralPath "$file.sha256" -Encoding ascii
    $artifacts += [pscustomobject]@{name=$info.Name;sha256=$hash;bytes=$info.Length}
}
$manifest=[pscustomobject]@{version='0.2.0';dsp_source=$validated.dsp_source;plugin_sha256=$dll.sha256;core_payload=$validated.files;zip_payload_verified=$true;installer_inputs_unchanged=$true;installer_method='ISCC source payload inventory pinned before/after compilation; installer not executed';artifacts=$artifacts;portable_bundle=$stagedBundle;owner_fl_audition='pending'}
$manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $ReportRoot 'package_manifest.json') -Encoding utf8
$manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath dist/release-manifest.json -Encoding utf8
Write-Output 'Installer, ZIP, demonstrations and SHA256 prepared. Installer has not been executed.'
