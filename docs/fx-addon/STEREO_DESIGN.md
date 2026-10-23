# HamloShirota — design note (module pass 8: Stereo)

Owner, 2026-10-23: "a real widener like Ozone Imager — really good; exactly mono when asked, and as if the
sound were behind or very wide". After the briefing: name **HamloShirota · MODEL SHR-2 · № 1957** (shirota =
width; 1957 = Sputnik), modes **EDINSTVO / PROSTOR / KOSMOS** with their real character written small under
the mode name (the owner asked for that; the same is done on the RedWall, Udarnik and Zastava pages); a node
in Mid / Side / Left / Right mode shows **OSHIBKA** on the vectorscope with the problem in small letters; the
mono guard (SAFE) is on for new modules. Monsta FX goes last.

## 1. Function set

| Function | Ozone Imager / others | HamloShirota |
|---|---|---|
| Width | Width per band | **WIDTH** 0…200 % (`width`, M6): 0 = exact mono, 100 % = unchanged; per band = the HPEQ region / point |
| Stereo from mono | Stereoize I (Haas) / II (decorrelation) | **STEREOIZE** 0…100 % (`stereoize`): side made from the mid through an all-pass network — the mono sum stays exactly the same |
| Behind / around | — (surround tools) | **SPACE** 0…100 % (`space`, KOSMOS): Blauert's directional bands + wider side |
| Mono bass | Ozone Mono Maker, bx_control | **MONO BASS** off / 20…500 Hz (`monoBassHz`) |
| Rotation, balance | Imager, Waves S1 | **ROTATION** ±45° (`rotation`), **BALANCE** (`balance`, M6) |
| Mono guard | Imager safe mode (correlation) | **SAFE** (`safe`): side never louder than mid ⇒ correlation ≥ 0 |
| Mono check | Imager mono | **MONO** lever (`monoListen`): L = R = the output's mid |
| Scope, correlation | Vectorscope, correlation meter | green CRT vectorscope, CORRELATION needle meter −1…+1 |
| Mix, output | yes | **MIX** = the slot mix, **OUTPUT** −24…+12 dB (`outputDb`) |
| Modes | — | **EDINSTVO** "M/S width" (unity) / **PROSTOR** "stereoize, mono-safe" (space) / **KOSMOS** "surround, behind" (`style` 0/1/2) |

Every new key absent = the M6 stereo law, bit-exact. Snapshot: every new value 0 = M6 (MONO BASS 0 = off).

## 2. Laws (per sample; needs two channels)

1. `M = (L + R) / 2`, `S = (L − R) / 2`.
2. MONO BASS f: `S ← HP⁸(S)`: four 2nd-order Butterworth high-passes at f (−49 dB at f/2, −0.07 dB at 4 f).
3. WIDTH w: `S ← w S`.
4. STEREOIZE a (PROSTOR, KOSMOS): `S ← S + a · D(M)`, D = a high-pass at 150 Hz (the bass stays mono) and a
   cascade of eight 2nd-order all-pass sections (Q 0.7, 270 Hz … 11 kHz, spread by octaves and 1/3): the
   same magnitude as M, the phase turned around many times. Only S changes, so **L + R = 2M exactly**: the
   mono sum is the input's mono sum, whatever the setting. With S uncorrelated with M and equal power the
   correlation of L and R is 0.
5. KOSMOS, space k: the whole signal gets Blauert's "behind" bands — bell +6k dB at 1 kHz (Q 1.2), bell
   +6k dB at 11 kHz (Q 1.2), bell −6k dB at 4.5 kHz (Q 0.9) (RBJ peaking filters on M and S alike) — and the
   side grows by `(1 + k)`.
6. SAFE: `corr(L, R) ∝ E[M²] − E[S²]` exactly (the cross terms cancel), so the guard keeps
   `E[S²] ≤ E[M²]`. M and S wait D = 2.5 ms (look-ahead, latency D, reported to the host); the side gain is
   `min(1, sqrt(ΣM² / ΣS²))` over the 5 ms window centred on the output sample, applied at once, recovering
   over 100 ms. (First design: 10 ms running means without look-ahead — changed after the first run, see the
   report.)
7. `L = M + S`, `R = M − S`; ROTATION θ: `[L; R] ← [[cos θ, −sin θ], [sin θ, cos θ]] [L; R]`; BALANCE (M6 law).
8. MONO lever: `L = R = (L + R) / 2`. OUTPUT `10^(O/20)` last.
9. One channel (Mid / Side / Left / Right node, mono input): the signal passes untouched and the page shows
   OSHIBKA.

Latency 0 (IIR only); with SAFE on, D = 2.5 ms. Meters: output correlation (300 ms), guard reduction (dB), the last 256 output
points (L, R) for the vectorscope, the one-channel flag.

## 3. Page (Black Steel, same style)

Nameplate "HamloShirota · MODEL SHR-2 · № 1957"; amber LCDs CORRELATION and WIDTH (guard reduction shown when
SAFE acts). Big knobs WIDTH, STEREOIZE, SPACE, MONO BASS, ROTATION and the mode knob titled with the mode and
its character written small below; small knobs BALANCE, MIX, OUTPUT; levers SAFE and MONO. Green CRT:
vectorscope (mid up, side across), OSHIBKA + the problem in small letters when the node is not stereo.
CORRELATION needle meter (−1 … +1), ON lamp and toggle, scratched pictures. 1180 × 780.

## 4. Measurements (thresholds fixed before the runs)

The briefing said "side below 60 Hz ≤ −60 dB" for MONO BASS 120 Hz; the design (8th order, no latency)
gives −49 dB at f/2, so the threshold is set here, before any run, to −40 dB.

| # | Test | Pass |
|---|---|---|
| S1 | keys absent = M6 bit-exact; WIDTH 0 → L = R bit-exact | exact |
| S2 | WIDTH 200 %: side +6.02 ± 0.01 dB, mid unchanged (≤ 1e−6) | pass |
| S3 | STEREOIZE 100 % on a mono source (drums): correlation ≤ 0.5; PROSTOR: mono sum = input mono sum (≤ −120 dB); KOSMOS: mono sum = input mono sum through the space bands (≤ −120 dB) | pass |
| S4 | KOSMOS space 100 %: measured M response at 1 k / 4.5 k / 11 kHz = the formula ± 0.1 dB; correlation < 0 with SAFE off | pass |
| S5 | MONO BASS 120 Hz: side at 60 Hz ≤ −40 dB, at 480 Hz within ± 0.2 dB; mid unchanged | pass |
| S6 | SAFE on, WIDTH 200 % + STEREOIZE + KOSMOS on drums and on noise: every 50 ms window's correlation ≥ −0.02 | pass |
| S7 | ROTATION 90° composed of ±45°: rotation 30° = the matrix (≤ 1e−6); MONO lever: L = R = (L + R)/2 of the normal output | pass |
| S8 | Correlation meter = independent 300 ms computation ± 0.02; one channel: passes untouched, flag set | pass |
| S9 | Latency 0 (SAFE: D, reported = measured); blocks 1 / 64 / 512 identical; 0 allocations; OUTPUT ±6 dB exact | pass |
| S10 | Page in the real editor (older stereo shows M6 values; OSHIBKA for a Mid node; knobs edit); T11 through the VST3; CPU and memory reported | pass |
