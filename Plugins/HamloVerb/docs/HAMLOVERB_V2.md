# HamloVerb 0.2.0 design and acceptance contract

Approved by the owner on 2026-10-29. This document precedes the M0 acoustic baseline and all sound changes.

## Product

Standalone Windows x64 VST3, developed only in this repository. No HPEQ source changes, subtree integration or push. Keep BBLB / HmVb / com.hamloprod.hamloverb, the HamloVerb state tag and every legacy parameter ID/range/order. English product text. No IR import.

Legacy retains the M0 additive wet law, exact float operation order, startup/bypass ramps, old programs and Classic limits (250 ms / 20 s). The known block-dependent legacy space-change boundary is retained and disclosed. New engine IDs are additive and default to Legacy; missing fields explicitly reset to defaults on state load.

## Sound and workflow

Six new structural characters: Clean (spatial FDN), Warm (cross-coupled diffusion and gentle saturation), Velvet (nested diffusion), Crisp (short dense plate), Deep (sparse early field and developed late field), Bloom (cascaded build-up). New predelay 0..500 ms; base decay 0.1..30 s. Six-band Decay EQ and six-band Post EQ, initially disabled; effective spectral decay bounded 0.1..120 s. Independent Early/Tail levels.

Temporal shape: Off / Gated / Reverse Swell / Reverse Audio. Gate uses input detection, threshold, hold and release. Reverse Audio reverses overlapping wet fragments of 50..2000 ms, with accumulation delay explicitly described. Free/tempo-synced shape time. Independent Shimmer (-12..24 semitones, bounded feedback). Freeze closes new input and pitch feedback, smoothly removes loop loss/nonlinearity and stops modulation; reports infinite tail. Output controls remain available.

Insert is linear dry/wet; Send is wet only and remembers Insert MIX. Mix Lock is a saved workflow preference and protects MIX on preset loads, not project-state restoration. Routing is also preserved on preset loads. New engine changes use at most two preallocated paths: outgoing input closes, freeze releases, old tail continues up to 120 s. A repeated change fades the oldest tail over 50 ms and applies the latest pending request. Dry and host latency remain zero; wet accumulation is part of the effect.

Keep Black Steel, preset CRT at (56,20,336,116) on the 1180-wide stage, and unchanged shared PresetScreen files. Relocate COLOR into Legacy. Material buttons filter only; LOAD START selects a real preset. Advanced exposes EQ, diffusion, modulation and effects. Context help and a truthful modified indicator are required.

## Presets

Append 200 recipes after Default and the seven M0 presets (208 host programs). Four genres: Boom Bap, Trap, Techno, DnB, each with Vocal 10, Drums 12, Samples / Keys 10, Synth 8, Atmosphere / FX 10. This owner-approved distribution supersedes the generic AGENTS.md quota. Stable IDs, material, tempo, note and depth are mandatory. MIX/Output differences alone do not establish uniqueness.

Calibrate on owner audio after DSP stabilizes. Complete-render wet/dry energy ratio bands: Gentle [-24,-16] dB, Medium [-16,-8] dB, Heavy [-8,+6] dB. Include tails. Verify actual Insert and Send separately, finite output, state round trip and demonstration true peak <= -1 dBTP (8x reconstruction). Resume caches must include DSP/source/material hashes; stale results cannot count.

## Fixed laboratory goals

