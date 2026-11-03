[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$repoPath = Split-Path $PSScriptRoot -Parent
$evidence = Get-Content -LiteralPath (Join-Path $repoPath 'docs/reports/s5/acceptance.json') -Raw | ConvertFrom-Json
$bundlePath = Join-Path $repoPath 'build/HamloZastava_artefacts/Release/VST3/HamloZastava.vst3'
$dllRelative = 'Contents/x86_64-win/HamloZastava.vst3'
$dllHash = (Get-FileHash -LiteralPath (Join-Path $bundlePath $dllRelative) -Algorithm SHA256).Hash.ToLowerInvariant()
if ($dllHash -ne $evidence.dll_sha256 -or $evidence.library_passed -ne 200) { throw 'Release is not accepted' }
$releasePath = Join-Path $repoPath 'dist/HamloZastava-0.2.0-VST3'
$zipPath = Join-Path $repoPath 'dist/HamloZastava-0.2.0-win64-vst3.zip'
if ((Test-Path -LiteralPath $releasePath) -or (Test-Path -LiteralPath $zipPath)) { throw 'Refusing to overwrite release' }
New-Item -ItemType Directory -Path $releasePath | Out-Null
Copy-Item -LiteralPath $bundlePath -Destination $releasePath -Recurse
Copy-Item -LiteralPath (Join-Path $repoPath 'docs/release/README.txt') -Destination $releasePath
$documentationPath = Join-Path $releasePath 'Documentation'
$licensePath = Join-Path $documentationPath 'ThirdParty/JUCE'
New-Item -ItemType Directory -Path $licensePath | Out-Null
Copy-Item -LiteralPath (Join-Path $repoPath 'docs/reports/s5/REPORT.md') -Destination $documentationPath
Copy-Item -LiteralPath (Join-Path $repoPath 'docs/ZASTAVA_CONTROLS.md') -Destination $documentationPath
Copy-Item -LiteralPath (Join-Path $repoPath 'docs/GENRE_PRESETS.md') -Destination $documentationPath
Copy-Item -LiteralPath (Join-Path $repoPath 'docs/release/THIRD-PARTY-NOTICES.txt') -Destination $documentationPath
Copy-Item -LiteralPath 'E:/HPEQ/build/_deps/juce-src/LICENSE.md' -Destination $licensePath
$sourceFiles = Get-ChildItem -LiteralPath $bundlePath -File -Recurse
foreach ($file in $sourceFiles) {
    $relativePath = [IO.Path]::GetRelativePath($bundlePath, $file.FullName)
    $copyPath = Join-Path (Join-Path $releasePath 'HamloZastava.vst3') $relativePath
    if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $copyPath).Hash) { throw "Bundle copy mismatch: $relativePath" }
}
[IO.Compression.ZipFile]::CreateFromDirectory($releasePath, $zipPath)
$archive = [IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $entry = $archive.GetEntry('HamloZastava.vst3/' + $dllRelative)
    if ($null -eq $entry) { throw 'Missing VST DLL in ZIP' }
    $stream = $entry.Open()
    try { $algorithm = [Security.Cryptography.SHA256]::Create(); $zipDllHash = [Convert]::ToHexString($algorithm.ComputeHash($stream)).ToLowerInvariant() } finally { $stream.Dispose() }
    if ($zipDllHash -ne $dllHash) { throw 'ZIP binary differs from validated VST' }
} finally { $archive.Dispose() }
$zipHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
@("$zipHash  $([IO.Path]::GetFileName($zipPath))", "$dllHash  HamloZastava-0.2.0-VST3/HamloZastava.vst3/$dllRelative") | Set-Content -LiteralPath (Join-Path $repoPath 'dist/SHA256SUMS-0.2.0.txt') -Encoding ascii
@{ version='0.2.0'; dll_sha256=$dllHash; zip_sha256=$zipHash; zip=$zipPath; release=$releasePath; bundle_files=$sourceFiles.Count; zip_dll_exact=$true; installed_plugin_changed=$false; installer=$false } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $repoPath 'docs/reports/s5/package.json') -Encoding utf8
Write-Output "Packaged exact validated VST3: $zipPath"
