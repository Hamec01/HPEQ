# Attack preservation and musical peak timing

2026-10-30. This completes the isolated attack-correction stage after 05a1445. It does not connect the new renderer to the VST3 or release 0.2.0.

## Change

The new path extracts detected leading attacks using complementary source weights, resamples those attacks at the selected pitch, and places their physical peaks on the musical delay timeline. The continuous body uses separately matched readers and cannot replay the extracted samples. Both channels share detection/event timing. The attack reader uses 96-tap Kaiser interpolation; the body retains 64-tap Blackman interpolation. Prepared event history holds 512 events and each voice has eight prepared attack readers. Adjacent events are sealed before a new attack, including the tested weaker hit after 8 ms.

Pitched effective delay is max(requested delay,80 ms); unity forward remains exactly the requested delay. Peak-centred resampling can expand a waveform's leading part before its original start; FIR support also precedes the peak. The UI must display the additional capture wait instead of hiding it in dry/PDC. Body windows are 80 ms when causal availability permits, otherwise 40 ms. No unpitched dry attack is mixed into a pitched reflection.

## Verification

| Measurement | Original prototype / intermediate | Final result |
|---|---|---|
| Short-attack matrix, same corrected v3 method | 48/2352 passed | 2352/2352 passed |
| Peak timing vs independent rate-aware reference | Up to thousands of samples wrong | 0 samples worst error |
| Energy error vs reference | Missing/duplicated fragments | <=0.000007 dB |
| Normalised waveform error | Up to about 2 | <=0.0000013 |
| Added rate/delay/stereo/rapid-pair subset | Candidate 2: 65/75, including disclosed test errors | 75/75 passed |
| Original pitch cases | 204/204, worst 0.2383 cents | 204/204, worst 0.2376 cents |
| Original timing/Reverse/block records | 44/44 | 44/44 |
| Long wrap/automation records | Four old records | Six passed, including dense event-history wrap and a 4 s reflection |
| Guarded C++ processing allocations | 0 | 0 |
| Legacy plugin expectations | 590/590 | 590/590 |

The final executable reproduces all 2685 previously analysed main/edge/original/wrap render hashes exactly; its two additional dense wrap renders match each other at blocks 64/4096. Total final records: 2687. The precision above establishes event/reader arithmetic against a defined independent resampler, not a universal perceptual-quality claim. Short finite attacks legitimately change duration/bandwidth with resampling; full captured-fragment duration acceptance remains pending production integration.

Method changes are fully retained in `docs/ATTACK_METHODS.md`: v1 mistakenly anchored to an unobservable clip boundary; v2 uses the independently known physical peak and 300 ms post-capture; v3 extends pre-capture to 32 ms and corrects the ghost boundary for independently scheduled double hits. The immutable original DSP is rebuilt against the same harness. Original reports before_v1.json/before_v2.json, candidate failures after1_v2.json/edges_analysis.json, and the final common-method before3.json/after3.json remain available. No +/-1 sample, +/-1 dB, 0.15 L2 or -60 dB ghost threshold was relaxed.

## Cost and limitations

Release x64, actual i5-10300H, 48 kHz/256; normal desktop and owner's FL Studio remain open. Wall-clock figures are primary. GetThreadTimes is an added diagnostic with coarse per-block accounting. No owner process was stopped. A process-time sample showed Chrome using about 22% of all logical CPU capacity; it does not prove the cause of any specific pause.

| Normal-priority workload | First attack-capable run | Final mean | Final p99 | Final worst block |
|---|---:|---:|---:|---:|
| Eight tone-driven pitch voices | 21.46% | 18.78% | 34.10% | 72.80% |
| Two tone-driven renderers | 44.13% | 38.02% | 59.69% | 85.14% |
| Eight voices, 6 ms hit every 8 ms | 41.47% (failed 25%) | 22.32% | 35.90% | 39.36% |
| Two dense-input renderers | 84.15% (failed 50%) | 46.14% | 97.69% | 161.33% |

An intermediate empty-mask optimisation reproduced output but its dual mean failed at 51.29%, with an 840% worst block. Normal-priority diagnostic repeated 20.73%/44.72%; AboveNormal, explicitly applied only to that short-lived child benchmark, measured 20.54%/42.19%. An intermediate dense optimisation still failed 26.43%/51.15%. All files remain in this directory. The final improvement is substantive: exact prepared run-length metadata skips wholly extracted body ranges, limits attack convolution to the actual fragment, and avoids multiplying by known unit masks. Every arithmetic optimisation is verified against existing raw hashes.

Final mean component limits 25%/50% are met for both workloads. Wall-clock outliers remain visible, including a dense dual block above one block duration. This is not full-plugin E10 acceptance: bands, feedback, space, transitions, host scheduling and real FL Studio playback remain to be measured. Do not use a successful average to claim every audio deadline is safe.

At 48 kHz, prepared per-instance history/window/run metadata occupies 10,641,168 bytes; immutable shared coefficient tables occupy 16,087,680 bytes. This costs more memory than the first prototype. New state/UI/host integration and the final whole-plugin allocation audit are still pending.

Steady-tone worst fitted residual is -30.92 dBc (previous -30.88), strongest separated spur -42.38 dBc (previous -43.12). All pitch targets pass, but the small spectral tradeoff is disclosed rather than described as uniformly better sound. Dense/complex material, detector behaviour on sustained audio, low-level material, Reverse+pitch, feedback accumulation and complete fragment-duration semantics still need production tests and listening.

## Artifacts and next work

Fresh known-note and owner-material examples: `E:/HamloEcho/out/attacks/VerifiedAudioExamples/` (27 dry/wet/insert WAVs). Before examples remain under out/prototypes/AudioExamples. Metadata and WAV identities accompany this report. No human listening or installed-plugin audition is claimed.

Preserve out/m0, out/prototypes and out/attacks, including failed/intermediate runs and executables. Build/reproduce:

```powershell
cmake --build build --config Release --target HamloEcho_PrototypeLab HamloEcho_AttackBaselineLab -- /m:2 /v:minimal
build/Release/HamloEcho_AttackBaselineLab.exe attacks out/attacks/new_before
build/Release/HamloEcho_PrototypeLab.exe attacks out/attacks/new_after
python -I Tools/analyse_attacks.py out/attacks/new_before docs/reports/attacks/new_before.json
python -I Tools/analyse_attacks.py out/attacks/new_after docs/reports/attacks/new_after.json
build/Release/HamloEcho_PrototypeLab.exe attackedges out/attacks/new_edges
build/Release/HamloEcho_PrototypeLab.exe stress out/attacks/new_wrap
build/Release/HamloEcho_PrototypeLab.exe bench docs/reports/attacks/new_dense.tsv 0 dense
```

Next: integrate the verified score/attack path into a separate new processing mode with old-state migration, then implement feedback/bands/space/glitch and pitched Reverse without losing timing or the measured voice budget. Preserve Legacy IDs/order/defaults/programs and reset additions when loading an old state over an edited instance. Full UI/library/release remains pending; no installer, push or subtree operation is included in this stage.
