# R15 acoustic reference measurement (2026-11-19)

Owner request: measure reference fills, **acoustic only**, no electronic ones. Measurement, not a change to the
fill engine. Tool: `Tests/DrumFillReferenceLab.cpp` (target `HamloRevolution_DrumFillReferenceLab`, uses the same R15
analyser) + `Tools/r15_reference.py`. Raw hits and manifest: `out/r15/reference` (local). Summary JSON:
`acoustic_reference_summary.json`. Reference audio is not in Git.

## Selection (by explicit labels only)

| Group | Source (D:\Drums\DRUMFILL) | Files listed / measured |
|---|---|---|
| A acoustic | MusicRadar `IF_AcouFill*` (tempo from folder) + Wave Alchemy `acoustic fills` | 153 / 146 |
| B live boom bap | Sonic Mechanics Boom Bap Breaks drum fills (live breaks per pack, not per file name) | 24 / 24 |
| C snare rolls | MusicRadar `IF_LudSnrRoll` (acoustic Ludwig snare) | 36 / 36 |
| R15 | last bar of R15 4-beat lab renders (12 loops x 3 seeds) | 36 / 36 |
| R14 | the same for R14 AUTO COMPOSE | 36 / 36 |

Excluded: Vandalism, Cymatics trap 808, Chop Shop house 124, Wave Alchemy electronic / experimental (electronic);
MusicRadar AcouGlitchChop (processed chops), KikSnrFill / SnareRoll (origin not labelled); Axel's Drum Fills (mixed,
not verifiable); vinyl fills (no tempo). 7 group-A files were shorter than 3.5 beats or silent and were skipped.
Measured unit: the last 4 beats of each file (the fill bar).

**Correction (same day, R15c).** The first version of this report counted a hit on the last 16th as a rest when it
came 1..2 ms early (thresholds `tick >= 90 / 89 / 84` with no tolerance; slice pre-roll and human rushing put real
last-16th hits at 89.4..89.9). Position classes now split half-way between grid points (87 / 87 / 81). All groups
were re-measured with the corrected tool; only the two "last 16th" rows changed. First (wrong) values: rest
58 / 71 / 47 / 39 / 8 %, accent 23 / 4 / 56 / 19 / 47 %. Conclusion 3 below was wrong and is rewritten.

## Results

| Measure | A acoustic | B live BB | C snare rolls | **R15** | R14 |
|---|---|---|---|---|---|
| Hits per bar (median) | 14 | 15 | 16 | 12 | 21 |
| Hits per beat 1-4 (mean) | 4.0 3.7 3.7 2.7 | 3.8 3.3 4.4 3.4 | 3.8 4.0 4.3 3.4 | 3.1 1.8 3.2 3.6 | 4.2 4.8 5.4 6.6 |
| Share on 8th / 16th / 32nd / triplet / off | .32/.22/.11/.11/.24 | .42/.40/.04/.03/.11 | .52/.38/.00/.11/.00 | **.67/.14/.02/.06/.11** | .06/.14/.14/.22/.44 |
| Syncopation (median, same formula as the R15 score) | 0.51 | 0.59 | 0.58 | **0.35** | 0.57 |
| Tension late vs early energy (median) | -0.09 | -0.04 | +0.20 | **+0.19** | +0.28 |
| Nothing in the last 16th (rest before the downbeat) | 41 % | 12 % | 8 % | **8 %** | 0 % |
| Accent in the last 16th | 26 % | 4 % | 61 % | 19 % | 47 % |
| Microtiming, median distance to nearest grid | 9.2 ms | 11.1 ms | 2.7 ms | **3.3 ms** | 13.7 ms |
| Longest run of even strokes (median) | 5 | 7 | 15 | **3** | 9 |
| Dynamics beat 4 / beat 1 | 0.72 | 0.59 | 1.34 | **1.15** | 2.05 |
| Tom-like hits (heuristic) | 40 % | 11 % | n/a | 0 % | 1 % |

## What it says about R15 (interpretation, with doubts)

1. **Too square.** R15 puts 67 % of hits on 8ths; acoustic fills spread over 16ths, 32nds and triplets, and
   syncopation is 0.51-0.59 against R15's 0.35. The R15 genre syncopation targets (0.30-0.45) are lower than any
   measured acoustic group: they were guesses and look too low.
2. **Too fragmented.** Acoustic players run 5-7 even strokes in a row; R15's median is 3. Cells change idea every beat.
3. **Landing (corrected).** Live boom bap players and snare rolls almost always play the last 16th (empty in
   12 % / 8 %); acoustic pack fills rest there in 41 %. R15 rested in 8 %: close to live players, but with no
   "strong hit, then rest" option. (First version said 58-71 % rest: measurement error, see the correction above.)
4. **No crescendo by default.** Acoustic full-bar fills do not get louder towards the end (tension -0.09, beat 4 at
   0.72 of beat 1); only snare rolls do. R15's score rewards a build (+0.19, 1.15). Doubt: these packs are whole-bar
   fills that start on a crash or a strong hit, while R15 keeps the groove first, so part of this is the format.
5. **Timing is close to quantized** (3.3 ms vs 9-11 ms). R14's 13.7 ms is not human feel but scattered positions
   (44 % off-grid). Real players' deviation is systematic (rushed rolls, laid-back flams), not random.
6. **Toms carry acoustic fills** (~40 % tom-like hits). The analyser has no tom role: toms are counted as snare /
   kick here, so role shares for group A are not reliable. A loop without toms cannot give a tom fill; a user mark
   for TOM would be the honest way to add it.

## Limits of this measurement
- The analyser was tuned on electronic one-shot loops; acoustic toms, cymbal washes and ghost notes are where it is
  weakest. 24 % "off-grid" in group A partly comes from ringing toms / cymbals and flams, not only timing.
- "Tom-like" is a band / decay heuristic, not a classifier. Fill bar = the last 4 beats of each file.
- These are structural statistics of 206 human fills, not a judgement of which fills sound good.

## After R15c (same corrected tool, R15 group re-rendered)
See `docs/DRUM_FILL_R15.md`, section R15c. Summary JSON: `r15c_reference_summary.json`
(`acoustic_reference_summary.json` is the corrected "before" for all groups).
