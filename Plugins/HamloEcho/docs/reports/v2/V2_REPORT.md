# HamloEcho 0.2.0 release report

2026-10-30. Local Windows x64 VST3 release for the owner's FL Studio audition. All approved Echo features and the 200-recipe library are implemented. Owner listening/FL acceptance is still pending; the installer is prepared, not executed by the agent.

Production DSP source SHA256: `c5dd2066c79ce00c6887862d740a59a4a94be0b3b9aff09a9760ae22770fa969`. Validated VST3 binary SHA256: `62127774c8d48e6e6bfbb04417521fd15a8e8db2daa0e93f7b3cc8ff914ec519`. Original VST3 identity, state root and 28 effect controls are preserved. The wrapper Program-selector compatibility exception is explicitly disclosed below.

## Acceptance

| Goal | Result and scope |
|---|---|
| E01 Legacy |160/160 immutable M0 raw renders identical; old states reset added controls on fresh and already edited instances. Original effect-parameter contract and numbered programs 0..7 retained. Wrapper Program automation is an exception. |
| E02 Determinism / memory |1198 unit expectations pass. New-mode blocks 1/64/127/512/4096, sample-positioned note/space/tempo/bypass/mode events, prepared audio allocation 0, reused captured paths match fresh epochs. |
| E03 Time |15 free/sync cases at 44.1/48/96 kHz: exact to one sample within effective delay limits. |
| E04 Pitch |Original component matrix retained via 2687 identical archived hashes; integrated fixed/cumulative tone checks pass, worst 0.0340 cents in their measured range. Source is declared manually; this is not pitch tracking or chord correction. |
| E05 Captures / rhythm |Analytic Reverse/Stutter/Tape Stop/texture references, 100 ms Reverse at 6 pitch intervals, lifecycle reuse and non-divisible 2..8 subdivision checks pass. |
| E06 Bands |Neutral reconstruction <=1e-6; independent timing measured. Ordered complementary one-pole bands and shared return-norm safety prevent overlap reinforcement. |
| E07 Freeze |60 s settled energy drift 0.000000 dB; 5 min finite stress; controlled release and genuine host infinity. |
| E08 Stereo |Width 0 wet channels bit-identical. All 200 recipes measured: minimum correlation -0.4369; worst mono energy change -4.844 dB. Stereo character is reported, not universally certified mono-safe. |
| E09 Routing / state |Dry/PDC 0 samples; Mix 0 exact dry; Send retains Insert Mix. Independent Insert/Send null maximum 7.45e-08; all 200 complete states round-trip. |
| E10 CPU |Controlled tone/dense means meet 25%/50%; an initial dense run failed and wall-time outliers remain disclosed in the table below. No unconditional deadline guarantee. |
| E11 Library |200/200 compiled recipes pass; 208 programs total, 200 unique recipes excluding Mix/Output; 400 actual Insert/Send renders; depth, finite output, state, tail, peaks/demo scale checked. |
| E12 Release |pluginval 5..10 and Steinberg 537/537 pass. Main/all Advanced/Legacy/885 px images inspected; four A/B demonstration sets prepared. Owner FL Studio audition pending. Packaging evidence below. |

## Library and headroom

Four genres x 50: Vocal 10 / Drums 12 / Samples-Keys 10 / Synth 8 / Atmosphere-FX 10. Permanent IDs, descriptions, source assumptions, material and tempo are in the compiled bank and GENRE_PRESETS.md. Both actual routing depths must be in the stated Gentle/Medium/Heavy band. Mix 0.5 puts Insert and Send 6.0206 dB apart; calibration targets their shared feasible interval. No energy threshold was relaxed.

The first calibration had 28 over-range Heavy recipes (required gain up to +20.94 dB). We revised their authored tap balance/pre-space direct share, retained failed results, then re-rendered after the lifecycle/stutter corrections. Final 200 gain calibrations and 400 compiled-bank renders use this production identity. Maximum unscaled Insert/Send true-peak estimates: -3.693/1.478 dBTP. This is an echo, not a limiter. Prepared demos use one common dry/Insert/Send scale per recipe and stay below -1 dBTP (target margin -1.1 dBTP). The scale and unscaled peaks are stored for each recipe.

Owner material: 16 named WAVs, energetic 10 s excerpts (or shorter complete clips), 48 kHz, target -18 dBFS RMS with -3 dBFS sample-peak ceiling taking precedence. Full tails are included, and the final second is checked for truncation. Resume identities pin DSP, executable, recipe, material and tempo. Results from older DSP candidates were retained as historical evidence and not adopted as final calibration.

## CPU evidence

