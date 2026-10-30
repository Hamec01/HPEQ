"""Assemble the release report only from completed measurement outputs."""
import hashlib
import json
import pathlib
import re

ROOT=pathlib.Path(__file__).resolve().parents[1]
def read(name):return (ROOT/name).read_text(encoding='utf-8-sig')
check=json.loads(read('docs/reports/presets/check_all.json'))
assert check['passed']==200 and check['failed']==0
tests=re.search(r'HamloVerb_Tests: (\d+) expectations passed, (\d+) failed',read('build/release_tests.log'))
assert tests and tests[2]=='0'
binary=ROOT/'build/HamloVerb_artefacts/Release/VST3/HamloVerb.vst3/Contents/x86_64-win/HamloVerb.vst3'
digest=hashlib.sha256(binary.read_bytes()).hexdigest()
assert digest==read('docs/reports/v2/validated_binary.sha256').split()[0]
cal=json.loads(read('docs/reports/presets/calibrate_all.json'))
assert cal['dspHash']==check['dspHash']
rows=list(check['presets'].values())
maximum=max(max(p['insertTruePeakDb'],p['sendTruePeakDb']) for p in rows)
report=f'''# HamloVerb 0.2.0 verification report — 2026-10-30

Six standalone characters, six-band Decay EQ and Post EQ, Gated, Reverse Swell, real Reverse Audio,
Shimmer, stable Freeze, two-path character handover, Insert/Send, Mix Lock and 200 genre presets.
The production target contains only Verb and its wrapper/UI. Legacy is the default for old-session compatibility.

## Acceptance

| Goal | Result | Evidence |
|---|---|---|
| V1 Legacy compatibility | PASS: 390/390 bit-exact | [Reference](v2_legacy_reference.md), preserved M0 states/hashes |
| V2 blocks and realtime constraints | PASS | 1/64/127/512/4096, static/timed automation, six engines; processBlock C++ allocation checks and DSP source review |
| V3 Clean decay | PASS | 0.1/0.25/1/2/10/30 s, T30 within max(10%,50 ms); 2 s measured 2.0007 s, 30 s measured 29.9980 s |
| V4 predelay | PASS | 20/250/500 ms shifts: 960/12000/24000 samples at 48 kHz |
| V5 density | PASS | NED .9: Clean 17.5 ms, Crisp 21.5 ms, Velvet 23.5 ms; goal <=150 ms |
| V6 Freeze | PASS | Each engine 300 s; maximum absolute audible drift 0.171 dB at 60 s, internal drift 0.000 dB; predictable release; size/diffusion automation null test |
| V7 stereo | PASS / characterized | Width0 exact; IR and [owner-material](material_stereo.md) correlation/fold-down diagnostics |
| V8 temporal effects | PASS | Gate residual after 400 ms below -60 dB; hold/swell semantic tests; reverse samples match independently indexed windows at 50/250/2000 ms |
| V9 Shimmer | PASS | Octave down/up/two octaves: >98% coherent target-tone energy; maximum-feedback finite/bounded stress |
| V10 CPU | PASS on isolated rerun | 2.91..4.91% mean per engine, 8.89% handover, i5-10300H, 48 kHz/256; goals 25%/50% |
| V11 state/workflow | PASS | Old states over modified V2, old bypass index22, additions afterward, 208 programs, routing/Mix Lock, filter-only and Load Start tests |
| V12 presets | PASS: 200/200 | [Independent compiled-bank check](../presets/check_all.md); maximum Insert/Send true peak {maximum:.3f} dBTP |
| V13 validators | PASS | pluginval 5..10; Steinberg extensive 537/537; exact binary hash below |
| V14 UI | PASS | Main/EQ/Texture/Effects/Legacy and 885x705 minimum screenshots reviewed; CRT fixed, no overlapping controls, contextual hints/tooltips |
| V15 distribution/host | PARTIAL | Installer/ZIP identity checked in PACKAGE_MANIFEST.txt; A/B renders supplied. FL Studio loading remains unverified; see below |

Final unit tests: **{tests[1]} passed, 0 failed**. No failed goal was relabeled as a pass.

## Measurements and method disclosures

[Before](before.md), [full after](after.md), [isolated CPU](cpu_isolated.md),
[98 independent spectral measurements](spectral_diagnostics.md), [first V2 run](first_v2_acoustic.md).
Existing acceptance functions and thresholds were retained. The spectral export/analysis is an additional
diagnostic and does not replace or relax a goal. The UI decay curve is a target model, not a measured response.
The independently measured Post EQ boost at 1 kHz is 6.0000 dB for a +6 dB setting.

The full after run passed 38 checks and failed seven wall-clock CPU rows while other compilation/calibration
jobs were active (single-engine means 29.62..55.15%, handover 96.68%). Those failures remain in after.md.
The same CPU test was rerun without concurrent jobs; it passed at 2.91..4.91% and 8.89%. No threshold changed.
The maximum individual block reached 41.58% in the isolated run; this is recorded, while the approved goal
is mean time. Scheduling jitter is not claimed absent.

The initial UI run had one failure (CRT at y110 instead of y20) while the old panel remained hosted.
The new panel fixed it without changing bounds in the test. A false modified marker from normalized
frequency rounding was fixed. Ordinary Freeze initially passed; subsequent source review found that live
Size/Diffusion changes could alter the held tank. Geometry/diffusion capture and a regression were added.
Bypass closes input; Freeze bypasses temporal shape so held audio is not gated away.

Two provisional calibration runs were interrupted during this DSP review. Their hash-keyed caches were
invalidated. Final calibration and independent compiled-bank check both use fingerprint
`{check['dspHash']}`. All 200 recipes pass depth, peak, finite, unique-sound and state-round-trip checks.
Uniqueness excludes MIX/Output; calibration never substitutes saturation/GR for reverb depth.

## Presets and audio examples

Boom Bap / Trap / Techno / DnB: 50 each, with Vocal10, Drums12, Samples/Keys10, Synth8, Atmosphere/FX10.
Old Factory presets retain programs1..7; new recipes occupy8..207. Each has an ID, purpose, source and tempo.
Depth bands: Gentle [-24,-16], Medium [-16,-8], Heavy [-8,+6] dB complete-render wet/dry energy.
Calibration uses the loudest available 10 seconds, normalized toward -18 dBFS RMS with peak <=-3 dBFS.
Tails are included; true peaks use 8x FIR reconstruction with flushing. Check mode renders actual Insert
and Send independently and reloads state. Output trim is common to dry/wet, so it does not change depth.

Twenty A/B sets (dry/Insert/Send) are in `dist/AudioExamples`, one per genre/material section.
They are listening material, not proof of subjective superiority over another product.

## Limits and the manual host check

Legacy retains additive MIX, Classic's 250 ms/20 s limits and its supported controls, and original
block-dependent Space switching. New routing/character/EQ controls are additive. Host latency is zero;
predelay, reverse accumulation and pitch windows are musical wet delays. Gate Release has an effective
5 ms floor. Shimmer is a granular-delay effect; no alias-rejection specification or competitor-equivalence
claim was tested. Overlapping Decay EQ boosts are bounded by feedback stability and may shorten attainable decay.

FL Studio 2026 was present with the owner's trap.flp open. A separate empty instance and Plugin Manager
were opened; the owner project was not edited. Both C: and E: protected Program Files VST3 destinations
denied writes; the alternate-folder dialog did not accept UI automation. The test instances were closed.
The final VST3 has therefore NOT been confirmed loaded/heard in FL Studio. This remains an explicit
manual check after installing/rescanning; automated validators and offline renders do not stand in for it.
The installer requires normal administrator installation approval. It was compiled; executing its installation
under administrator privileges was not available in this session.

## Binary and provenance

Validated VST3 SHA256: `{digest}`.
Package checksums: `PACKAGE_MANIFEST.txt` and `dist/*.sha256`.
M0 baseline: 02446a7; fixture/design commit: dc6f952; standalone extraction: dbd2000;
V2 implementation and calibrated recipes: a418499. Later release commits carry the reports/tooling.
No HPEQ source, subtree, remote repository or owner FL project was changed. No push was performed.
'''
(ROOT/'docs/reports/v2/V2_REPORT.md').write_text(report,encoding='utf-8')
print(f'Report: {tests[1]} tests, 200/200 presets, max TP {maximum:.3f}, binary {digest}')
