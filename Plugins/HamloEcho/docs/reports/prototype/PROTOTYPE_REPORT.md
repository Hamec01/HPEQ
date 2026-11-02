# Note/reflection and Reverse prototype

2026-10-30. Research stage following Legacy extraction 8c9cdb0. The shipping VST3 is unchanged, version 0.1.0. No new UI or host parameters were added, and this is not a 0.2.0 release.

## What exists

`ReflectionPrototype` owns one shared stereo input-history ring and eight independently pitched readers. Source/target MIDI numbers define fixed intervals; each voice reads the original signal rather than the previous voice's output. Eight configurable times/notes/gains/pans, free/synced timing, stereo-linked splice alignment, 64-tap band-limited interpolation and independent unpitched Reverse are exercised by a separate C++ executable. No external implementation was copied. The conceptual references and methods were recorded before implementation in `docs/PROTOTYPE_METHODS.md`.

Unity pitch forward is an exact integer delay. Other forward pitches use overlapping 80 ms windows with up to 20 ms of alignment search. Reverse uses a fixed captured range and exact backwards playback with 50% Hann overlap. Reverse+pitch, feedback pitch accumulation, the final score/state interface and the complete effects chain remain unimplemented.

## Measurements

| Check | First candidate | Refined candidate |
|---|---:|---:|
| Eligible pitch scenarios | 204/204 within 5 cents | 204/204 within 5 cents |
| Measured pitch channels | 224 | 224 |
| Worst absolute error | 1.8274 cents | 0.2383 cents |
| Worst fitted-sine residual | -26.9704 dBc | -30.8835 dBc |
| Strongest separated spur, worst case | -40.7494 dBc | -43.1156 dBc |
| Exact forward impulse timing | 12/12 | 12/12 |
| Independent Reverse reference | 12/12, max error 0 | 12/12, max error 0 |
| Host-block records | 20/20 equal to their group reference | 20/20 equal to their group reference |
| C++ allocations in guarded processing/updates | 0 | 0 |
| Finite renders | 254/254 | 254/254 |

The pitch matrix includes every eligible integer shift -24..+24 on 110/220/440/880 Hz at 48 kHz (184 cases), plus 20 stereo/rate cases at 44.1/96 kHz. Frequency is measured using a global FFT peak, not merely searching around the expected answer. Each channel is independently fitted afterward. The worst residual is the differing-channel 96 kHz/+7 case, not an undisclosed failure of the 5-cent target. Residual/spur levels are diagnostics without a newly invented acceptance threshold; passing pitch does not establish transparent sound.

The 20 block records comprise four mono/stereo pitch/Reverse group references and 16 cross-block comparisons at 1/64/127/512/4096 with exact note/tempo event positions. Supplemental 12 s/96 kHz stereo stress produces four more renders (two cross-block comparisons) across history-ring wrap, including later automation; all are finite, bit-identical within their group and allocation-free. This is isolated-renderer evidence, not a whole-plugin allocation/state/mode-transition acceptance claim.

The final executable reproduces all 254 analysed refined-candidate render hashes exactly. No spectral re-analysis was substituted for different output. Details: `run1_analysis.json`, `run2_analysis.json`, `wrap_stress_analysis.json`, `final_render_verification.json` and the SHA256 manifests in this directory. The original 160-case Legacy tool source was not changed.

## CPU and memory

Measured on the actual Intel Core i5-10300H, 48 kHz/256, Release x64; no build or spectral-analysis job ran concurrently. The owner's existing FL Studio session and normal desktop/audio services remained open. They were not stopped or changed. The first second is a declared warmup; the following approximately five seconds are timed. Keep all three CPU runs; small run-to-run variation is visible.

| Workload | Run 1 mean | Run 2 mean | Final executable mean | Final p99 | Final worst block |
|---|---:|---:|---:|---:|---:|
| Eight pitched stereo voices | 17.8730% | 15.2952% | 15.3109% | 24.1369% | 25.5507% |
| Two eight-voice renderers | 34.6243% | 30.7601% | 31.0961% | 46.8919% | 51.3057% |
| Eight unpitched Reverse voices | 4.3469% | 3.9123% | 3.9236% | 6.0057% | 6.7538% |
| Two Reverse renderers | 7.7108% | 7.7331% | 7.9080% | 11.9419% | 38.4075% |

Refinement preserves the method and targets: fractional splice alignment reduces phase error, contiguous mirrored-history correlation removes inner-loop modulo operations, and offset voice windows spread searches over sample time rather than stacking them at one block. First-candidate pitched worst blocks were 54.0957%/79.9538% for one/two renderers; these results are retained, not discarded.

