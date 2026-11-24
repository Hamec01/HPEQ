# Rack Power presets

Controls stage 0.2.3: all 20 canonical Power renders were replayed both before
and after embedding, with exact PCM matches to 0.2.2. Authored settings, trims,
original 160 checks and 20 tail measurements retain their previous provenance;
the current source proof is separate in `RACK_POWER_RECEIPT.json`. See
[the controls report](CONTROLS_REPORT.md).

Twenty authored demonstrations of complete current plug-ins and HPEQ layer/region routing. Audio verification is recorded separately; topology alone does not prove musical quality.

All 20 passed 160 source/level/tempo checks and 20 silent-tail renders. Output trim alone was adjusted; every authored DSP parameter stayed fixed. Loudness is within +/-0.5 LU for 18/20; 2 headroom-constrained exceptions remain. Listening verdicts remain pending. Full metrics and hashes: [RACK_POWER_RECEIPT.json](RACK_POWER_RECEIPT.json).

| ID | Output trim dB | LU difference | Worst dBTP | Tail end dBFS |
|---|---:|---:|---:|---:|
| PW001 | 1.94 | -0.00 | -2.74 | -240.0 |
| PW002 | 3.98 | +0.00 | -4.62 | -163.2 |
| PW003 | 5.09 | +0.00 | -3.12 | -240.0 |
| PW004 | 1.72 | -0.19 | -0.35 | -240.0 |
| PW005 | 4.04 | +0.00 | -4.02 | -240.0 |
| PW006 | 10.16 | -0.00 | -0.77 | -240.0 |
| PW007 | 4.89 | -0.00 | -3.75 | -158.5 |
| PW008 | 7.28 | -0.00 | -1.99 | -133.8 |
| PW009 | 1.55 | -0.46 | -0.35 | -240.0 |
| PW010 | 2.27 | +0.00 | -3.97 | -240.0 |
| PW011 | 5.28 | -1.13 | -0.35 | -119.1 |
| PW012 | 0.84 | +0.00 | -4.99 | -125.5 |
| PW013 | 0.25 | -0.00 | -2.39 | -240.0 |
| PW014 | 0.19 | -0.00 | -3.91 | -240.0 |
| PW015 | 3.77 | -0.00 | -2.55 | -126.2 |
| PW016 | 1.22 | -0.09 | -0.35 | -240.0 |
| PW017 | 4.09 | -0.00 | -3.92 | -240.0 |
| PW018 | 2.32 | +0.00 | -8.13 | -100.2 |
| PW019 | 1.44 | -1.32 | -0.35 | -240.0 |
| PW020 | -1.82 | -0.00 | -3.86 | -222.8 |

## PW001 — Boom Bap / Basement Parallel Punch

Dry attack over a crushed FET/iron body, with short dirty room behind the backbeat.

Routing: Parallel; reference 90 BPM.

- Attack and tape (layer mix 100%): HamloUdarnik → HamloMag
- Parallel iron crush (layer mix 40%): 140–7500 Hz: HamloComp → HamloMag
- Basement chamber (layer mix 100%): HamloVerb

Reject if: The dry leading hit disappears or the room washes over the next kick.

## PW002 — Boom Bap / Twelve Bit Vinyl Mirage

Split a loop into a steady bass foundation and grainy sampled mids; answer it with pitched stereo echoes.

Routing: Parallel; reference 90 BPM.

- Sampler split (layer mix 100%): HamloComp; 160–14000 Hz: HamloMag
- Fifth answers (layer mix 100%): HamloEcho
- High room (layer mix 100%): HamloVerb → HamloShirota

Reject if: Sub notes wobble or the pitched answers hide the original phrase.

## PW003 — Boom Bap / Broadcast Vocal Theatre

A dense opto/FET vocal with de-essing, dark tape slap and a ducked plate.

Routing: Serial; reference 90 BPM.

- Level and density (layer mix 100%): HamloFader → HamloComp → HamloComp → HamloMag
- Slap and plate (layer mix 100%): HamloEcho → HamloVerb

Reject if: Consonants vanish or the plate pushes the lead behind the beat.

## PW004 — Boom Bap / Break Spin Matrix

Keep the dry break under edited reverse, stop and repeat gestures in the mid band.

