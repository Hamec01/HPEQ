# R17 / 0.5.0 — the drum fill engine, rebuilt

Owner, 2026-11-19, after R16 in FL: *the drum fill always sounds weak: no breath, no motion, no rhythm; it does not
sound like a drum fill.* References he sent (watched as titles / descriptions; the audio was not analysed):

| Reference | Published behaviour used here |
|---|---|
| Hack Music Theory, *3 Types of Drum Fills* | three jobs: VARIATION (a few extra notes, groove kept), TENSION (big and loud, starts on 1/4 or 1/8 and builds into 1/16, 1/32, 1/64; toms and 1/16 / 1/32 triplets at the end), NOTIFICATION (size by the section it leads into; a build may stop in silence) |
| Cableguys, *A Drum Fill Recipe For Transitions And Pre-Drops* | repeating snares lead into one huge, reverbed snare before the drop |
| Drumming Basics 101, *Drum Fill Around the Kit* | even strokes snare -> high tom -> middle tom -> floor tom, at the end of a groove |
| Splice, *4 Types Of Fills* | (title only) |

Owner, the same day: *re-make it completely, other knobs are fine; and make a fill from samples too: drop it and it
gets chopped (sometimes interesting, sometimes bad, that is fine), but a drum part must sound more or less good every
time.*

## Why R12..R15 were weak

R15c took live acoustic drummers as its reference and flattened the fill on purpose: the end was not louder or denser
than the start (beat 4 / beat 1 = 0.72), velocities were narrow, cells were chosen one beat at a time with no motif,
hats and the groove dropped out, and every new stroke choked the last one. That is a played break, not the
producer fill the owner means: a fill has to *move towards the downbeat*. The mistake was the reference, not a knob.

## The new engine (recipes 35 .. 41; 0 .. 34 stay as they were)

A fill is one of six recognisable TYPES, each a small set of rhythm rules with variations, played with the best
slices of the source. AUTO chooses the type from the source and the length. Every type keeps the original groove up
to its turn point; the source never stops dead on a wrong beat.

| Recipe | Type | What it does |
|---|---|---|
| 35 | FILL AUTO | drums found: BUILD / AROUND THE KIT / BIG SNARE / GROOVE BREAK / TRAP ROLL by length, tempo and seed; no drums (or a sustained sample): SAMPLE CHOP |
| 36 | BUILD | snare (plus a kick drive) accelerating 1/4 or 1/8 -> 1/16 -> 1/32 (-> 1/64 at the end when ENERGY is high and the tempo allows); level climbs, pitch rises with PITCH; the groove under it is high-passed and fades |
| 37 | AROUND THE KIT | even 16ths: snare, high tom, middle tom, floor tom, an accent at the start of every drum, a kick under the downbeats; marked toms are used, otherwise three toms are made from the snare (pitched down, low-passed) |
| 38 | BIG SNARE | snares on 1/8, then 1/16, into one huge layered snare (snare + the snare an octave lower + the kick) with a long reverb tail |
| 39 | GROOVE BREAK | the groove plays on; a few ghost snares in its gaps, and the last beat answers with a flam, a kick pickup and (with toms) a tom pickup |
| 40 | TRAP ROLL | a hat roll (snare if no hat) 1/8 -> 1/16 -> 1/16T -> 1/32 (-> 1/64), a volume wave per stage and rising pitch; the groove keeps its kick and snare (hats filtered out) |
| 41 | SAMPLE CHOP | any audio: slices on its attacks (or on 1/8 when it has none) re-cut as a stutter, a re-arrangement, a half-speed drop or a tape stop, with a reverse pickup; a SAMPLE mark on the wave chooses the material |

Knobs (new names for 35 .. 41; the old recipes keep CHOP / ROLL / REVERSE / STOP):

| Knob | 0 % | 100 % | Default |
|---|---|---|---|
| ENERGY | a short, soft fill (1/16 at most, small crescendo) | long, dense, loud (1/32 .. 1/64, +12 dB crescendo, kick drive) | 60 % |
| SPACE | dry, tight, tails choked | rests left open, tails ring, a room on the fill and a long tail on the landing | 40 % |
| PITCH | no pitch movement | builds rise up to +7 semitones, toms spread wider, chops step up | 50 % |
| DROP | runs straight into the downbeat | silence before the downbeat (1/16 .. 1/4) and a low cut on the build | 25 % |

LENGTH (the EDIT LAST button): 1, 2, 4 beats or 8 beats (2 and 4 bar sources). NEW VARIATION keeps the type and
gives another variant of it. Kit switches (KICK .. TOM) mute a role. FILTER + ROOM is hidden for 35 .. 41 (SPACE).
All four knobs at 0 keep the source (as R15). A fresh page opens on FILL AUTO; transition settings are not part of the saved state, so no project changes.

## Targets (set before the code; checked by `Tests/DrumFillR17Tests.cpp`)

