# R15 PHRASE FILL - plan, reviewed three times

2026-11-19, parent: Revolution 0.4.0 / R14 as imported in HPEQ `Plugins/HamloRevolution`.
Input: owner request + external research note (`HPREVOLUTION_DRUMFILL_RESEARCH_AND_CODEX_TASK_RU.md`,
reference `DrumFillPlanner.hpp`). That note audited the old HPEQ `Source/` Revolution, which has no fill at all;
the product the owner actually hears is this plug-in, which already has R12/R13/R14 fills.

## Why R14 AUTO COMPOSE still sounds weak (diagnosis from the code)

1. Its objective rewards being *unlike* the source (`+10` when input similarity <= 0.65). Novelty is not musicality.
2. Kick, snare and hat positions are drawn independently by weighted random sampling. There is no phrase:
   no pulse, no build, no cadence. Coherence is left to chance.
3. Slices come from fixed band-pass streams (kick < 260 Hz, snare 260..5000 Hz, hat > 5.5 kHz): the kick loses
   its click, the snare loses body and crack. They sound thin and filtered, not like the loop's own drums.
4. The whole edited window is replaced; the groove of the loop stops at the edit point. Swing and microtiming of
   the source are not used (events sit on a straight 24-tick grid).

## R15 design: analyse -> plan phrases -> score -> render original slices

New recipes, appended (old 0..29 and RISE stay bit-exact): 30 PHRASE AUTO (genre detected), 31 BOOM BAP,
32 TRAP, 33 DNB, 34 TECHNO. New FILL default: 30.

**Analysis** (`DrumPhraseFill.cpp`, message thread, offline):
- 4-band (sub / low-mid / mid / high) energy envelopes, 1 ms hop; band-normalised positive flux, adaptive
  median threshold, peak picking, 25 ms refractory; attack start refined on the waveform.
- Per onset *band novelty* (energy after minus before the attack). Role posterior P(kick/snare/hat) by
  softmax over band-fraction features (not "low band = kick"). Confidence p = max posterior.
- Isolation q: foreign-band novelty, pre-attack sustained energy (melody/bass masking), free time before the
  next onset. A slice is usable only when p*q passes a gate.
- Contamination-aware cleaning, only when needed: kick with a hat on top -> low-pass 3.5 kHz; hat over a
  kick/snare -> high-pass 5 kHz; snare over a kick -> high-pass 160 Hz. Clean hits stay full band.
- Grid: global phase from 8th-note onsets (+/-12 ms), swing = median offset of odd 16ths, per-role median
  microtiming. Genre guess from tempo, quarter-kick regularity, backbeat/half-time snare and hat density.

**Planner** - optimisation over a grammar, hard constraints first:
- Phrase = pulse (original audio kept exactly) + figure (one-beat cells of rising energy) + cadence cell.
  Turn point: 1 or 2 beats of pulse in a 4-beat fill, 0 or 1 in a 2-beat fill.
- Genre cell libraries (16th / 32nd roll / flam / ghost / triplet cells, energy 0..3). Trap and Techno carry
  the source hats under low-energy cells; Techno keeps every quarter kick unless STOP asks for a drop.
- Hard constraints (never traded against score): only roles with a usable source slice; strong original
  kick/snare anchors in the figure stay (same role, same time, own slice); no event outside the window;
  minimum same-role spacing; bypass when no clean kick or snare exists; neutral knobs = source.
- Score (heuristic baseline, to be fitted on owner A/B preferences later):
  `Q = 1.2 T + 0.8 V + 0.6 D - 1.5 X - 2.0 U - 1.2 R - 0.6 M + 0.5 F`
  T tension (late vs early accent energy, accented landing), V variation as an inverted U around 0.5 change
  (Witek et al. 2014: moderate syncopation is preferred), D syncopation inverted U per genre
  (Longuet-Higgins/Lee weights), X collisions/mush (near-simultaneous different roles, too-close same role),
  U mean slice uncertainty (1 - p q), R artifact risk (truncated tails, low-band stacking), M monotony
  (same cell repeated), F density fit to CHOP. 48 candidates per seed; NEW VARIATION = next seed.
