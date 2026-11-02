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

## 6. R1 as designed (2026-11-02, before the code)

Owner: "R1 lanes first". R1 grows the three engines into lanes **inside the HPEQ engine** (TIME, GLITCH, MOTION keep
their DSP); the 3-band split is R2 (with SHAPE). Every new setting has a default that means "as before", so the
Legacy scene (all defaults) must give R0's output bit for bit (Tests/GoldenTests.cpp: R0's hashes, recorded first).

Per lane (TIME, GLITCH, MOTION):
- **LENGTH**: FOLLOW (= STEPS, legacy) or 1 .. 32 steps; lanes of different lengths run as a polymeter, each from bar 1.
  Steps 17 .. 32 are their own pattern values (4 more packed quads per lane).
- **RATE**: FOLLOW (= GRID / TRIPLET, legacy) or 1/4, 1/8, 1/16, 1/32 and their triplets. Swing and humanize apply
  to every lane on its own grid. In FREE mode lanes follow (free steps have no note value).
- **CONDITION** (per pattern cycle of the lane, counted from bar 1): ALWAYS, 1:2, 2:2, 1:4, 4:4 (the fill cycle),
  8:8, NOT 1:4. A cycle that does not pass plays through (dry for that engine).
- **PROBABILITY** per step: 100 % (legacy), 88, 75, 63, 50, 38, 25, 13 %. The draw is a hash of (seed, lane, absolute
  step), so it differs per cycle, is the same on every playback and does not depend on the host block size.

Engine slot: `FxSlotParams::values` grows from 48 to 112 floats in this repository (HPEQ keeps 48); new indices
48 .. 92: per lane length, rate, condition, 4 quads for steps 17 .. 32, 8 quads of probability codes (4 bits per step,
0 = 100 %).

**Scenes**: 8 per instance (pattern, feel, seed and the lane settings each), chosen by the automatable **SCENE**
parameter (1 .. 8; it changes at once, the engine crossfades as for any pattern edit). Scene 1 is what R0 had; GENERATE
and every edit act on the scene shown. A new scene is a copy of scene 1 until edited (COPY / CLEAR on the page).

Page (until R6 redraws it): right click on a lane plate - LENGTH, RATE, CONDITION, "show steps 17 - 32"; right click on
a step - PROBABILITY; a SCENE selector (1 .. 8) with COPY TO / CLEAR in the top row.

R1 targets: V1 (the slot of every program = HPEQ's; GoldenTests: R0's hashes, scenes and lanes at defaults), V2 (a lane
at 1/8 with a 1/16 grid: the GLITCH gap starts within 1 sample of the lane's step boundary, swing included), V3 (blocks
1 / 64 / 127 / 4096 bit-identical with lengths 3 / 5 / 32, rates, conditions and probabilities on), V6 (latency 0).
Lane checks: length 3 repeats every 3 steps; 1:2 silences the even cycles; 50 % keeps 40 .. 60 % of the events over
64 cycles and 100 % all of them; SCENE 2 plays scene 2's pattern; scenes and lanes survive the state.

## 7. R2 as designed (2026-11-02, before the code)

Owner: "R2 SHAPE + bands".

**Bands.** Every lane (TIME, GLITCH, MOTION, SHAPE A, SHAPE B) gets BAND: FULL (as before), LOW, MID, HIGH. A lane on a
band splits its input with a 3-band Linkwitz-Riley tree (4th order; the low band through the mid / high crossover's
all-pass so the three bands sum to an all-pass), processes only its band and adds the others back - glitch the hats,
leave the kick and the 808 alone. Zero latency (IIR). Two automatable crossovers for the whole plug-in: LOW / MID
(40 .. 1000 Hz, default 200) and MID / HIGH (1 .. 12 kHz, default 2.5 kHz). FULL = no split = R0 / R1 bit for bit.

**SHAPE A, SHAPE B** - two new lanes after TIME / GLITCH / MOTION (in that order, after the three in any ORDER). Each:
ON, MIX, DEPTH, SMOOTH, RES (filter resonance), a TARGET - VOLUME (gate / trance / pump), LOW PASS, HIGH PASS, BAND
PASS, PAN (alternating left / right per step), WIDTH, DRIVE (tanh, level-matched at -10 dBFS), CRUSH - and STEPS or
CURVE (a Catmull-Rom curve through the step levels instead of steps with SMOOTH ramps); 16 levels per step, steps
17 .. 32, and the R1 lane settings (length, rate, condition, probability). Level 0 = untouched; an empty SHAPE lane =
dry exactly. Defaults: ON, VOLUME / LOW PASS, empty pattern (a new instance sounds as R1). The pattern is per scene.