| # | Target |
|---|---|
| F1 | BUILD, 4 beats, ENERGY 60 %: strokes in the last beat >= 4 x the first beat; the last beat's stroke gain >= +6 dB over the first beat's |
| F2 | AROUND THE KIT: >= 3 tom voices, struck in strictly falling pitch order (marked toms by their pitch, made toms by their rate) |
| F3 | BIG SNARE: the landing snare's level >= +3 dB over every other snare stroke; its tail rings for >= 0.8 s or to the end of the clip (DROP 0) |
| F4 | GROOVE BREAK: the source is unchanged (bit-exact) before the last beat apart from added strokes (the groove plays on) |
| F5 | TRAP ROLL: at least four stroke rates, the fastest >= 1/32 at ENERGY >= 60 % |
| F6 | SAMPLE CHOP: on a sustained tone (no drums) AUTO picks SAMPLE CHOP and the clip differs from the source after the turn |
| F7 | every type, 1 / 2 / 4 / 8 beats, 4 seeds, 90 / 140 / 174 BPM: finite, the exact length, true peak <= max(source peak, -1.8 dBTP), the audio before the turn bit-exact, deterministic (same seed = same samples) |
| F8 | recipes 0 .. 34: every existing test and golden comparison unchanged (old renders bit-exact) |
| F9 | groove and rhythm: every stroke is placed on a 1/48-beat grid plus the loop's own swing on the off 16ths (by construction; measured by the lab's onset-grid deviation, not a unit test) |

What the tests cannot prove: taste. The owner's listening in FL decides; the targets only prove that each type does
what its name says.

## Results (2026-11-19)

Code: `Source/DSP/DrumFillR17.cpp` (new; the R15 analyser is reused as it is), dispatch in `TransitionRender.cpp`,
`TransitionRecipes.cpp`, `ModulePluginProcessor.cpp`; page: knob names per recipe, STYLE menu (R17 first, the older
recipes in submenus), EDIT LAST 8 BEATS, FILTER + ROOM hidden; help texts. Version 0.5.0.

Tests: `Tests/DrumFillR17Tests.cpp` F1 .. F8 pass on the first run of the finished engine (F7: 7 types x 1/2/4/8 beats x
4 seeds x 90/140/174 BPM). Full suite 8598 passed, 0 failed. Old tests that asserted the FILL default (PHRASE AUTO,
recipe 30) or `maximumRecipe(Fill) == 34` now assert FILL AUTO / 41; the R15 owner-scenario tests choose PHRASE AUTO
explicitly and still pass unchanged otherwise.

Lab (`PHRASE_R17=1 HamloRevolution_DrumPhraseLab <kit> <out>`, 12 loops built from real one-shots: Ecliptiq Boom Bap
Supreme Vol.1 and AngelicVibes BLXCK kits, boom bap / trap / DnB / techno patterns, 756 renders): no non-finite output;
onset-grid deviation median 0.16 .. 0.36 ticks (1/24 beat) per type. Level of the edited beats vs the same beats of the
loop: median |difference| 0.1 .. 3.9 dB; the large differences are 1-beat fills over a sparse last beat (+7 .. +20 dB:
the loop is nearly silent there) and some SAMPLE CHOP variants on drums (down to -19 dB: a re-arranged or half-speed
piece of a drum loop can be quiet) - the owner accepted that chops are sometimes poor.

SAMPLE CHOP, first lab run: the stutter piece was a random one of the first three slices and could be a quiet hat
(-19 dB); it now takes one of the two loudest of the first four. Disclosed here; no target changed.

Listening files for the owner (not in git): `E:\HPM\R17_listen` - each file is the 2-bar loop, the 2-bar render, then
the first beat again (the landing); 4 loops x 7 types x 2 lengths + the R15 render of the same loop for comparison.

Not done: no listening by the agent, no pluginval / Steinberg run, no installer, no manual FL check. Taste is the
owner's call.

## R18 / 0.5.1 — WHOLE SAMPLE and BARS 8 / 16 (2026-11-20)

Owner in FL with 0.5.0: *when SAMPLE CHOP is chosen, re-cut the whole sample (there is no such choice); and BARS should
be 1-2-4-8-16, only 1-2-4 now.*

- SAMPLE CHOP opens on EDIT / WHOLE SAMPLE (fillBeats `kWholeLoop`; the other R17 types do not offer it, and leaving
  SAMPLE CHOP goes back to the last 4 beats). Every bar but the last is re-cut from its own attacks (a SAMPLE mark: from
  the marked material): a re-arrangement (50 %), three beats re-arranged plus a stutter on the 4th (30 %) or half speed
  (20 %) - no tape stop mid-phrase. The last bar plays the chosen pattern (stutter / rearrange / half speed / tape stop)
  into the downbeat. 1 / 2 / 4 / 8 beats of SAMPLE CHOP render exactly as in 0.5.0.
- BARS 1, 2, 4, 8, 16 (`legalBars`, every engine; old projects only had 1 / 2 / 4, which are unchanged). The input
  ring grew from 20 s to 66 s, enough for 16 bars down to 59 BPM; slower tempos capture what there is (not complete).
  A dropped sample is repeated to fill the bars, as before.
- Test F10: SAMPLE CHOP / WHOLE SAMPLE at 90 / 140 BPM, 1 / 4 / 16 bars, 4 seeds - finite, exact length, the ceiling,
  deterministic, the first bar changed; BUILD ignores WHOLE SAMPLE. Page test: BARS 8 / 16 reach the processor, SAMPLE
  CHOP sets WHOLE SAMPLE, BUILD resets it to 4 beats.
- Not done: listening; 16-bar renders of the drum types (BUILD etc. still play the last 1 .. 8 beats of the 16 bars).