- Knobs on FILL keep their names: CHOP = how much of the window changes / energy, ROLL = roll density,
  REVERSE = reverse pickup into the bar line, STOP = drop pause before the downbeat.

**Renderer**: original audio kept up to the turn, 10 ms crossfade, then source slices (full-band or cleaned)
with same-role choke, velocity relative to the role's own typical peak, swing + role microtiming applied,
alternating hands in rolls, optional room on the cadence (FX), 8x-checked ceiling only on the edited tail.

## Review pass 1 - does it actually solve the complaints?
- Thin sound: fixed by full-band slices; filtering only to remove a measured contaminating band.
- Chaos: fixed by phrase structure (pulse -> build -> cadence) and coherent cells instead of per-role sampling.
- Lost groove: pulse kept as original audio; swing/microtiming inherited; anchors locked.
- Risk: the role classifier is heuristic. Mitigation: confidence gates + a lab with real one-shots (Metro kit)
  assembled into genre loops with ground truth, reporting classification accuracy, not just "tests pass".

## Review pass 2 - what could break?
- Capture not on the bar line -> wrong grid. Mitigation: phase estimate; low timing confidence -> conservative.
- Full mix with music: band novelty vs sustained energy gives "drumness"; low -> conservative (snare/hat
  cells, last beat only) or bypass with a reason shown on the page. Never pretend.
- Kick under a hat everywhere (common): low-pass cleaned kick instead of no kick.
- Legacy: recipe < 30 code paths untouched; old exact tests remain; only default recipe changes for new
  FILL sessions (old state with recipe 21 still renders 21).
- Peaks: tail-only gain with true-peak estimate.

## Review pass 3 - doubts kept honest
- The score weights are a hypothesis; tests prove legality/determinism/anchors, not taste.
- No tom: no tom role is invented (absent in the source -> no tom cascade).
- 4/4 only, like the rest of FILL; no tempo detection (host BPM).
- Owner FL listening remains the real acceptance.

## Results (2026-11-19, Release build, this machine)

Native tests: **6259 passed / 0 failed** (5336 pre-existing incl. old exact renders + R15 `DrumPhraseTests`).
Two old expectations changed and are disclosed: the FILL default recipe is now 30 (PHRASE AUTO) instead of 21,
in `TransitionRecipeTests` and `DrumComposeTests` (the latter now selects 21 explicitly so R14 stays tested).

`HamloRevolution_DrumPhraseLab` (new, EXCLUDE_FROM_ALL): 12 loops = 4 genre patterns x 3 kits assembled from real
one-shots (`E:\Metro Boomin - #MetroWay Sound Kit [Nexus XP]`, not in Git), 2 bars, 4 ms random timing, swing in
Boom Bap. Ground truth known. 108 R15 and 108 R14 renders (1/2/4 beats x seeds 1..3), same metrics for both.

| Measure | Result |
|---|---|
| Onset recall / precision | 0.885 / 0.866 |
| Attack error vs true sound start | median 0.25 ms, p90 1.46 ms (slice pre-roll is 1.5 ms) |
| Role accuracy | 0.961 (kick 53/53, snare 36/36, hat 110/118; 8 mid-heavy hats read as snare) |
| Genre guess | 12/12 |
| Band-tone cosine, fill window vs source (median / worst) | R14 0.950 / 0.191 -> **R15 0.994** / 0.274 |
| abs level change of the fill window, dB (median / worst) | R14 2.58 / 22.6 -> **R15 1.48** / 11.4 |
| Onset deviation from 32nd / triplet grid, ticks (median) | R14 0.49 -> **R15 0.16** |

Worst R15 tone/level cases are 1-beat fills where the source's last beat was only hats (a snare is added: louder)
or a snare double-up replaced a kick (no sub: lower band energy). These are musical choices, not defects; they are
listed here because the numbers would otherwise look better than the per-case truth.

