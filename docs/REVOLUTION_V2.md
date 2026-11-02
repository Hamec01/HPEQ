# HamloRevolution v2: design and targets

Owner (2026-11-02): HamloRevolution must not be a "screensaver" effect but a rhythmic multi-effect sequencer in the
spirit of ShaperBox, Looperator and Effectrix (ideas, not copies). Today's page (TIME / GLITCH / MOTION lanes, 16
steps) becomes the **ADVANCED** mode; a **SIMPLE** mode is added - generation controlled by 4 - 5 knobs; a
**CONNECT TO HPDG** button lets the owner's drum generator HPDG feed tempo, genre, groove and its pattern, and
Revolution writes its patterns from them. Targets are fixed here **before** any code (AGENTS.md rules).

## 1. References (published behaviour used as orientation)

| Reference | What it shows |
|---|---|
| Cableguys ShaperBox 3 | drawn LFO shapes for Volume, Filter, Time, Pan, Width, Drive, Crush, Noise, Liquid; each shaper in three bands (low / mid / high); Audio Trigger (shapes started by the input's transients); sidechain [1] |
| Sugar Bytes Looperator | a 16-step slicer / looper (slice, stutter, reverse, filter); per-step modulation (knob, random, envelope follower, curves); run from the host transport or MIDI notes [2] |
| Sugar Bytes Effectrix 2 | 32 steps, 14 effects painted on a grid (loopers, vinyl, grain, tonal delay, reverb, phaser, crush, filter, level); a modulation sequencer per effect (from-to, steps with smoothing, LFO, envelope follower, step train); 12 patterns recalled by MIDI keys; chaos / randomise [3] |
| HamloRevolution in HPEQ (pass 10 / 10b) | TIME (half, reverse, freeze, tape stop, double, pitch-kept stretch: WSOLA / phase vocoder), GLITCH (repeat, stutter, ratchet, reverse, gap, jump, scratch), MOTION (16 levels -> volume / low-pass / high-pass / width / crush) in 6 orders; 16 steps; a generator with characters; LISTEN (attack strength per step); zero latency; click-free splices (C-ATD); block-size independent clock (`E:\HPEQ\Source\DSP\HamloRevolution.h`, `Core\Revolution.h`) |

What none of the references has, and Revolution will: **genre-aware generation that follows the real groove** -
HPDG's genre, substyle, swing and the actual hits of its pattern - and a one-screen SIMPLE mode built on it.

## 2. The two modes

### SIMPLE (the default page)

| Control | What it does |
|---|---|
| CONNECT TO HPDG | links to a running HPDG instance (a list when there are several); lamp: off / searching / linked; when linked, GENRE = AUTO follows HPDG |
| GENRE | AUTO (from HPDG, else from LISTEN), BOOM BAP, TRAP, TECHNO, DNB |
| INTENT (knob) | SUBTLE -> GROOVE -> FILLS -> BREAKDOWN -> CHAOS: what kind of events the generator writes |
| AMOUNT (knob) | how much of each bar is affected (event density, depth) |
| VARIATION (knob) | 0 = one loop repeats; up = every bar mutates, fills differ |
| GROOVE (knob) | 0 = straight grid; 100 % = the steps take the groove of HPDG's pattern (its swing and micro-timing) or, unlinked, the input's (LISTEN) |
| FILL | off / every 1 / 2 / 4 / 8 bars: where the bigger events go (phrase ends) |
| TARGET | what is processed: HATS / SNARE / TOPS (everything above the kick) / FULL - with the bands and, when linked, the HPDG lanes' hit positions |
| GENERATE, LOCK, MIX | a new pattern (new seed); keep the current one; dry / wet |

SIMPLE writes the same lanes ADVANCED shows: switching to ADVANCED opens what SIMPLE made, to edit by hand.

### ADVANCED (today's page, grown)

- **Lanes** instead of three fixed engines; each lane: its own length (1 .. 32 steps; different lengths = polymeter),
  rate (1/4 .. 1/32, triplets), per-step probability, intensity and condition (every Nth bar, fill only, not on
  the first bar ...), and a band (FULL / LOW / MID / HIGH, a 3-band Linkwitz-Riley split - glitch the hats, leave the
  kick and 808 alone).
- **SLICE lanes** (from TIME + GLITCH): repeat, retrigger, ratchet, reverse, tape stop, half / double speed (pitch
  kept or not), scratch, jump / juggle (play step k's slice at step j), freeze, gap.
- **SHAPE lanes**: volume (gate, trance, pump), filter (LP / HP / BP + resonance), pan, width, drive, crush - each
  as **steps** or a drawn **curve** with smoothing.
- **THROW lanes**: per-step sends to a tonal delay (tuned to HPDG's key when linked), a reverb tail, a phaser /
  flanger, vinyl (wow, brake), downsample.
- **Triggers**: the host transport (default), the input's own hits (LISTEN: an event sits on the hit, not only on the
  grid), an external sidechain (pump from another track's kick), MIDI notes (FL Studio sends MIDI to a VST3 effect
  only when routed; it is optional - the SCENE parameter is the main switch).
- **Scenes**: 8 patterns per instance; switched by the SCENE parameter (automation), MIDI notes, or FILL EVERY N BARS.
- Today's orders / TIME / GLITCH / MOTION settings load as a **Legacy** scene that sounds exactly as in HPEQ.

## 3. CONNECT TO HPDG (HPDG Link)

Protocol and both sides: [HPDG_LINK.md](HPDG_LINK.md). In short: HPDG publishes a small fixed-size snapshot (tempo,
genre, substyle, swing / timing / humanize / density / intensity, key, bars, its pattern's hits per lane with tick,
velocity and role, a version counter and a heartbeat) into a named, versioned shared-memory slot; Revolution reads
it off the audio thread, and the generator uses it. Read-only, local, no network, no audio-thread locks; if HPDG
closes, Revolution keeps the last pattern and falls back to LISTEN.

The generator's genre rules (first version; every rule is a test in the lab):
- **Boom Bap**: chops - jump / juggle on the sample hits, vinyl brake or tape stop at the 4th or 8th bar end, low-pass
  swells on the loop, short repeats on the snare before a phrase change; GROOVE pulls everything to the swing.
- **Trap**: hat rolls (ratchet 1/32, 1/48 on the hat lane's positions), tape stop / pitch-down of the whole bar before a
  drop, stutter of the last snare, 808 kept clean (TARGET avoids LOW), half-time switches on BREAKDOWN.
- **Techno**: volume gate / pump on 1/16 and 1/8, filter sweeps over 4 - 8 bars, reverb and delay throws on the off-beat
  claps, slow width moves; CHAOS = crush and stutter bursts.
- **DnB**: break reverse on the last beat, ratchets on the snare (2 and 4), tape stops at 8-bar ends, half-speed drops,
  amen-style jumps on the break; fast, short events.

## 4. Measurements (`HamloRevolution_Lab`) and targets

| # | Check | Target |
|---|---|---|
| V1 | Legacy: HPEQ's Revolution settings (every order, character, stretch, the factory presets) against the M0 build | bit-identical |
| V2 | Timing: every step boundary against the host position (tempo 60 .. 200, tempo ramps, transport jumps, 44.1 / 48 / 96 kHz) | within 1 sample |
| V3 | Block sizes 1 / 64 / 127 / 512 / 4096 after 256, every lane type on | bit-identical |
| V4 | Splices: a sine and the drum loop through every SLICE action: the largest discontinuity at a splice (residual of a smooth fit) | <= -40 dB re the signal |
| V5 | Bands: every lane on but at zero depth, 3-band split: the sum against the input | magnitude flat within 0.1 dB |
| V6 | Latency: no stretch, no band split = 0 samples; otherwise the reported latency equals the measured one | exact |
| V7 | CPU: one instance, 48 kHz / 256, every lane type on, on the owner's i5-10300H | <= 15 % of the block time |
| V8 | LISTEN / HPDG lock: events marked "on the hit" sit on the input's (or HPDG's) hit positions | within 1 ms |
| V9 | GROOVE 100 %: the step grid's offsets against HPDG's swing / micro-timing | within 1 ms |
| V10 | Generator, per genre x intent: event density follows AMOUNT (monotonic), FILL events at phrase ends (>= 90 %), TARGET HATS never touches the LOW band | as stated |
| V11 | VARIATION 0: bar n = bar n + k exactly; VARIATION 100 %: bars differ, the genre rules still hold | as stated |
| V12 | HPDG Link: the snapshot round trip (HPDG writes, Revolution reads, every field); HPDG quits -> lamp off, last pattern kept; a newer protocol version -> refused cleanly | exact |
| V13 | State: SIMPLE and ADVANCED settings, scenes, the lanes, the link choice recalled | exact |
| V14 | pluginval 10, Steinberg validator | pass |

## 5. Stages (one stage = one chat for the agent, a commit at the end)

1. **R0 - M0**: `E:\HamloRevolution` as the other M0s (HPEQ's module-plug-in stack, a new ModuleKind::Revolution,
   kind 9, code HmRv; the pattern / feel / seed kept in the plug-in state; AGENTS.md "HamloRevolution" plan). V1 holds.
2. **R1 - lanes**: the lane model (length, rate, probability, condition, band), scenes, SCENE parameter; Legacy scene =
   M0. V1 - V3, V6.
3. **R2 - SHAPE lanes** with steps / curves and the 3-band split. V4, V5.
4. **R3 - THROW lanes** (tonal delay, reverb tail, phaser / flanger, vinyl, downsample). V4, V7.
5. **R4 - generator + SIMPLE page** (genre rules, INTENT, AMOUNT, VARIATION, GROOVE from LISTEN, FILL, TARGET). V8 -
   V11.
6. **R5 - HPDG Link**: HPDG's publisher (in `C:\DRUMENGINE`, additive, see HPDG_LINK.md) + Revolution's client and
   CONNECT button; GENRE AUTO and GROOVE from HPDG. V8, V9, V12.
7. **R6 - ADVANCED page** (grid painting, curve editor, lanes), help texts, 200 genre presets (groups: Drum Bus, Hats /
   Percussion, Loops / Samples, Vocals, Transitions / Fills, Master FX), lab "after", validators, installer, report.

## Sources

1. [Sound On Sound: ShaperBox 3](https://www.soundonsound.com/reviews/cableguys-shaperbox-3), [MusicRadar: ShaperBox 3](https://www.musicradar.com/reviews/cableguys-shaperbox-3), [ShaperBox 3 manual](https://www.cableguys.com/downloads/Cableguys-ShaperBox-3-Manual.pdf?v=3.3.1)
2. [KVR: Looperator](https://kvraudio.com/product/looperator-by-sugar-bytes), [App Store: Looperator](https://apps.apple.com/app/looperator/id1506569603)
3. [Rekkerd: Effectrix 2](https://rekkerd.org/sugar-bytes-releases-effectrix-2-multi-effects-sequencer-plugin/), [Dubspot: Effectrix 2](https://blog.dubspot.com/plugins/effectrix-2), [Synth Anatomy: Effectrix 2](https://synthanatomy.com/2023/10/sugar-bytes-effectrix-2-popular-sequenced-multi-fx-plugin-gets-a-makeover.html)
