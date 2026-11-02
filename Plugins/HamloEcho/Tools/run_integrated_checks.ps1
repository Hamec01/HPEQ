param([string]$Run = 'stable_020')
$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
cmake --build build --config Release --target HamloEcho_ScoreLab HamloEcho_ScorePresetLab HamloEcho_Tests HamloEcho_Snapshot HamloEcho_Lab HamloEcho_VST3 -- /m:2 /v:minimal *> "build/$Run-build.log"
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
& build/Release/HamloEcho_Tests.exe *> "build/$Run-tests.log"
if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
New-Item -ItemType Directory -Path "docs/reports/integration/$Run" -Force | Out-Null
# Keep CPU measurements quiet: before long captures/calibration, sequential.
& build/Release/HamloEcho_ScorePresetLab.exe bench "E:/HamloEcho/docs/reports/integration/$Run/plugin_cpu.json" *> "build/$Run-plugin-cpu.log"
if ($LASTEXITCODE -ne 0) { throw 'Plugin CPU tool failed' }
& build/Release/HamloEcho_ScoreLab.exe "out/score/$Run" bench *> "docs/reports/integration/$Run/core_cpu.txt"
if ($LASTEXITCODE -ne 0) { throw 'Core CPU tool failed' }
foreach ($mode in 'bandstress_raw','bandstress','effects','timing','freeze') {
    $directory=if ($mode -eq 'bandstress_raw') { "out/score/$Run/raw_feedback" } elseif ($mode -eq 'bandstress') { "out/score/$Run/bands_space" } else { "out/score/$Run" }
    & build/Release/HamloEcho_ScoreLab.exe $directory $mode *> "build/$Run-$mode.log"
    if ($LASTEXITCODE -ne 0) { throw "Score lab $mode failed" }
}
& C:/Users/Ham_h/AppData/Local/Programs/Python/Python311/python.exe -I Tools/analyse_score.py "out/score/$Run" "docs/reports/integration/$Run/effects.json" *> "build/$Run-analysis.log"
if ($LASTEXITCODE -ne 0) { throw 'Independent Score analysis failed' }
& build/Release/HamloEcho_Lab.exe verify E:/HamloEcho/out/m0/fixtures "E:/HamloEcho/docs/reports/integration/$Run/legacy_160.json" *> "build/$Run-legacy.log"
if ($LASTEXITCODE -ne 0) { throw 'Legacy archive mismatch' }
Write-Output 'Integrated checks complete; inspect CPU thresholds and all reports before calibration.'
