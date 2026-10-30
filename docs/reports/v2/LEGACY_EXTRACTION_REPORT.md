# HamloEcho Echo-only Legacy extraction

2026-10-30. This is a compatibility stage, not a 0.2.0 sound-quality release. Version remains 0.1.0.

## Result

The standalone plugin owns its Echo processing and no longer builds LayerEngine, FxNodeProcessor, the processor pools, other effects or their panels. 51 unchanged unused M0 files were removed; Source now consists of 26 files. The shared library's cpp count fell from 35 to 10. Existing recipes and audio laws were preserved rather than redesigned.

| Check | Result |
|---|---|
| Original M0 unit executable | 109 expectations passed, 0 failed |
| M0 lab capture and self-check | 160 captured; 160/160 self-verifications |
| Extracted Legacy vs archived M0 | 160/160 raw float renders bit-identical; all finite |
| Parameter contract | Same 28 parameters, IDs, names, ranges, normalisation, defaults, labels and order |
| Old state restore | All 160 states byte-identical after loading on fresh and already edited instances |
| Current unit executable | 590 expectations passed, 0 failed; no skipped archive checks on this machine |
| Latency | 0 samples across all capture cases |
| Programs | Default + seven original recipes, same positions and names |
| pluginval | Levels 5,6,7,8,9,10 passed; seed 0x484d4563, default rates/blocks and GUI tests enabled |
| Steinberg | 537 passed, 0 failed; exit 0 |
| Editor | Snapshot generated and visually inspected; CRT preset screen remains top left, existing controls legible |
| VST3 identity | Original processor/controller class IDs retained; see checked moduleinfo and validator class inventory |

The measured DLL size changed from 12,561,408 to 10,702,848 bytes. This is a binary-size observation, not a CPU or RAM claim.

## Evidence and commands

Baseline description: [M0_BASELINE.md](M0_BASELINE.md). Baseline executable/source hashes: [M0_SHA256.txt](M0_SHA256.txt). All rendered case hashes: [before.json](before.json) and [legacy_after.json](legacy_after.json). Host contract: [m0_contract.json](m0_contract.json). State coverage: `Tests/LegacyStateTests.cpp`. UI: [legacy_core.png](legacy_core.png). Validator output and exit summary: `validators/`.

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DHAMLO_ECHO_JUCE_DIR=E:/HPEQ/build/_deps/juce-src
cmake --build build --config Release --target HamloEcho_Lab HamloEcho_Tests HamloEcho_VST3 HamloEcho_Snapshot -- /m:2 /v:minimal
build/Release/HamloEcho_Lab.exe verify E:/HamloEcho/out/m0/fixtures E:/HamloEcho/docs/reports/v2/legacy_after.json
build/Release/HamloEcho_Tests.exe
build/Release/HamloEcho_Snapshot.exe docs/reports/v2/legacy_core.png 1
powershell -NoProfile -ExecutionPolicy Bypass -File Tools/validate_legacy.ps1
```

The frozen lab source was not changed between capture and extraction verification. The old states and raw renders are in machine-local ignored `out/m0/fixtures`; archive-dependent unit checks explicitly skip on a checkout without those artifacts. The archived original VST3/lab/tests are separate from normal build output and must not be overwritten.

## Limits and remaining work

E01 is demonstrated within the captured Legacy test matrix. Coverage includes sample rates 44.1/48/96 kHz, mono/stereo, old programs, control boundaries, both old delay engines, time modes and event automation; host fallback tempo is 120 BPM. Tempo automation and new-parameter migration are future checks. Old Classic/Echo switching is intentionally compared using equal block schedules, preserving its old block dependence.

There are no new note voices, score editor, independent spectral bands, new spatial/glitch effects, Send mode or 200 new presets in this stage. Their goals remain pending/n/a. Audio-thread allocation instrumentation, pitch/transient/alias measurements, CPU limits and full Freeze stress belong to the upcoming new-engine acceptance work. The Legacy screenshot retains the original artwork, including its existing markings; new product controls/help are to be English.

FL Studio audition of this extracted VST3 and an installer run were not performed in this stage. Automated validation does not substitute for those release checks. No 0.2.0 installer or release-ready claim is made. HPEQ/Verb sources were not edited, and no subtree/push occurred.
