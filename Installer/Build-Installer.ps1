# Builds HamloVerb (Release) and its installer: dist\HamloVerb_VST3_Setup_<version>.exe (+ .sha256).
#   powershell -ExecutionPolicy Bypass -File Installer\Build-Installer.ps1 [-SkipBuild] [-CompilerPath <ISCC.exe>]
# Inno Setup 6 and the Microsoft VC++ runtime are downloaded into build\ when missing (signatures checked).
[CmdletBinding()]
param([switch]$SkipBuild, [string]$CompilerPath)
$ErrorActionPreference = 'Stop'
$repoDir = Split-Path $PSScriptRoot -Parent
Push-Location $repoDir
try {
    $cmake = Get-Content (Join-Path $repoDir 'CMakeLists.txt') -Raw
    if ($cmake -notmatch 'project\(HamloVerb VERSION ([0-9]+)\.([0-9]+)\.([0-9]+)') { throw 'version not found in CMakeLists.txt' }
    $version = "$($Matches[1]).$($Matches[2]).$($Matches[3])"
    $fileVersion = "$version.0"
    Write-Output "HamloVerb version $version"

    $toolDir = Join-Path $repoDir 'build\tools'
    $runtimeFile = Join-Path $repoDir 'build\prerequisites\vc_redist.x64.exe'
    New-Item -ItemType Directory -Force $toolDir, (Split-Path $runtimeFile), (Join-Path $repoDir 'dist') | Out-Null
    if (!(Test-Path -LiteralPath $runtimeFile)) {
        Invoke-WebRequest 'https://aka.ms/vc14/vc_redist.x64.exe' -OutFile $runtimeFile
    }
    $runtimeSignature = Get-AuthenticodeSignature -LiteralPath $runtimeFile
    if ($runtimeSignature.Status -ne 'Valid' -or $runtimeSignature.SignerCertificate.Subject -notmatch 'Microsoft Corporation') {
        throw 'Visual C++ Runtime must have a valid Microsoft signature.'
    }
    if (!$CompilerPath) {
        $CompilerPath = Join-Path $toolDir 'InnoSetup\ISCC.exe'
        if (!(Test-Path -LiteralPath $CompilerPath)) {
            $compilerSetup = Join-Path $toolDir 'innosetup-6.7.3.exe'
            Invoke-WebRequest 'https://github.com/jrsoftware/issrc/releases/download/is-6_7_3/innosetup-6.7.3.exe' -OutFile $compilerSetup
            $signature = Get-AuthenticodeSignature -LiteralPath $compilerSetup
            if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Pyrsys') { throw 'Invalid Inno Setup signature.' }
            $portableDir = Split-Path $CompilerPath
            $process = Start-Process -FilePath $compilerSetup -ArgumentList ('/PORTABLE=1 /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CURRENTUSER /DIR="' + $portableDir + '"') -WindowStyle Hidden -PassThru -Wait
            if ($process.ExitCode -ne 0) { throw "Inno Setup extraction failed: $($process.ExitCode)" }
        }
    }
    if (!$SkipBuild) {
        & cmake --build build --config Release --target HamloVerb_VST3 --parallel
        if ($LASTEXITCODE -ne 0) { throw 'VST3 build failed.' }
    }
    $vst3 = Join-Path $repoDir 'build\HamloVerb_artefacts\Release\VST3\HamloVerb.vst3'
    if (!(Test-Path -LiteralPath $vst3)) { throw "Release build missing: $vst3" }

    # JUCE's licence travels with the plug-in (Documentation\ThirdParty\JUCE)
    $cache = Get-Content (Join-Path $repoDir 'build\CMakeCache.txt')
    $juceDir = ($cache | Where-Object { $_ -match '^HAMLO_VERB_JUCE_DIR:PATH=(.+)$' } | ForEach-Object { $Matches[1] }) | Select-Object -First 1
    if (!$juceDir) { $juceDir = Join-Path $repoDir 'build\_deps\juce-src' }
    Copy-Item -Force (Join-Path $juceDir 'LICENSE.md') (Join-Path $repoDir 'build\JUCE-LICENSE.md')

    & $CompilerPath "/DMyAppVersion=$version" "/DMyFileVersion=$fileVersion" (Join-Path $PSScriptRoot 'HamloVerb.iss')
    if ($LASTEXITCODE -ne 0) { throw 'Inno Setup failed.' }
    $setup = Join-Path $repoDir "dist\HamloVerb_VST3_Setup_$version.exe"
    $hash = (Get-FileHash -Algorithm SHA256 $setup).Hash.ToLower()
    Set-Content -Encoding ascii "$setup.sha256" "$hash  HamloVerb_VST3_Setup_$version.exe"
    Write-Output $setup
    Write-Output "sha256 $hash"
}
finally {
    Pop-Location
}