| ID | Goal and method |
|---|---|
| V1 | Legacy equals M0 bit-for-bit at identical block/event schedules: Default + 7 programs, Classic boundaries, automation, mono/stereo, 44.1/48/96 kHz. Preserve state fixtures. |
| V2 | New engines bit-identical at blocks 1/64/127/512/4096 after prepare(256), including identically timed events; no processing allocations. |
| V3 | Clean neutral RT60 error <= max(10%, 50 ms). Schroeder backward-energy T30 fit (-5..-35 dB), T20 also reported. Test 0.1/0.25/1/2/10/30 s; report fit quality, bandwise decay and artistic modes separately. |
| V4 | Predelay changes shift wet onset by the selected delay within one sample, including 500 ms. |
| V5 | Clean/Crisp/Velvet normalized echo density reaches 0.9 within 150 ms of late-field onset at decay 2 s / size 1. Abel-Huang normalization, 25 ms Hann window. |
| V6 | Freeze settled energy drift <= 1 dB over 60 s; five-minute stability with finite output and no runaway. Report internal energy and audible RMS independently. |
| V7 | Width zero gives identical output channels; report stereo correlation and mono-fold energy on impulses and owner clips. |
| V8 | Gate timing and release residual, Swell envelope and Reverse Audio sample ordering are independently verified. |
| V9 | Shimmer pitch and bounded maximum-feedback operation verified; freeze disables new pitch injection. |
| V10 | Intel i5-10300H, Release, 48 kHz / 256: mean processing <=25% block duration; two-path transition <=50%. Warm-up excluded; record maximum and machine conditions. |
| V11 | State migration resets new fields even over a modified V2 instance; preserve old program numbers; routing/Mix Lock behavior and filter-only UI actions verified. |
| V12 | 200/200 new recipes pass depth/peak/finite/uniqueness/state checks with exact final DSP hash. |
| V13 | pluginval 5..10 and Steinberg validator pass on packaged binary. |
| V14 | Visually inspect Main/Advanced/Legacy screenshots at normal/minimum sizes, contextual help and CRT placement. |
| V15 | Installer/ZIP/bundle identity and SHA256 verified; FL Studio smoke test and A/B audio supplied or explicitly marked unverified if unavailable. |

The same lab implementation runs before/after; nonexistent features are n/a. A failed goal remains failed. Every later method/threshold correction must be explained here, in code and in the final report, retaining original results. Listening evaluation complements measurements and is never replaced by a claim of objective superiority.

## References and release

- Valhalla DSP: https://valhalladsp.com/2022/10/21/the-more-you-know-valhalla-modes-algorithms/ — structural algorithm variety, not copied implementations.
- FabFilter: https://www.fabfilter.com/help/pro-r — separate decay/output EQ and workflow reference.
- Abel and Huang, AES 6985: https://aes.org/publications/elibrary/elibrary-page/?id=13819 — normalized echo density.

Deliver version 0.2.0, README/help, before/after and preset reports, known limitations, local stage commits, installer, portable ZIP and SHA256 manifests. Update local handover to remove obsolete Mag work. Do not integrate or push HPEQ.

## Execution disclosures (2026-10-30)

- The first V2 unit run passed 1130 expectations and failed one CRT-position check because the old 760-high panel was still hosted in the new 940-high editor. The standalone panel resolved it; the test bounds were not changed.
- Ordinary Freeze passed its initial five-minute tests. Source review then identified live Size/Diffusion changes altering a frozen tank; the geometry/diffusion capture was fixed and a bit-exact regression added. Bypass now closes new input, and Freeze bypasses temporal shaping so a gated preset can hold audibly.
- Two provisional calibration runs were interrupted during DSP review. Their source-keyed caches were invalidated; only the complete final-DSP calibration/check results count.
- The full after run included concurrent compilation/calibration and failed all seven wall-clock CPU rows (38 other checks passed). These failures remain in after.md. The identical CPU function was rerun without concurrent jobs: 2.91..4.91% for a single character, 8.89% for a handover. Thresholds/methods were unchanged.
- An additional spectral IR-export mode and independent octave-band analysis were appended after the first runs. They add diagnostics only; existing acceptance functions and thresholds were not modified. See spectral_diagnostics.md.
- Native preset comparison uses normalized parameter values to avoid a false modified marker from frequency round-trip rounding. No audio metric was relaxed.
- Gate Release has an effective 5 ms floor. Reverse and Shimmer are causal delay-based effects. No measured alias-rejection or subjective superiority claim is made for them.
- FL Studio is installed and a separate empty instance was opened, but loading the final VST3 could not be confirmed: protected installation paths denied writes and the folder dialog did not accept automation. The owner's trap.flp instance was left untouched; test processes were closed. This manual host check remains unverified, not a pass.
