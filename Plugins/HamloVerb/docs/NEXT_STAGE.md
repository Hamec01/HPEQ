# HamloVerb 0.2.0 release handoff — 2026-10-30

## Delivered work

Standalone core and Legacy compatibility, six new characters, Decay/Post EQ6, Gated/Reverse Swell/
Reverse Audio/Shimmer, stable Freeze, two-path handover, Insert/Send, Mix Lock, standalone UI and
200 calibrated genre recipes are implemented. Production version: 0.2.0.

- M0 compatibility: 390/390 bit-exact.
- Final unit tests: 1154/1154.
- Final-DSP calibration: 200/200; independent compiled-bank check: 200/200.
- pluginval5..10 and Steinberg537/537 passed on the packaged binary.
- Five-minute Freeze stress passed for all6 characters; spectral and temporal contracts checked.
- Main/EQ/Texture/Effects/Legacy/minimum-size snapshots reviewed.
- Installer, portable ZIP, VST3, SHA256 manifests and20 dry/Insert/Send audio sets are in dist/.

Read docs/reports/v2/V2_REPORT.md for exact measurements and disclosures,
docs/reports/v2/PACKAGE_MANIFEST.txt for package hashes, and docs/GENRE_PRESETS.md for the bank.

## Explicit limits

Manual final-VST3 loading/listening in FL Studio and administrator installer execution are unverified.
Protected Program Files destinations denied writes; an alternate-folder dialog did not accept UI
automation. A separate test FL instance was closed; the owner's trap.flp instance was untouched.
Do not describe these as passes. The installer is compiled and its exact input binary is validated.

The full after report keeps7 CPU failures under concurrent workloads. The same isolated test passed
at2.91..4.91% for one character and8.89% for handover. The report explains both results; no goals moved.

## Provenance and commands

M0 baseline02446a7, design/fixturesdc6f952, standalone extractiondbd2000, V2 implementationa418499.
Subsequent release commits hold the reports/tools. Use git log/status for the latest revision.
Validated VST3 SHA256: b3728742ce6b59bd65293dff53a972fdeb5b3a75dff1dd05cdaccad82b6edbae.

```powershell
cmake --build build --config Release
build\Release\HamloVerb_Tests.exe
build\Release\HamloVerb_Reference.exe check build\reference\m0 docs\reports\v2\v2_legacy_reference.md
build\Release\HamloVerb_Lab.exe docs\reports\v2\cpu_isolated.md V2-isolated cpu
python -I Tools\run_preset_lab.py check <material-directory>
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Validate-Release.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Package-Release.ps1
```

Fixtures are in build/reference/m0 or docs/reports/v2/m0-fixtures.zip. Do not overwrite M0 fixtures.
Calibration caches require matching DSP/source/material/recipe hashes; never reuse stale results.
Final audio source directory is recorded in each example description. No background job is required.

No HPEQ source, subtree or remote work was performed. No push. HamloMag is finished; do not redo it.
No subagents. Continue with another plugin only when requested; do not silently expand this release.