Routing: Parallel; reference 90 BPM.

- Dry knock (layer mix 100%): HamloUdarnik → HamloMag
- Mid-band tape cuts (layer mix 65%): 250–9000 Hz: HamloRevolution
- Snare space (layer mix 100%): HamloVerb

Reject if: The kick loses its pulse or generated gestures occur outside the authored final steps.

## PW005 — Trap / 808 Harmonic Engine

A centered sub with aggressive small-speaker harmonics above it and controlled high-band spread.

Routing: Parallel; reference 140 BPM.

- Sub and buzz (layer mix 100%): 25–120 Hz: HamloMag; 120–5000 Hz: HamloMag → HamloComp
- Upper prism (layer mix 50%): 700–9000 Hz: HamloShirota

Reject if: The sub collapses in mono or grit replaces the note fundamental.

## PW006 — Trap / Rage Gate Hammer

Punchy overdriven drums interrupted by a deliberate sixteenth gate, with stuttered stereo answers.

Routing: Serial; reference 140 BPM.

- Hammer (layer mix 100%): HamloUdarnik → HamloMag → HamloComp
- Cut rhythm (layer mix 100%): HamloZastava
- Stutter echo (layer mix 100%): HamloEcho

Reject if: The kick pattern becomes unreadable or the silent gate steps leak a loud dry duplicate.

## PW007 — Trap / Vocal Hologram Stack

A forward compressed lead surrounded by octave/fifth voices and a dark ducked shimmer bed.

Routing: Parallel; reference 140 BPM.

- Lead anchor (layer mix 100%): HamloComp → HamloMag
- Pitch constellation (layer mix 100%): HamloEcho
- Cloud plate (layer mix 100%): HamloVerb

Reject if: The lead becomes a distant harmony pad or sibilants excite a bright wash.

## PW008 — Trap / Cloud Tape Orbit

Tape-coloured chords breathe in a drawn volume pattern while pitched echoes orbit a wide hall.

Routing: Serial; reference 140 BPM.

- Tape pulse (layer mix 100%): HamloMag → HamloFader
- Orbit (layer mix 100%): HamloEcho → HamloVerb → HamloShirota

Reject if: Low notes move off centre or the pulse clicks at a pattern boundary.

## PW009 — Techno / Rumble Foundry

A dry punch layer drives a low-frequency distorted reverb, followed by a tempo-shaped rumble envelope.

Routing: Parallel; reference 132 BPM.

- Kick steel (layer mix 100%): HamloUdarnik → HamloMag
- Low room furnace (layer mix 70%): 35–220 Hz: HamloVerb → HamloMag → HamloFader

Reject if: The next kick is masked by a continuous sub tail or rumble develops a runaway feedback note.

## PW010 — Techno / Acid Ratchet Laboratory

Mid-band reverse, ratchet and stop events cut across saturated material while the low pulse stays intact.

Routing: Parallel; reference 132 BPM.

- Acid bite (layer mix 100%): HamloMag
- Edited mids (layer mix 85%): 300–10000 Hz: HamloRevolution
- Metal reflections (layer mix 100%): HamloEcho

Reject if: The bottom octave stutters or the edited phrase loses its downbeat.

## PW011 — Techno / Warehouse Gate Chamber

A crushed drum body feeds a huge dark room that is chopped into an explicit off-beat rhythm.

Routing: Parallel; reference 132 BPM.

- Dry mechanics (layer mix 100%): HamloUdarnik → HamloComp → HamloMag
- Warehouse return (layer mix 65%): HamloVerb → HamloZastava

Reject if: The gated room replaces the kick attack or leaves high-frequency ringing in the stops.

## PW012 — Techno / Dub Feedback City

Four pitched, panned tape taps feed a dark chamber and a second rhythmic phrase cut.

Routing: Parallel; reference 132 BPM.

- Tape source (layer mix 100%): HamloMag
- Dub network (layer mix 100%): HamloEcho → HamloVerb
- Phrase holes (layer mix 45%): 350–7500 Hz: HamloRevolution

Reject if: Feedback continues to build after the input stops or dry chords lose their placement.

## PW013 — Drum and Bass / Break Surgery Console

Treat the sub, snare body and hat edge independently, then add a crushed mid-band parallel body.