### Changes made after a measured run (disclosed, rule 4)
1. Classifier: first run used band fractions only (role accuracy 0.77; a bodiless mid-heavy hat read as snare).
   Added a body term (120..600 Hz present at all) and a decay term. Then 0.83, later 0.96.
2. First run: identical fills for seeds 1..3 (the optimiser always picked the same optimum) and DROP / REVERSE in
   most plans (the score cherry-picked them). Now the seed picks among distinct candidates within 0.3 of the best;
   drop / pickup are decided per seed from STOP / REVERSE, shared by all candidates.
3. Clap on kick (Techno / Trap) left no snare source: added secondary roles (kick+clap -> snare high-passed at 200 Hz,
   kick+hat -> hat high-passed at 5 kHz, isolation x0.6, never anchors). Weak same-band bursts within 60 ms merged
   (precision 0.74 -> 0.82).
4. Swing: a straight DnB loop measured 1.6 ticks of swing. Swing now needs >= 4 consistent late odd 16ths.
   TRIPLET RUN won too often: triplet positions count less as syncopation and carry a 0.3 prior over straight grooves.
5. Level-fit term L added after a 1-beat fill measured 9 dB under the groove (it did not change that case; see above).
6. Unit tests found (a) attacks 6..12 ms late (searched on a filtered band with group delay; slices lost the
   transient) -> attack now found on the full-band waveform; (b) a kick's low-mid ripple 52 ms after the attack read
   as a confident snare -> onsets must rise over the tail (novelty >= 12 % of novelty + pre-energy); (c) a sine onset
   read as a snare -> hits that never decay 20 dB in 300 ms with nothing after them are not usable slices.
   One test threshold was a guess and was aligned with the code: drumness >= 0.35 (the Generated threshold), not 0.5.

### Not done / limits
- No owner listening yet: 228 WAVs for listening are in `out/r15/final` (loop, then fill, then the landing beat).
  Statistics above measure faithfulness and timing, not taste.
- pluginval / Steinberg validator not run on this machine (tools live on E:/HamloComp, absent here). The VST3 was
  built: `build/HamloRevolution_artefacts/Release/VST3/HamloRevolution.vst3`. No version bump, ZIP or installer.
- Real full mixes (drums + music) were not in the lab corpus; the drumness / conservative path is tested only on
  synthetic signals. No tom role exists. 4/4 only. Host BPM must match the loop.
- Score weights remain a hypothesis to fit on owner A/B choices.

## R15b: user marks, PERC, WHOLE LOOP (owner request, same day)

- **Marks (optional).** On PHRASE FILL the wave shows the source (faint) behind the result. Right-click a hit to
  snap to its analysed region, or right-drag any region, then choose KICK / SNARE / HAT / PERC. Right-click an
  existing mark to relabel it (its role is ticked) or REMOVE MARK; UNDO LAST MARK (64 steps) and CLEAR ALL MARKS
  are in the same menu. Without marks everything stays automatic.
- **How marks are used.** The strongest attack inside a marked region takes the mark's role with full confidence;
  the region's end is the slice end; no cleaning filter. A role with at least one mark uses only marked hits;
  unmarked roles stay automatic. A region with no detected attack gets one at its first strong sample.
- **PERC** exists only from marks (the analyser never invents it). It plays the snare lane when there is no snare,
  and in 70 % of candidates answers ghost snares and some off-beat hats. Own kit switch and rhythm row.
- **WHOLE LOOP** (EDIT menu, PHRASE recipes only): every bar is re-made from the start of the source, with the
  strong original kick / snare hits kept as the skeleton, bar ends answering, the final bar resolving. Older
  recipes fall back to their 4-beat edit.
- Marks live with the instance like the kit switches (survive NEW VARIATION and editor reopening); a new capture,
  bar change or sample clears them and their undo history.

