# Builds the Windows x64 beta package from an existing Release build:
#   dist/HamloRack-<version>-beta-win64.zip
# and prints its SHA-256. Not code-signed (no signing infrastructure yet).
#
#   powershell -ExecutionPolicy Bypass -File scripts/package.ps1

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$cmake = Get-Content (Join-Path $root 'CMakeLists.txt') -Raw
if ($cmake -notmatch 'project\(HPEQ VERSION ([0-9.]+)') { throw 'version not found in CMakeLists.txt' }
$version = $Matches[1]
$suffix = '-beta'

$bundle = Join-Path $root 'build/HPEQ_artefacts/Release/VST3/HamloRack.vst3'
if (-not (Test-Path $bundle)) { throw "Release build missing: $bundle (cmake --build --preset release)" }

$name = "HamloRack-$version$suffix-win64"
$dist = Join-Path $root 'dist'
if ((Test-Path -LiteralPath $dist) -and ((Get-Item -LiteralPath $dist -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Package output must be a regular directory' }
$stage = [IO.Path]::GetFullPath((Join-Path $dist $name))
if ([IO.Path]::GetDirectoryName($stage) -ne [IO.Path]::GetFullPath($dist)) { throw 'Invalid package stage path' }
if ((Test-Path -LiteralPath $stage) -and ((Get-Item -LiteralPath $stage -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Package stage must not be a link' }
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null

Copy-Item -Recurse $bundle (Join-Path $stage 'HamloRack.vst3')
Copy-Item (Join-Path $root 'docs/release/HAMLO_RACK_BETA.md') (Join-Path $stage 'RELEASE_NOTES.md')
Copy-Item (Join-Path $root 'docs/release/INSTALL.txt') (Join-Path $stage 'INSTALL.txt')
$validation = Join-Path $stage 'docs/hamlorack'
New-Item -ItemType Directory -Force -Path $validation | Out-Null
foreach ($report in @('RACK_COMPLETE.md','RACK_COMPLETE.json','RACK_BANK_REPORT.md','RACK_BANK_RECEIPT.json','RACK_BANK_METHOD.md')) {
    Copy-Item -LiteralPath (Join-Path $root "docs/hamlorack/$report") -Destination $validation
}

$zip = Join-Path $dist "$name.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
$hash = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLower()
Set-Content -Encoding ascii (Join-Path $dist "$name.zip.sha256") "$hash  $name.zip"
Write-Output "$zip"
Write-Output "sha256 $hash"
