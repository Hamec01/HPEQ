# HamloEcho 0.2.2 — QUICK Note Score

The owner requested a beginner version of Note Score while retaining the complete editor. QUICK is a separate page in the same VST3, with three large TIME / MOTION / SPACE knobs, eight moves and a separate MIX slider. It controls the existing Score engine; Legacy is retained separately. Default stays bit-identical to dry in Insert until Mix rises.

ECHO, TAPE, BOUNCE, NOTE FALL, NOTE RISE, REVERSE, TEXTURE and CLOUD provide clear repeats, loop coloration, stereo movement, falling/rising intervals, cumulative feedback pitch, backwards capture, chopping and spatial textures. Motion adds up to four reflections and 68% shared loop feedback; Space blends the existing spatial field and increases decay to 6 s, or 12 s for Cloud. Pitch remains source-relative, with the existing capture and bandwidth limits.

Entering QUICK leaves the detailed score stored. Full-page tabs recall that score. EDIT IN NOTE SCORE instead transfers the effective recipe using canonical host conversions and retains the running sound. Mix, wet Output, Insert/Send, Mix Lock, Freeze and note-label convention remain shared. Project state saves the macros and underlying score. Loading Default or a factory program disables QUICK.

Six parameters are appended after the previous 143 Score parameters. Old IDs, ranges, metadata, programs, XML root and VST3 class IDs remain intact. Old projects loaded over an edited QUICK instance reset new controls. Recipe fingerprints migrate so clean old Score projects keep their clean preset label, while modified projects stay marked modified. Program count remains 208; the eight moves are controllers rather than new host programs. Shared PresetScreen files and all Source/DSP files are unchanged.

## Acceptance evidence

Methods were recorded before code in [QUICK_METHODS.md](../../QUICK_METHODS.md). Baseline: commit257a863, neutral Default0.2.1 with1234 expectations and160 M0 cases.

- Final native suite: **4197 expectations passed, 0 failed**. The exact output is in native_tests.txt. Tests cover bounded deterministic recipes, all eight audible moves, maximum controls, state round trips, old-state reset and labels, zero-effect Default, and unchanged factory-program entry.
- EDIT transfer: every applied raw parameter matches the virtual recipe exactly; continuous audio before/after transfer matches the still-running QUICK reference bit-for-bit for all eight moves.
- Macro/style/tempo events at identical sample positions: blocks1/64/127/512/4096 after prepare256 match bit-for-bit. Audio allocation audit reports zero allocations, including move and macro changes.
- All **160 immutable M0** states/renders pass with the unchanged lab/reference signals. Command: `build/Release/HamloEcho_Lab.exe verify E:/HamloEcho/out/m0/fixtures E:/HamloEcho/docs/reports/quick_022/legacy_160.json`.
- The 200 compiled genre recipes retain every old parameter value, ID and metadata field; appended controls have neutral defaults with QUICK disabled. All DSP files match the original calibrated source byte-for-byte. See equivalence.json. Original calibration reports remain historical evidence supported by this explicit equivalence; no fresh 200-preset calibration is claimed and stale-source guards remain intact.
- Pluginval5–10 and Steinberg validator **537/537**, no failures. The wrapper reports178 parameters including its extra Program selector. Main QUICK at1180 and885px, detailed Reflections/Space/Glitch and Legacy snapshots were inspected: preset screen remains top left; no control overlap.
- Installer0.2.2, portable ZIP, loose `dist/HamloEcho.vst3` and SHA256 files prepared. Every ZIP/loose file is hash-compared to staging; installer core inputs remain identical to the validated VST3 before/after compilation. package_manifest.json records final source/binary/artifact hashes. Installer was not executed; the owner's FL Studio session was not changed.

Final review corrected the beginner TIME knob's duration ordering: the full editor's existing choice indices put some dotted/triplet values out of duration order. QUICK now maps its knob monotonically without changing those original choices/IDs, and typed divisions round-trip correctly. Added22 assertions, then rebuilt and repeated dependent tests, M0, snapshots, validators and packaging. Earlier4175-pass controller results are superseded by the final4197-pass run; acceptance thresholds were not relaxed.

Packaging first encountered a Windows PowerShell module-path mismatch when launched through Python. The packaging tool now uses the environment's PowerShell7 and can resume verified staging. The existing Inno script emits a deprecated GetFileVersion-name warning; compilation succeeds. No runtime-signature checks were bypassed.

## Remaining scope and practical limits

Owner listening in FL Studio remains pending. Manual source-reference pitch, 80 ms minimum pitched capture, additional Reverse/fragment windows, effective4s delay ceiling, runtime-only Freeze audio and unscaled Send headroom remain documented in the user guide. Earlier Program-selector normalisation and Windows CPU outliers remain disclosed in the0.2.0 report. This controller patch does not claim new CPU deadline guarantees or remove those limitations. No Verb/HPEQ, subtree, push or subagent work is included; no model switch was performed.

The latest VST3 bundle is `E:/HamloEcho/dist/HamloEcho.vst3` (copy the entire folder), with an identical build bundle at `build/HamloEcho_artefacts/Release/VST3/HamloEcho.vst3`. Installer: `dist/HamloEcho_VST3_Setup_0.2.2.exe`; ZIP: `dist/HamloEcho-0.2.2-win64-vst3.zip`.
