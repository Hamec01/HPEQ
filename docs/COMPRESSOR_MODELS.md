# HamloComp models and sources: design and targets

Owner (2026-10-27): "it is nothing among other compressors, no character, you do not feel it at all"; add
character and switches for the source (drums, samples, vocals). SAMPLES = everything that is not drums or
voice: melodies, keys, pads, chops. Plain names (VCA, FET, OPTO, TUBE), no Soviet ones.

Targets below are fixed **before** the implementation and the "after" runs (RULES: measure before tuning,
the same test before and after, numbers). They come from the published behaviour of the classic units; the
plug-in does not copy any circuit, it reproduces the behaviour that defines each type.

## 1. What makes a compressor type audible

A type is three things: the **ballistics** (how attack and release react to the material), the **detector**
(what it listens to) and the **colour** (the harmonics the gain element adds while it works).

| Type | Reference units | Published behaviour used here |
|---|---|---|
| VCA | SSL G bus, dbx 160 | attack 0.1 / 0.3 / 1 / 3 / 10 / 30 ms, release 0.1 / 0.3 / 0.6 / 1.2 s + program-dependent Auto, ratio 2 / 4 / 10 [1]; clean gain element |
| FET | UREI / UA 1176LN | attack 20-800 us, release 50 ms - 1.1 s, THD < 0.5 % while limiting (more when driven: "all buttons") [2] |
| Opto | Teletronix LA-2A (T4 cell) | attack about 10 ms; release about 60 ms for the first 50 %, then 1 - 15 s for the rest, slower the longer and harder the signal was over the threshold [3] |
| Vari-mu (tube) | Fairchild 670, Manley | ratio grows with the level (very soft knee); release program-dependent, e.g. 0.3 s for single peaks, 10 s for repeated peaks, 25 s for a constantly high level [4]; tube colour (mostly 2nd harmonic) |

## 2. Controls

- **MODEL** (new parameter `model`): CLASSIC (today's compressor, Clean / Punch / Glue via CHARACTER,
  bit-identical), VCA, FET, OPTO, TUBE. A new instance starts on **VCA**; a state saved before this change
  (no `model` in it) loads as CLASSIC, so old projects sound the same.
- **SOURCE** (new parameter `source`): ALL (no detector shaping, as today), DRUMS, SAMPLES, VOC. It shapes only
  what the detector hears, never the audio path.
- THRESHOLD, RATIO, KNEE, ATTACK, RELEASE, MAKEUP, MIX, DETECTOR, AUTO MAKEUP keep their meaning. CHARACTER
  only acts in CLASSIC.

### Models

| Model | Detector | Static curve | Ballistics | Colour |
|---|---|---|---|---|
| VCA | as DETECTOR (peak / RMS), feed-forward | the law as set | attack as set; **auto release**: a fast and a slow release run in parallel, the larger reduction wins, so short peaks recover with RELEASE, sustained compression with ~ 5 x RELEASE (SSL Auto) | clean (no added harmonics) |
| FET | **feedback**: the detector listens to the output, as the 1176 | the law as set (feedback softens it, as on the unit) | attack and release as set; release is faster from deep reduction (a FET recovers non-linearly) | odd + even harmonics growing with the reduction (class-A FET stage) |
| OPTO | RMS-like (the lamp integrates) | soft: the knee is at least 12 dB | **two-stage release**: a fast stage (RELEASE, ~ 60 ms) recovers about half, a slow stage (1 - 15 s) the rest; the slow stage charges while the reduction is held, so long compression releases slowly | gentle 2nd harmonic of the tube make-up stage |
| TUBE | RMS-like | **vari-mu**: the ratio rises with the level over the threshold (1 : 1 at the threshold towards RATIO 18 dB above), knee at least 18 dB | attack as set; release program-dependent: single peaks recover with RELEASE, repeated / long reduction with ~ 8 x RELEASE | 2nd harmonic (asymmetric) growing with the level and the reduction |

### Sources (detector shaping)

| Source | Detector hears | Why |
|---|---|---|
| ALL | the signal | as today |
| DRUMS | high-pass 2nd order at 100 Hz | the kick does not pull the whole kit down (no pumping on hats and snare); peak detection is kept so the hits are caught |
| SAMPLES | high-pass at 50 Hz, a 30 ms RMS blended in | melodic parts: smooth, follows the phrase, not each note's attack |
| VOC | high-pass at 120 Hz + a +4 dB bell at 3 kHz | plosives and room rumble do not trigger it; the loud, harsh syllables are caught first |

## 3. Targets (fixed before the runs)

Measured by `HamloComp_Lab` at 48 kHz. GR = gain reduction.

