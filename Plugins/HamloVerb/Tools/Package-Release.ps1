[CmdletBinding()]
param([string]$CompilerPath = 'C:\DRUMENGINE\build\tools\InnoSetup\ISCC.exe')
$ErrorActionPreference = 'Stop'
$verbRoot = Split-Path $PSScriptRoot -Parent
Push-Location $verbRoot
try {
    $bundle = Join-Path $verbRoot 'build\HamloVerb_artefacts\Release\VST3\HamloVerb.vst3'
    $binary = Join-Path $bundle 'Contents\x86_64-win\HamloVerb.vst3'
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $binary).Hash.ToLower()
    $validated = (Get-Content -LiteralPath 'docs\reports\v2\validated_binary.sha256' -Raw).Split(' ')[0].Trim()
    if ($hash -ne $validated) { throw 'The current binary has not passed release validation' }
    $presets = Get-Content -LiteralPath 'docs\reports\presets\check_all.json' -Raw | ConvertFrom-Json
    if ($presets.passed -ne 200 -or $presets.failed -ne 0) { throw 'Preset validation is incomplete' }
    if ((Get-Content CMakeLists.txt -Raw) -notmatch 'project\(HamloVerb VERSION ([0-9.]+)') { throw 'Version not found' }
    $version = $Matches[1]
    & powershell -NoProfile -ExecutionPolicy Bypass -File Installer\Build-Installer.ps1 -SkipBuild -CompilerPath $CompilerPath
    if ($LASTEXITCODE -ne 0) { throw 'Installer build failed' }
    $stage = Join-Path $verbRoot ('build\package\run-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff'))
    New-Item -ItemType Directory -Force -Path $stage | Out-Null
    Copy-Item -LiteralPath $bundle -Destination $stage -Recurse
    $stagedBundle = Join-Path $stage 'HamloVerb.vst3'
    $documentation = Join-Path $stagedBundle 'Documentation'
    New-Item -ItemType Directory -Force -Path $documentation | Out-Null
    Copy-Item -LiteralPath 'docs\release\README.txt','docs\release\THIRD-PARTY-NOTICES.txt' -Destination $documentation
    $juceDocumentation = Join-Path $documentation 'ThirdParty\JUCE'
    New-Item -ItemType Directory -Force -Path $juceDocumentation | Out-Null
    Copy-Item -LiteralPath 'build\JUCE-LICENSE.md' -Destination (Join-Path $juceDocumentation 'LICENSE.md')
    $portable = Join-Path $verbRoot "dist\HamloVerb-$version-win64-vst3.zip"
    Compress-Archive -LiteralPath $stagedBundle -DestinationPath $portable -Force
    $distBundle = Join-Path $verbRoot 'dist\HamloVerb.vst3'
    if (Test-Path -LiteralPath $distBundle) {
        # Both resolved paths are inside this repository before moving a directory.
        $checkedSource = [IO.Path]::GetFullPath($distBundle)
        $checkedDestination = [IO.Path]::GetFullPath((Join-Path $stage 'previous-dist'))
        $allowedRoot = [IO.Path]::GetFullPath($verbRoot).TrimEnd('\') + '\'
        if (!$checkedSource.StartsWith($allowedRoot) -or !$checkedDestination.StartsWith($allowedRoot)) { throw 'Unsafe package path' }
        Move-Item -LiteralPath $checkedSource -Destination $checkedDestination
    }
    Copy-Item -LiteralPath $stagedBundle -Destination (Join-Path $verbRoot 'dist') -Recurse
    $copiedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $distBundle 'Contents\x86_64-win\HamloVerb.vst3')).Hash.ToLower()
    if ($copiedHash -ne $hash) { throw 'Portable bundle differs from validated binary' }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead($portable)
    try {
        $entry = $archive.Entries | Where-Object { $_.FullName.Replace('\','/') -eq 'HamloVerb.vst3/Contents/x86_64-win/HamloVerb.vst3' }
        if (!$entry) { throw 'VST3 binary missing from ZIP' }
        $stream = $entry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $zipHash = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLower() }
        finally { $stream.Dispose(); $sha.Dispose() }
        if ($zipHash -ne $hash) { throw 'ZIP binary differs from validated binary' }
    } finally { $archive.Dispose() }
    $zipHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $portable).Hash.ToLower()
    Set-Content -Encoding ascii -LiteralPath ($portable+'.sha256') -Value ($zipHash+'  '+[IO.Path]::GetFileName($portable))
    $setupPath = Join-Path $verbRoot "dist\HamloVerb_VST3_Setup_$version.exe"
    $setupHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $setupPath).Hash.ToLower()
    $demos = Join-Path $verbRoot 'build\demos'
    if ((Get-ChildItem -LiteralPath $demos -Directory).Count -ne 20) { throw 'Audio examples are incomplete' }
    Copy-Item -LiteralPath $demos -Destination (Join-Path $stage 'AudioExamples') -Recurse
    Copy-Item -LiteralPath (Join-Path $stage 'AudioExamples') -Destination (Join-Path $verbRoot 'dist') -Recurse -Force
    $manifest = @("Validated VST3: $hash", "Portable ZIP: $zipHash", "Installer: $setupHash", "Version: $version", "Preset source/DSP: $($presets.dspHash)", 'Audio examples: 20 sets (dry / Insert / Send)')
    Set-Content -Encoding ascii -LiteralPath 'docs\reports\v2\PACKAGE_MANIFEST.txt' -Value $manifest
    Write-Output ($manifest -join "`n")
} finally { Pop-Location }
