# HamloEcho M0 baseline

Captured 2026-10-30 before changing any audio processing. Original audio source revision: ede0db8 (M0 extraction a3d9627). The lab was added without changing processing and built Release with JUCE 8.0.12, MSVC / Visual Studio 18 2026, x64.

- Original unit executable: 109 expectations passed, zero failures.
- EchoLab capture: 160 cases, zero failures, all finite, reported latency zero throughout.
- Archived executable self-verification: 160/160 raw float renders bit-identical; 160/160 initial states restore after editing every parameter and bypass on the destination instance; host contract identical.
- Cases include all eight programs at 44.1/48/96 kHz, mono/stereo; min/max for every specified control; all five characters, both time modes and host blocks 1/64/127/512/4096; sample-positioned automation; bypass retirement and restart.
- Long free-time boundary cases last 4.5 s. Other cases last 2 s, automation/restart cases 3 s. Inputs are reproducible gated stereo tones, noise and asymmetric impulses specified in the lab source. Host tempo is the original fallback 120 BPM; host tempo automation is not yet covered by these M0 fixtures.
- Legacy character-switching results are compared at equal block schedules. They are not falsely required to match each other across schedules.
- New score/pitch-note/bands/space/glitch/Send measurements: n/a, features absent in M0.

## Artifacts

Machine-local immutable archive (ignored by Git): `E:/HamloEcho/out/m0/`. Contains the original VST3 bundle, original tests, frozen lab executable, 160 initial binary states and planar float renders under `fixtures/`, and capture/selfcheck logs. Do not overwrite this directory during normal builds. Binary and lab-source identities: [M0_SHA256.txt](M0_SHA256.txt). Every render and initial-state SHA256 is recorded in [before.json](before.json). Parameter/program contract: [m0_contract.json](m0_contract.json). Self-verification: [m0_selfcheck.json](m0_selfcheck.json).

Reproduce verification using the preserved executable:

```powershell
out/m0/HamloEcho_Lab.exe verify E:/HamloEcho/out/m0/fixtures E:/HamloEcho/out/m0/recheck.json
```

Verify an extracted build using the same lab source:

```powershell
build/Release/HamloEcho_Lab.exe verify E:/HamloEcho/out/m0/fixtures E:/HamloEcho/docs/reports/v2/legacy_after.json
```

No sound-quality improvement is claimed by this baseline. Musical material, pitch quality, spectral bands, complete feedback/Freeze stress, allocation audit and performance measurements remain subsequent acceptance work.