**Page** (grows to 1180 x 964): two more rows (SHAPE A, SHAPE B: plate, ON, MIX, DEPTH, SMOOTH, RES, TARGET window,
16 level cells; no GEN / LOCK until the generator, R4). Lane plates' menu: BAND, CURVE (shape lanes), crossover presets.
A small tag beside the lamp shows a lane's band (and CURVE).

R2 targets:
- V1: GoldenTests unchanged (bands FULL, shape lanes empty).
- V3: blocks 1 / 64 / 127 / 4096 bit-identical with bands and both SHAPE lanes on (every target over the test).
- V4: SHAPE VOLUME as a 1/16 gate (levels 15 / 0) on a DC input (the output is the gain itself): the largest second
  difference of the gain, |g[n] - 2 g[n-1] + g[n-2]| (the residual of a straight line through three samples; a hard
  step = 0 dB), <= -40 dB with SMOOTH 0 (the 2 ms fade) and in CURVE mode. *Fixed before the measurement: the first
  wording, the gain's change between two samples, is 0.016 (-36 dB) for the 2 ms raised-cosine fade by its design
  and is not a click measure.*
- V5: every lane on a band with an empty pattern, crossovers 200 Hz / 2.5 kHz: output magnitude 20 Hz .. 20 kHz flat
  within 0.1 dB (impulse response).
- Band isolation: GLITCH GAP on HIGH over a 60 Hz + 6 kHz pair: the 60 Hz level within 0.5 dB of dry, 6 kHz down
  >= 30 dB in the gap.
- SHAPE checks: LOW PASS at full level takes 8 kHz down >= 20 dB; PAN step levels move the balance by >= 10 dB with the
  sign alternating per step; DRIVE adds harmonics (THD >= 5 % at level 15) and keeps a -10 dBFS sine within 1 dB;
  CURVE passes through the step levels at step starts.
- V6 latency 0; V13 shape lanes and bands in the state; V14 validators.

## 8. R3 as designed (2026-11-02, before the code)

Owner: "R3 THROW".

**THROW A, THROW B** - two more lanes after SHAPE A / B (lanes 5 and 6; the R1 lane settings, steps 17 .. 32 and BAND
work as for every lane). A step's level 0 .. 15 (x DEPTH) is how much that step sends to, or how hard it drives, the
lane's TARGET:

| Target | Kind | Level = | TAIL | TONE | TIME / NOTE |
|---|---|---|---|---|---|
| DELAY | send | the send into a ping-pong delay | feedback 0 .. 0.9 | damping 1 .. 18 kHz | TIME (synced) |
| TONAL | send | the send into a short delay tuned to NOTE (a pitched comb, C3 .. B3) | feedback 0 .. 0.97 | damping | NOTE (HPDG's key in R5) |
| REVERB | send | the send into a reverb (8-line FDN) | decay 0.3 .. 8 s | damping 2 .. 16 kHz | - |
| PHASER | insert | intensity (6 all-pass stages swept by an LFO) | feedback 0 .. 0.85 | centre 200 Hz .. 2 kHz | TIME = LFO period |
| FLANGER | insert | intensity (0.3 .. 5 ms swept delay) | feedback 0 .. 0.9 | sweep width | TIME = LFO period |
| VINYL | insert | wow depth (to 2 ms), record EQ (30 .. 150 Hz high pass, 18 .. 4 kHz low pass) | brake: the speed falls to 1 - TAIL over the step | crackle | TIME = wow period |
| DOWNSAMPLE | insert | the hold factor 1 .. 32 (no bit loss; CRUSH in SHAPE has the bits) | - | - | - |

- A **send** throw's tail keeps ringing after its step (that is the throw); the return is added: out = in + MIX x wet.
  An **insert** throw blends: out = in + MIX x (fx - in), level 0 = exactly dry.
- The send is faded like a SHAPE step (the 2 ms raised cosine); LFOs run from the musical position (q), so the
  result does not depend on the block size and repeats on every playback.
- An empty THROW lane whose tail has died (or an insert target) is skipped: exactly dry. A lane goes idle after its
  return stayed below -120 dBFS for its buffer's length; its state is cleared on the way out (and on a target change).
- Controls per lane: ON, MIX, DEPTH, TAIL, TONE, TARGET, TIME (1/32 .. 4 bars, triplets, dotted), NOTE (C .. B).
  Defaults: ON, MIX 100 %, DEPTH 100 %, TAIL 50 %, TONE 50 %, TIME 1/8 dotted, NOTE C; A = DELAY, B = REVERB; empty
  pattern (a new instance sounds as R2).
- Slot: values grow to 208: THROW A / B 152 .. 175 (stride 12), their lane settings 176 .. 205, their bands 206, 207.

**Page** grows to 1180 x 1280 (two rows; the band footer moves down); the editor opens at 75 % (885 x 960, resizable
50 .. 200 % as before) so it still fits a 1080p screen. The TARGET window shows target and TIME / NOTE; click =
next target, right click = a menu with targets, TIME and NOTE. Row knobs: MIX, DEPTH, TAIL; TONE on the right.

R3 targets:
- V1: GoldenTests unchanged (throws empty).
- V3: blocks 1 / 64 / 127 / 4096 bit-identical with both THROW lanes on (every target over the test, on a band too).
- V4: a 100 Hz sine at -6 dBFS, a 1/16 gate pattern (levels 15 / 0) on each target: the largest fourth difference
  |y[n] - 4 y[n-1] + 6 y[n-2] - 4 y[n-3] + y[n-4]| (the residual of a cubic through the four samples before) within
  3 ms of a step boundary <= -40 dB re the sine's peak, or not above the same target's own largest value at a constant
  level 15 (a downsampler's stairs are the effect, not a splice click).
- V7: one instance, 48 kHz / 256, release build, on this machine (Intel i5-10300H, the owner's): TIME, GLITCH,
  MOTION patterns, SHAPE A / B on bands, THROW A / B (every pair of targets; the worst counted) on the drum loop:
  mean processing time <= 15 % of the block's duration.
- Throw checks: DELAY echoes at TIME within 1 sample, alternating L / R, echo 2 / echo 1 = 0.9 x TAIL within 0.5 dB
  (TONE 100 %), nothing sent from a level-0 step (<= -90 dB), the tail continues after the step; TONAL's spectrum
  peaks at NOTE within 1 %; REVERB's T30 within 25 % of the nominal decay (TAIL 50 % = 2.2 s); PHASER and FLANGER move
  a sine's level by >= 10 dB over the LFO; VINYL's wow moves a 1 kHz sine's frequency by >= 0.3 %, BRAKE (TAIL 100 %)
  lowers it by >= 40 % over the last quarter of the step; DOWNSAMPLE at level 15 makes the fs/32 alias of 1 kHz
  (500 Hz) >= -30 dB re the tone; an emptied DELAY lane keeps its tail, then goes idle (output = input exactly).
- V6 latency 0; V13 throws in the scenes and the state; V14 validators.

*Changed during R3 (each found by a measurement above, written here before the final run):*
- *VINYL's wow depth glides to the level over about 50 ms instead of following the 2 ms step fade (V4 found the head
  moving up to 2 ms within 2 ms, a speed jump: -22.8 dB).*
- *VINYL and DOWNSAMPLE blend in with a smooth step that is full at a quarter of the level, instead of a straight ramp
  to a tenth (its kink: -27.1 dB), and VINYL's record EQ is redesigned every sample while the level moves instead of
  every 16 samples (the staircase: -34.5 dB). After both: -48.3 dB. SHAPE's filters and DRIVE (R2) still use the
  straight ramp; R2's V4 measured VOLUME only - an open item, not changed here.*
- *A lane on a band passes through the split's all-pass even when idle (as SHAPE in R2): "exactly dry" holds on FULL.*

## Sources

1. [Sound On Sound: ShaperBox 3](https://www.soundonsound.com/reviews/cableguys-shaperbox-3), [MusicRadar: ShaperBox 3](https://www.musicradar.com/reviews/cableguys-shaperbox-3), [ShaperBox 3 manual](https://www.cableguys.com/downloads/Cableguys-ShaperBox-3-Manual.pdf?v=3.3.1)
2. [KVR: Looperator](https://kvraudio.com/product/looperator-by-sugar-bytes), [App Store: Looperator](https://apps.apple.com/app/looperator/id1506569603)
3. [Rekkerd: Effectrix 2](https://rekkerd.org/sugar-bytes-releases-effectrix-2-multi-effects-sequencer-plugin/), [Dubspot: Effectrix 2](https://blog.dubspot.com/plugins/effectrix-2), [Synth Anatomy: Effectrix 2](https://synthanatomy.com/2023/10/sugar-bytes-effectrix-2-popular-sequenced-multi-fx-plugin-gets-a-makeover.html)
