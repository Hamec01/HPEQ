# HamloVerb — design note (module pass 2: Reverb)

Owner, 2026-10-22: the reverb must not just delay the sound and widen it — it must imitate spaces
("допустим 5 кабин", like FL Studio's Convolver), sound no worse than Valhalla VintageVerb / FabFilter
Pro-R, **have the same functions as those competitors**, page in the HamloComp style, AAA visuals.

## 1. Function parity

| Function | VintageVerb | Pro-R | HamloVerb | Notes |
|---|---|---|---|---|
| Space / algorithm choice | Reverb Mode (Room, Chamber, Hall, Plate, Cathedral, Ambience, …) | Space (character) | **Space**: Room, Chamber, Hall, Plate, Cathedral, Ambience (+ Classic = the M4 FDN, for old projects) | each space = its own early-reflection room, tank delays, diffusion, damping and modulation profile |
| Era / colour | Color 1970s / 1980s / Now | — | **Color**: 1970 / 1980 / NOW | tank bandwidth, modulation style, 1970 adds soft saturation in the loop |
| Pre-delay, tempo sync | yes | yes (sync) | **Pre-delay 0–500 ms**, **Free / Sync** (1/64 … 1/1) | host tempo, as the Delay module |
| Decay | 0.2–70 s | Decay rate | **Decay 0.1–30 s** | mid-band RT60 |
| Size | yes | Space size | **Size** | scales room, reflections and tank together |
| Attack (build-up) | Attack | — | **Attack** | slow swell of the late field |
| Early diffusion | yes | — | **Early diff** | |
| Late diffusion | yes | — | **Late diff** | in-loop allpasses, density growth |
| Modulation rate / depth | Mod Rate, Mod Depth | Character (chorus) | **Mod rate, Mod depth** | removes metallic ringing |
| Multiband decay | Bass Mult + Bass Xover, High Shelf + High Freq | Decay Rate EQ | **Bass ×, Bass Xover, High ×, High Xover** | drawn and draggable on the display as a decay-by-frequency curve |
| Post EQ / output filters | High Cut, Low Cut | Post EQ | **Low Cut, High Cut** (12 dB/oct) | HPEQ's own EQ covers anything finer |
| Early / late balance | — | Distance | **Distance** | near = strong, bright early field; far = late field dominates |
| Width | — (stereo) | Stereo width | **Width 0–150 %** | |
| Ducking | — | Ducking | **Duck** | wet ducks under the dry input |
| Freeze | — | Freeze | **Freeze** | lossless loop, input closed |
| Mix | Mix | Mix | module **Mix** (already in HPEQ, the send amount) | |

Not copied on purpose: Pro-R's 6-band free post EQ (HPEQ itself is the EQ; the reverb sits on an EQ band),
VintageVerb's 20 modes (6 spaces × 3 colours = 18 characters here).

## 2. Engine (written for HPEQ, D3)

```
in ─ pre-delay (0–500 ms, sync) ─┬─ early: image-source room taps (L/R mics) ─ early diffusion (4 allpasses) ─┐
                                 │                                                                            ├─ distance mix ─ width ─ low/high cut ─ duck ─ wet
                                 └─ attack taps (build-up) ─ input diffusion ─ 16-line FDN ───────────────────┘
                                                                           (Hadamard, per line: modulated read,
                                                                            late-diffusion allpass, 3-band decay,
                                                                            colour bandwidth, 1970 soft saturation)
```

- **Early reflections**: per space a shoebox room (dimensions below), source and two microphones,
  image sources up to order 3; the 24 strongest taps per microphone are kept (time, gain with
  1/r and wall absorption per order). Computed in `prepare()` for every space; Size scales the times.
- **Late field**: 16-line FDN, fast Hadamard mixing (lossless, orthogonal), delay lengths per space
  (mutually prime, spread to the space's mean free path). Each line: read with a sine LFO (own rate
  and phase, cubic interpolation when modulated, exact integer read when depth is 0), a Schroeder
  allpass (late diffusion), a three-band gain split by two one-pole low-passes at Bass Xover and High
  Xover: `g_L·low + g_M·mid + g_H·high` with `g = 10^(-3 d / (T60_band · fs))`. Equal gains rebuild
  the line exactly. Freeze sets all gains to 1 and closes the input.
- **Attack**: 8 taps per channel from the pre-delay line spread over 0–300 ms × size with a rising
  envelope (equal energy).
- **Spill-over, pools, tails, wet-only send, block-size independence, no audio-thread allocation**: as M4.
- **Classic** keeps the M4 code path bit-exact (projects saved before this pass have no `space` key).

| Space | Room (m) | Base tank delays | Character |
|---|---|---|---|
| Room | 6 × 4.5 × 3 | 9–28 ms | strong early field |
| Chamber | 9 × 6 × 4 (hard walls) | 14–41 ms | dense, bright |
| Hall | 32 × 22 × 13 | 28–83 ms | slow build, smooth |
| Plate | — (no early field) | 8–37 ms | instant density, bright |
| Cathedral | 60 × 28 × 30 (stone) | 41–127 ms | very slow build, deep modulation |
| Ambience | 8 × 6 × 3.5 | 6–19 ms | early field forward, short lines |

As in VintageVerb and Pro-R, choosing a space does not change Decay / Size: the space is the character.

## 3. Page (HamloVerb, Black Steel like HamloComp)

- Nameplate **HamloVerb · MODEL РВ-1 · № 0420 · MADE IN USSR**, amber LCD with space and decay.
- Left: big **SPACE** rotary selector (chicken-head pointer, 7 engraved positions), **COLOR** three-way
  chrome lever, **PRE-DELAY FREE / SYNC** lever, **FREEZE** lever, red **ON** lamp.
- Centre: **green phosphor CRT** (oscilloscope С1 style): graticule, bloom, scanlines, curved-glass glare.
  Mode lever **DECAY / TIME**:
  - DECAY: decay time by frequency (the 3-band curve from Bass ×/Xover, Decay, High ×/Xover) with two
    draggable handles, the output filters (low / high cut) as shaded regions, a fill that breathes with
    the live wet level and a phosphor sweep dot.
  - TIME: the impulse picture on a square-root time axis — early reflections of both microphones as
    spikes, the late envelope per band with the build-up, a LIVE level readout.
- Right: brass-bezel needle meter **OUTPUT** (wet level); ducking shows on the LCD.
- Knobs (brass, knurled, engraved scales): big row — PRE-DELAY, SIZE, DECAY, ATTACK, DISTANCE, WIDTH;
  small row — EARLY DIFF, LATE DIFF, MOD RATE, MOD DEPTH, BASS ×, BASS XOVER, HIGH ×, HIGH XOVER,
  LOW CUT, HIGH CUT, DUCK.

## 4. Measurements (thresholds fixed before the runs, see the pass report)

Classic bit-exact; mid-band RT60 per space ±15 %; low/high band RT60 follow Bass × / High × ±25 %;
echo density (normalised, Abel–Huang) ≥ 0.9 within 120 ms for Hall and Plate at full diffusion and
above Classic's; modulation makes the tail time-varying (first stated as "lowers the fixed coloration
of the late spectrum", changed after the first run, see the report §5); tail L/R correlation
< 0.3 at width 100 %; freeze holds energy ±1 dB over 10 s; pre-delay sync 1/8 at 120 BPM = 250 ms;
attack moves the energy centroid later; ducking; random-parameter fuzz finite; block sizes 1/64/127/512
identical; 0 audio-thread allocations; CPU per reverb reported.