Each instance prepares 5,023,872 bytes for mirrored stereo history and windows at 48 kHz. Immutable shared interpolation coefficients occupy 6,435,072 bytes. History capacity is 6.5 s, covering a 4 s delay plus 2 s Reverse range with margin. All tested processing and parameter updates allocate zero C++ objects after preparation. Interposition covers new/new[] and aligned allocation; it is not a CRT/OS heap trace or a whole-plugin audit.

The component is below the provisional 25%/50% mean ceilings. E10 for the complete plugin is still pending: spectral bands, space, feedback, transitions and GUI communication have not been included. Do not multiply this pitch stage independently across all three spectral bands; design shared resynthesis/routing within a measured total voice budget.

## Audible limits that still need work

The impulse diagnostic exposes a real limitation. Relative to a 0.5 impulse at the input, measured output energy is -0.90/-13.40/-4.71/-5.03/-11.19 dB at -24/-12/+7/+12/+24 semitones. At +24, samples above -140 dBFS extend from sample 7269 through 17355 (48 kHz, requested delay 6000 samples). Granular weighting changes short attacks; it is not an attack-preserving final algorithm. Resampling itself also changes pulse bandwidth/duration, so the next test must compare against a rate-aware reference and many onset phases, rather than demand raw equal energy at every ratio.

Pitch accumulation is a bound, not constant host latency. The oldest source age beyond the requested delay is at most `34 + ceil(abs(ratio-1)*grain) + search` samples. At 48 kHz this is up to 260.71 ms for +24; first audible transient positions can be earlier and depend on the window/attack phase. The score UI must disclose wet accumulation and the main engine still needs a deliberate policy for pitched rhythmic onset. Dry/host PDC remains zero; no hidden dry delay is proposed.

A 10 kHz input at +24 (ideal 40 kHz, unavailable at 48 kHz) leaves steady output at -83.22 dB relative to input RMS. That single out-of-band diagnostic is not a full aliasing or passband guarantee. Full-band audio, mixed transients, near-Nyquist cases, formant behaviour and cumulative feedback still require further design/listening/tests.

## Listening files and preserved state

`E:/HamloEcho/out/prototypes/AudioExamples/` contains 27 float WAVs: dry/wet/insert sets for the known C5 -> C4/G4/C5/C3 score and eight real-material pitch/Reverse examples (snare, keys, synth, vocal). The known source is MIDI 72 / 523.251 Hz; target MIDI numbers are 60/67/72/48. Requested delays are 500/1500/2500/3500 ms, before pitch accumulation. Owner-source roots are unspecified and those files demonstrate relative textures only.

Files are prepared at 48 kHz, with shared gain for each dry/wet/insert set and an 8x resampled peak ceiling of -1 dBTP. Actual maxima are -12.04 dBTP for the known-note example and at most -5.20 dBTP for owner examples; no final normalisation was needed. Metadata/input identities: `examples.json`; WAV hashes: `AUDIO_SHA256.txt`. The source uses a four-second energy-selected excerpt and two seconds of tail. These are listening artifacts, not the future 200 calibrated presets.

A separate 200 ms listening-file diagnostic measures the four isolated example notes at 261.625/391.986/523.251/130.813 Hz, corresponding to C4/G4/C5/C3. See `note_example_diagnostic.json`. Its short window is explicitly not the 500 ms E04 acceptance measurement and its Hann sidelobes must not be interpreted as independently proven DSP spurs.

No human listening or FL Studio test of this prototype has been performed by the agent. No subjective quality claim or installed-VST3 claim follows from generating the files.

Original M0/Legacy files remain in `out/m0`. Shipping VST3 SHA256 is still `128d1e76f93dec5b7f61a2ddb5c38818af45abc7a1b5043408ccd8428beb9e3d`; current original/compatibility tests remain 590/590. Only a standalone prototype target was added, so validators were not needlessly rerun on an unchanged DLL. No other repository was modified and no subtree/push occurred.

## Reproduction and next stage

```powershell
cmake --build build --config Release --target HamloEcho_PrototypeLab -- /m:2 /v:minimal
build/Release/HamloEcho_PrototypeLab.exe render out/prototypes/new_run
python -I Tools/analyse_prototype.py out/prototypes/new_run docs/reports/prototype/new_analysis.json
build/Release/HamloEcho_PrototypeLab.exe stress out/prototypes/new_wrap
python -I Tools/analyse_prototype.py out/prototypes/new_wrap docs/reports/prototype/new_wrap_analysis.json
build/Release/HamloEcho_PrototypeLab.exe bench docs/reports/prototype/new_cpu.tsv 0
```

Render directories refuse overwriting; preserve run1/run2/final and their archived executables under ignored out/prototypes. Begin the next stage with short-attack and onset-phase tests plus an explicit pitched-onset policy. Improve or add a transient-capable path before integrating this candidate as the only pitch engine for drums. Then implement the complete new DSP, state migration and UI, followed by 200 recipes and full release acceptance.