Verified: 6390 native checks / 0 failed, including an owner scenario test (WAV dropped -> FILL -> mark kick and a
wrong snare -> relabel -> undo -> remove -> clear -> undo -> perc -> NEW VARIATION keeps marks and the dragged WAV
-> WHOLE LOOP -> PERC switch -> editor reopen -> GENERATE clears marks). Lab numbers unchanged (marks off).
Screens: `out/r15/screens/phrase_marks.png`, `phrase_whole.png` (local, not in Git). Not FL-tested.

## R15c: phrase formula from the acoustic reference (targets written before code)

Owner: "Da, popravj" after `docs/reports/r15/ACOUSTIC_REFERENCE.md`. Same tool before and after
(`Tools/r15_reference.py`, R15 group = last bar of the 36 4-beat lab renders, unchanged). "Before" = that report.
The reference is acoustic: it informs Boom Bap and DnB (break-based) fully, Trap and Techno only partly.

| # | Measure (R15 group) | Before | Acoustic A / live BB | Target |
|---|---|---|---|---|
| C1 | Syncopation, median | 0.35 | 0.51 / 0.59 | >= 0.45 |
| C2 | Share on 8ths / on 16ths | .67 / .14 | .32 / .22, .42 / .40 | <= .55 / >= .22 |
| C3 | Longest run of even strokes, median | 3 | 5 / 7 | >= 5 |
| C4 | Last 16th empty | 39 % | 58 % / 71 % | 50..75 % |
| C5 | Dynamics beat 4 / beat 1, median | 1.15 | 0.72 / 0.59 | <= 1.0 |
| C6 | Microtiming median / off-grid share | 3.3 ms / .11 | 9.2 / 11.1 ms, .24 / .11 | 5..9 ms / <= .15 |
| C7 | TOM mark (user marks only; toms are ~40 % of acoustic hits) | none | - | mark, kit switch, runs |
| C8 | No regression: native tests, lab tone cosine (median), recall / role accuracy | 0.994 | - | all pass, >= 0.98, unchanged |

Doubts written before the run: the measured bar includes R15's kept groove beats (quantized source), so C1/C2/C6
cannot reach acoustic values by the figure alone; C6 includes analyser error (~2.7 ms on clean snare rolls); the
lab grid-deviation metric (0.16 ticks) will rise with deliberate microtiming and is not a target any more.

### R15c results (2026-11-19, corrected tool, 36 R15 4-beat renders; per-genre n = 9)

| # | Measure | Before | After | Target | Met |
|---|---|---|---|---|---|
| C1 | Syncopation median (BB / DnB / Trap / Techno) | 0.35 (.36/.33/.33/.37) | 0.40 (.46/.44/.39/.34) | >= 0.45 | no (BB yes) |
| C2 | 8ths / 16ths | .67 / .14 | .62 / .20 | <= .55 / >= .22 | no |
| C3 | Longest even run, median | 3 | 5 | >= 5 | yes |
| C4 | Last 16th empty | 8 % | 47 % | *corrected: 8..41 %* | over by 6 pts |
| C5 | Beat 4 / beat 1; tension | 1.15; +0.19 | 0.96; +0.08 | <= 1.0 | yes |
| C6 | Microtiming median (BB / DnB / Trap / Techno); off-grid | 3.3 ms; .11 | 5.0 ms (10.0/2.8/6.0/0.9); .12 | 5..9 ms; <= .15 | yes (just) |
| C7 | TOM mark | none | mark, kit switch, row, descending runs | mark | yes (R15d, below) |
| C8 | Tests; tone cosine median / worst; level worst; recall, role acc. | 6390; .994/.274; 11.4 dB; .885, .961 | 6391 / 0 failed; .994/.319; 11.4 dB; unchanged | pass, >= .98 | yes |