i5-10300H, 48 kHz/256 samples, Release. Eight distinct reflection notes, bands, space, captured texture and cumulative feedback pitch. The dual plugin case is an actual incoming path plus a retiring tail. Wall-clock time includes scheduling; FL64 was open (a spot process counter read 49 processor-percent). This does not identify a proven cause for any individual spike.

| Workload / run | Mean single | Mean transition | Worst single | Worst transition |
|---|---:|---:|---:|---:|
| Tones |24.362% |36.732% |42.064% |69.323% |
| Dense initial (FAILED single mean) |33.302% |38.874% |738.750% |100.823% |
| Dense repeat1 |24.676% |36.959% |41.775% |90.298% |
| Dense repeat2 |24.855% |36.421% |77.438% |61.056% |

The initial dense run exceeds the single-instance mean target; it is not discarded or relabelled a pass. Repeated controlled runs meet the mean targets with limited single-path headroom. Earlier pre-optimisation 40.23%/68.52% and reset-only dense dual 142.856% worst-block failures are also retained. Exact-zero history metadata reduces tail-transition cost without changing any of 2687 archived component outputs. No acceptance threshold was weakened.

## Fixed failures and method history

SCORE_METHODS.md records the initial sustained Plate loop, adversarial frequency-band reinforcement, 80 ms pitched Reverse grain-clock shift, inactive-history epoch reuse and truncated non-divisible Stutter subdivisions. Each was reproduced before correction and checked again afterwards. Fresh/reused bit identity, allocation and all original block comparisons retain their strict tests. The M0 verifier extension permits appended controls/programs and compares old XML values plus reset additions; original signals/event schedules/raw references are unchanged. Earlier unpinned reports are diagnostics, not final source identity evidence.

## Compatibility and practical limits

- The 28 original effect controls retain IDs, ranges, order, defaults and normalisation. JUCE adds a separate host Program selector: its range grows from 0..7 to 0..207. Existing normalised automation of that selector needs review/rebinding (old 1/7 now selects approximately 30). Direct numbered recall and saved effect states retain old programs. See release_020/SDK_PROGRAM_SELECTOR.md. This is an exception to blanket host-program-automation compatibility.
- SOURCE is manual, fixed intervals +/-24 st; out-of-range cards are visibly muted. Cumulative feedback can move farther, eventually reaching audio bandwidth limits.
- Pitched capture minimum 80 ms, additional Reverse/Glitch capture as displayed. Effective reflection delays are limited to 1..4000 ms and loop requests to 20..4000 ms; pitch capture can impose a longer minimum. Tempo/scale requests beyond the effective range are clipped. Dry/host latency is 0 samples.
- High combined band returns receive a safety trim. Freeze holds runtime audio; projects save its controls rather than captured memory. Structural changes release outgoing Freeze; outgoing paths are limited to 120 s.
- Actual unscaled wet output can exceed 0 dBFS; demonstration peak compliance uses the disclosed common scale. Windows wall-time outliers remain; owner must verify practical behaviour in FL Studio.

## Open acceptance items

E01 is not a blanket pass for every host automation lane: the expanded wrapper Program selector changes its normalised scale. E10 has a retained failed dense run and deadline outliers despite passing controlled repeats. E12 still requires the owner's FL Studio audition. These exceptions are not counted away by the successful core/library checks.

## Artifacts

- `HamloEcho_VST3_Setup_0.2.0.exe` — SHA256 `ccf2261b9684460dc359cfdc9c0236ef5c7a0a94f25d0a46d05117595f8cd028` (26363655 bytes).
- `HamloEcho-0.2.0-win64-vst3.zip` — SHA256 `1ffe202db6cb6791b43d96aa08cabd320a8cc2c3300bd8f13db512d4ff34fdf7` (7184183 bytes).
- `HamloEcho-0.2.0-audio-examples.zip` — SHA256 `b06702b6230a56dad72be70a91f00c19a9f3d845637dd47ca74f22bc3d5f3d41` (77560220 bytes).

ZIP core and documentation entries were decompressed in memory and hash-compared to staging. Installer inputs were pinned to the validated core inventory and checked unchanged after ISCC compilation; the installer was not run or extracted. Do not confuse the compiler-input audit with an installation/FL audition. Companion.sha256 files and dist/release-manifest.json are provided.

## Reproduction and files

Tools/run_integrated_checks.ps1 -Run stable_020f; Tools/calibrate_score.py calibrate/check; Tools/validate_release.ps1; Tools/package_release.ps1. Logs build/stable_020f-*, shipping_*. Machine-readable data: integration/stable_020f, presets/calibration.json, presets/verification.json and release_020_final. Preserve ignored out/m0, out/attacks, out/score and out/presets. No HPEQ/Verb edits, subtree or push are included.
