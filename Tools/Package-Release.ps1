[CmdletBinding()]
param([string]$CompilerPath = 'C:\DRUMENGINE\build\tools\InnoSetup\ISCC.exe')
$ErrorActionPreference = 'Stop'
$magRoot = Split-Path $PSScriptRoot -Parent
Push-Location $magRoot
try {
    $magCmake = Get-Content CMakeLists.txt -Raw
    if ($magCmake -notmatch 'project\(HamloMag VERSION (\d+\.\d+\.\d+)') { throw 'Missing version' }
    $magVersion = $Matches[1]
    & "$magRoot\Installer\Build-Installer.ps1" -SkipBuild -CompilerPath $CompilerPath
    if ($LASTEXITCODE -ne 0) { throw 'Installer packaging failed' }
    $magSource = "$magRoot\build\HamloMag_artefacts\Release\VST3\HamloMag.vst3"
    Copy-Item -LiteralPath $magSource -Destination "$magRoot\dist" -Recurse -Force
    $magDocs = "$magRoot\dist\HamloMag.vst3\Documentation"
    New-Item -ItemType Directory -Force "$magDocs\ThirdParty\JUCE" | Out-Null
    Copy-Item docs/release/README.txt, docs/release/THIRD-PARTY-NOTICES.txt $magDocs -Force
    Copy-Item build/JUCE-LICENSE.md "$magDocs\ThirdParty\JUCE\LICENSE.md" -Force
    $magZip = "$magRoot\dist\HamloMag-$magVersion-win64-vst3.zip"
    Compress-Archive -Path "$magRoot\dist\HamloMag.vst3" -DestinationPath $magZip -Force
    $magHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $magZip).Hash.ToLower()
    Set-Content -Encoding ascii "$magZip.sha256" "$magHash  $(Split-Path $magZip -Leaf)"
    Write-Output $magZip
    Write-Output "sha256 $magHash"
}
finally { Pop-Location }