What changed in the formula: 42 new cells over 4 genres (linear kick/snare 16ths, cadences with a rest on the last 16th, RUN
cells), a RUN mode (20..60 % of candidates by CHOP: even strokes over the last two beats into a run cadence), a
flatter energy contour (0.7..1.0 of base, first 0.45..1.0), T = 0.5 x closeness of tension to +0.05 plus landing
(first: raw tension), a run term +0.4 N (N = longest even stream 3..6 -> 0..1), syncopation targets BB 0.55 /
Trap 0.50 / DnB 0.55 / Techno 0.40 (first 0.35 / 0.45 / 0.45 / 0.30), and deterministic feel in ms (ghosts +9,
backbeat accents +5, added kicks -3, hats +4 on odd / -1.5 on even 16ths, rolls / runs rushing -2..-6 by figure
position; scaled BB 1.0, DnB 0.7, Trap / Techno 0.35, conservative x0.5). Anchors, carried hats, reverse and
grace notes are not moved.

Changes after a measured run (disclosed, rule 4):
1. **Measurement error found.** `rest_last_16th` used `tick >= 90` with no tolerance: rushed or pre-rolled last-16th
   hits counted as rests. Fixed (split half-way, 87), all groups re-measured. This changed the reference itself:
   acoustic 41 % (not 58 %), live BB 12 % (not 71 %), rolls 8 % (not 47 %). Target C4 (50..75 %) came from the
   wrong numbers; corrected range 8..41 % (the measured groups). R15c is 6 points over it (Techno 78 %, Trap 67 %).
2. Landing weights: first R15c run rewarded only the rest (+0.35, hit +0.15) -> 86 % rest on the wrong tool. A
   retune to +0.2 was made and then reverted once the tool error was found. Final: hit on the last 16th and
   "strong last 8th, then rest" are both +0.35 (the choice comes from the cell library).
3. New Trap cadence `...x / ..X. / RRR.` put a kick into 1-beat hat-only windows: worst level +18.8 dB (first
   R15 11.4). Its kick was removed: worst 11.4 dB again.
4. Lab grid deviation (0.16 -> 0.19 ticks median) rises by design (feel); it is no longer a target.

Not met and why: C1 / C2 - the measured bar includes the 1..2 kept groove beats (quantized source, 8th hats) and
Techno keeps quarter kicks, so the figure alone cannot move the whole-bar numbers to acoustic values (doubt written
before the run). C6 is met overall by Boom Bap and Trap; DnB 2.8 ms (its rushing rolls are short at 172 BPM).
C7 TOM was done in the next step (R15d).
Same seed now gives a different phrase than the R15 build from earlier today (R15 is unreleased).
Renders: `out/r15/r15c` (local). Not listened to in FL.

## R15d: TOM mark (owner: "Da, delaj TOM")

- **TOM is a 5th role, from marks only** (the analyser never classifies a tom, like PERC). Mark menu, kit switch
  (`TOM / ON / n SLICES`, orange) and rhythm row on the page; `kKitRoles` 4 -> 5. Kit switches and marks are not in
  the saved state, so no state migration; all kit arrays default to ON.
- **How toms are played.** With tom marks, 75 % of candidates are tom candidates. In them the normal and accented
  snare strokes of the later figure (from 40 % of the figure on for 3+ beat fills, the whole figure for 1..2 beats,
  each bar's second half in WHOLE LOOP) move to the toms; ghosts and rolls stay on the snare. Without a snare and
  a perc, toms play the snare lane. Several toms are ordered high -> low by the zero-crossing rate of their body
  (5..80 ms after the attack) and the fill runs down them in time. Toms choke only their own drum. Slice length
  up to 0.40 s. Family name gets "+ TOMS".
- **Verified.** 6444 native checks / 0 failed. New test: never invented without marks; 2 marked toms (low marked
  first on purpose) used in >= 6 of 12 seeds (62 strokes), only the marked hits, the high tom always before the low
  one, switch mutes, deterministic, bounded. Owner scenario adds a TOM mark from the page. Without marks all 228
  lab renders are bit-identical to R15c (hash compare). Screen `out/r15/screens/phrase_tom.png` checked: five
  buttons and five rows fit.
- **Limits.** Pitch order is a zero-crossing heuristic (a tom with a loud stick click may sort higher than it
  sounds). Not FL-tested.