Routing: Parallel; reference 174 BPM.

- Three-band surgery (layer mix 100%): 30–160 Hz: HamloUdarnik; 160–4500 Hz: HamloUdarnik → HamloMag; 4500–18000 Hz: HamloUdarnik → HamloShirota
- Parallel snare engine (layer mix 35%): 180–6500 Hz: HamloComp → HamloMag
- Fast chamber (layer mix 100%): HamloVerb

Reject if: Ghost notes disappear, the snare turns brittle or the centered low band loses weight.

## PW014 — Drum and Bass / Neuro Harmonic Prism

A solid low foundation under heavily driven, rhythmically edited mid harmonics and wide high-frequency answers.

Routing: Parallel; reference 174 BPM.

- Harmonic split (layer mix 100%): 130–3500 Hz: HamloMag → HamloComp; 3500–14000 Hz: HamloMag → HamloShirota
- Neuro edits (layer mix 50%): 250–8500 Hz: HamloRevolution

Reject if: The fundamental changes pitch or the mono bass becomes quieter than its untreated reference.

## PW015 — Drum and Bass / Liquid Chord Cinema

A warm chord source blooms into stereo octave/fifth echoes, long chamber tails and a protected wide high band.

Routing: Serial; reference 174 BPM.

- Warm source (layer mix 100%): HamloMag → HamloComp
- Harmonic cinema (layer mix 100%): HamloEcho → HamloVerb → HamloShirota

Reject if: The next chord is buried or mono fold removes the main harmony.

## PW016 — Drum and Bass / Bar End Break Mayhem

Keep the first half of the break recognizable, then trigger explicit reverse, freeze, stop and repeat events above the kick band.

Routing: Parallel; reference 174 BPM.

- Break anchor (layer mix 100%): HamloUdarnik → HamloMag
- Bar-end surgery (layer mix 90%): 230–14000 Hz: HamloRevolution
- Reverse reflections (layer mix 100%): HamloEcho

Reject if: Events move away from the programmed steps or the downbeat loses its dry anchor.

## PW017 — Studio / Four Stage Vocal Machine

Ride quiet words, level them with opto compression, catch peaks with FET, de-ess and place the result in a ducked plate.

Routing: Serial; reference 120 BPM.

- Ride and tone (layer mix 100%): HamloFader → HamloComp → HamloComp → HamloMag
- Voice stage (layer mix 100%): HamloEcho → HamloVerb

Reject if: Breaths surge, consonants dull or the long tail obscures the dry words.

## PW018 — Studio / Infinite Guitar Room

Tape-softened pick attacks open a wide chamber with octave echoes and a controlled shimmer layer.

Routing: Serial; reference 120 BPM.

- Pick foundation (layer mix 100%): HamloComp → HamloMag
- Infinite room (layer mix 100%): HamloEcho → HamloVerb → HamloShirota

Reject if: Low strings smear, tuning becomes unstable or the wet layer masks the initial pick.

## PW019 — Studio / Parallel Drum Machine

Separate clean snap, an overdriven FET body and a gated stereo room into three independently adjustable layers.

Routing: Parallel; reference 120 BPM.

- Clean snap (layer mix 100%): HamloUdarnik
- Crush engine (layer mix 40%): 100–9000 Hz: HamloComp → HamloMag
- Gated room (layer mix 55%): HamloVerb → HamloZastava → HamloShirota

Reject if: Layer summation flattens the dry hit or the room pumps between the intended gate accents.

## PW020 — Studio / Whole Rack Transformation

A serial demonstration of spectral punch, multiband saturation, harmonic echo, edited motion and controlled stereo depth.

Routing: Serial; reference 120 BPM.

- Punch console (layer mix 100%): HamloComp → HamloUdarnik
- Band colour (layer mix 100%): 140–4000 Hz: HamloMag; 4000–15000 Hz: HamloMag
- Motion cut (layer mix 100%): 400–11000 Hz: HamloRevolution
- Pitch and depth (layer mix 100%): HamloEcho → HamloVerb
- Stereo guard and ceiling (layer mix 100%): HamloShirota → HamloRedWall

Reject if: The effect is only louder, the bass wanders off centre or a sustained section grows without bound.