| # | Check | Target |
|---|---|---|
| T1 | CLASSIC (Clean / Punch / Glue) | bit-identical to before (reference check stays at -140 dB vs HPEQ) |
| T2 | VCA static curve (1 kHz, peak, threshold -30, 4 : 1, knee 0) at -10 dBFS | within 0.5 dB of the law |
| T3 | VCA auto release: GR left 300 ms after a 3 s burst vs after a 30 ms burst (release 100 ms) | long >= 2 x short |
| T4 | VCA colour at 6 dB GR (1 kHz) | THD <= 0.05 % |
| T5 | FET attack 0.1 ms: time to 90 % of the final GR after a +20 dB step | <= 2 ms |
| T6 | FET colour at 6 dB GR | 0.3 % <= THD <= 3 %; H2 and H3 both >= -70 dB |
| T7 | OPTO release after a 30 ms burst (release 60 ms): time to recover 50 % of the GR | 40 - 120 ms |
| T8 | OPTO release after a 3 s burst: time to recover 90 % | >= 1 s and >= 4 x the 90 % time after a 30 ms burst |
| T9 | OPTO attack (attack 10 ms): time to 63 % of the final GR | 5 - 20 ms |
| T10 | TUBE effective ratio (slope of the static curve) 4 dB over the threshold vs 20 dB over (ratio 8) | low <= 0.6 x high |
| T11 | TUBE colour at 6 dB GR | H2 >= H3 + 6 dB; 0.2 % <= THD <= 3 % |
| T12 | DRUMS: GR from a 50 Hz sine vs a 1 kHz sine of the same level | 50 Hz >= 8 dB less |
| T13 | VOC: GR from 3 kHz vs 1 kHz (same level); from 80 Hz vs 1 kHz | 3 kHz >= 2 dB more; 80 Hz >= 6 dB less |
| T14 | SAMPLES: GR from 30 Hz vs 1 kHz | 30 Hz >= 4 dB less |
| T15 | Every model and source: block sizes 1 / 64 / 127 / 512 | bit-identical |
| T16 | Every model and source, below the knee with 0 dB makeup | output = input within -100 dB (the colour stages are identities without reduction) |
| T17 | pluginval 10 and the Steinberg validator | pass |

The "before" column is today's compressor (CLASSIC) on the same checks: it shows what is missing.

## Sources

1. SSL G bus: [MusicRadar, UAD SSL 4000 G review](https://www.musicradar.com/reviews/universal-audio-ssl-4000-g-bus-compressor), [UAD manual](https://help.uaudio.com/hc/en-us/articles/30847649785748-SSL-4000-G-Bus-Compressor-Manual)
2. 1176LN: [Wikipedia, 1176 Peak Limiter](https://en.wikipedia.org/wiki/1176_Peak_Limiter), [Universal Audio 1176LN spec (Markertek)](https://www.markertek.com/product/ua-1176ln/universal-audio-1176ln-classic-limiting-amplifier)
3. LA-2A: [UA help: LA-2A](https://help.uaudio.com/hc/en-us/articles/215779663), [Sound On Sound, Warm Audio WA-2A](https://www.soundonsound.com/reviews/warm-audio-wa-2a)
4. Fairchild 670: [Thomann Fairchild 670](https://www.thomann.co.uk/fairchild_670.htm), [Vintage King](https://vintageking.com/blog/fairchild-recording-equipment-670/)

## 4. Factory presets on their material (presets pass, 2026-10-27; fixed before the runs)

Every preset names the material it is for (drums, kick, snare, hats, vocal, backing, bass, keys, guitar, synth,
mix) and a depth: GENTLE, MEDIUM or HEAVY. `HamloComp_Lab presets <material dir>` plays each preset on its
material - the owner's clips of material set 2 (HPEQ), each gained so its active RMS (blocks above -50 dBFS) is
**-18 dBFS** (0 VU, the usual gain staging), mixes **-14 dBFS** - so a threshold means the same for every
preset. For the new presets the threshold is the one result of a measurement: it is set so the average gain
reduction hits the middle of the depth's band (GENTLE 2, MEDIUM 4.5, HEAVY 9 dB; rounded to 0.5 dB); all other
settings are designed, not tuned.

| # | Check (every preset, on its material) | Target |
|---|---|---|
| P1 | output finite | yes |
| P2 | average gain reduction (the block meter, over blocks above -50 dBFS) | GENTLE 1 .. 3, MEDIUM 3 .. 6, HEAVY 6 .. 12 dB |
| P3 | sample peak out - in | <= +6 dB |
| P4 | active RMS out - in | -12 .. +6 dB |
| P5 | no two presets with the same settings | all differ |

The 16 presets made before this pass are measured the same way and reported; they are not changed.
