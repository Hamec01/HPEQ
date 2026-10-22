# Builds the Windows x64 beta package from an existing Release build:
#   dist/HPEQ-<version>-win64.zip  (HPEQ.vst3 bundle, INSTALL.txt, RELEASE_NOTES.md)
# and prints its SHA-256. Not code-signed (no signing infrastructure yet).
#
#   powershell -ExecutionPolicy Bypass -File scripts/package.ps1

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$cmake = Get-Content (Join-Path $root 'CMakeLists.txt') -Raw
if ($cmake -notmatch 'project\(HPEQ VERSION ([0-9.]+)') { throw 'version not found in CMakeLists.txt' }
$version = $Matches[1]
$suffix = '-beta'

$bundle = Join-Path $root 'build/HPEQ_artefacts/Release/VST3/HPEQ.vst3'
if (-not (Test-Path $bundle)) { throw "Release build missing: $bundle (cmake --build --preset release)" }

$name = "HPEQ-$version$suffix-win64"
$dist = Join-Path $root 'dist'
$stage = Join-Path $dist $name
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage | Out-Null

Copy-Item -Recurse $bundle (Join-Path $stage 'HPEQ.vst3')
Copy-Item (Join-Path $root "docs/release/RELEASE_NOTES_$version$suffix.md") (Join-Path $stage 'RELEASE_NOTES.md')
Copy-Item (Join-Path $root 'docs/release/INSTALL.txt') (Join-Path $stage 'INSTALL.txt')

$zip = Join-Path $dist "$name.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
$hash = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLower()
Set-Content -Encoding ascii (Join-Path $dist "$name.zip.sha256") "$hash  $name.zip"
Write-Output "$zip"
Write-Output "sha256 $hash"
