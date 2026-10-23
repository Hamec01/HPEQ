# HamloRedWall — design note (module pass 5: Limiter) and whole-layer modules (D7)

Owner, 2026-10-23: a module must work in three places — **on a point, on a range, on the whole mix
(of the layer)**; today the "+" menu offers only 4 quick items (2 FX). Limiter: Black Steel page named
**HamloRedWall**, styles with Soviet names written in English (owner's examples: Perestroika, Collectivism,
October Revolution — "take others if they fit the meaning").

## 1. Whole-layer modules (DECISIONS D7)

Each layer gets a **MIX chain** of 0–6 modules that process the whole layer signal, like inserts on a
mixer channel. Order inside a layer:

```
x -> PRE-EQ -> frequency nodes (regions and points, H3 tree) -> MIX chain -> layer controls (gain, width, pan)
```

The MIX chain runs on one engine node outside the 8 frequency nodes (it does not take a band). Its
latency adds to the layer's (`L_layer = L_nodes + L_mix`); the dry path is aligned. Inserts replace the
signal (`y = D(x) + mix (P(x) - D(x))`), Reverb / Delay stay wet-only sends. The node list shows a "MIX
(whole layer)" row; the "+" menu and the workspace menu get full submenus: EQ point (every shape), FX
region (every module), **module on the whole layer** (every module). The state grows additively
(`MixChain` with `FxSlot` children); projects without it are unchanged.

## 2. HamloRedWall — function set

| Function | Pro-L 2 / L2 / Ableton | HamloRedWall |
|---|---|---|
| Push into the limiter | Gain / Threshold | **THRESHOLD** 0 … −30 dB: the input is raised by −threshold, the output stays at the ceiling (L2) |
| Output ceiling | yes | **CEILING** −30 … 0 dB, sample peak guaranteed (M6) |
| True peak | yes | **TRUE PEAK**: the detector also sees 4× interpolated inter-sample peaks |
| Release | manual + auto (ARC) | **RELEASE** 1 … 1000 ms + **AUTO**: program-dependent (short peaks recover in 0.25×, long reduction in 6×) |
| Lookahead | yes | **LOOKAHEAD** 0.1 … 10 ms (latency, PDC) |
| Styles | Transparent / Punchy / Aggressive … | **GLASNOST** (clean, the M6 law), **STAKHANOV** (punch), **OCTOBER** (loud) |
| Channel link | transients / release | **LINK** 0 … 100 % |
| Monitor | Unity gain, Audition limiting | **MONITOR**: NORMAL / UNITY (level-matched by the threshold) / DELTA (only what is removed) |
| Display | GR history, meters | green CRT with the scrolling GR history, GR needle meter, amber LCD (GR, output peak) |

## 3. Laws (all options off = the M6 limiter, bit-exact)

M6: target `t = min(1, c / |x|)`, sliding minimum of t over L + 1 samples, release one-pole on rising
gain, moving average over L, audio delayed by L; every value in the average is ≤ the target of the sample
being output, so the output sample peak never exceeds c.

- THRESHOLD: `x' = x · 10^(−T/20)` before the detector and the audio path.
- TRUE PEAK: the level is `max(|x|, max over 3 interpolated points between x[n−1] and x[n])` (16-tap
  polyphase windowed sinc, the loudness meter's interpolator); the guarantee holds for the samples, and the
  inter-sample peaks of the output are measured in the tests.
- AUTO release: `h` = one-pole (0.3 s) of [reduction > 1 dB]; the release coefficient is interpolated between
  those of 0.25 × and 6 × the release time by h. (First version: 1 s and 3 ×; it missed criterion 5 —
  see the pass report.)
- STAKHANOV: the moving average runs over L/4 instead of L (still ≥ 1 sample): the gain starts to fall
  only in the last quarter of the lookahead before a peak, so less of the transient's lead-in is ducked,
  and the release is halved. The guarantee is kept (a shorter window ⊂ the minimum window).
- OCTOBER: soft clip before the limiter, `x'' = s · tanh(x' / s)` with `s = 1.25 c` (for |x'| < 0.3 s the
  shape is within 3 % of linear): peaks are rounded, the limiter has less to do, the result is louder at
  the same ceiling, with harmonics.
- LINK `λ`: per-channel targets `t_ch`; `t'_ch = λ · min(t_L, t_R) + (1 − λ) · t_ch`; λ = 1 is the M6 linked law.
- MONITOR UNITY: output · 10^(T/20) (back to the input level); DELTA: `D_L(x') − y` (what the limiter
  removed, at the pushed level).

## 4. Page (HamloRedWall, Black Steel)

Nameplate **HamloRedWall · MODEL OGR-1 · № 1917** (OGR = ограничитель), amber LCDs for GR and for the
output peak. Brass knobs: THRESHOLD, CEILING, RELEASE, LOOKAHEAD, LINK, and a STYLE knob with three
detents GLASNOST / STAKHANOV / OCTOBER. Toggles: TRUE PEAK, AUTO RELEASE, MONITOR (3 positions). Green CRT
with the scrolling GR history (last ~4 s); GAIN REDUCTION needle meter; ON lamp and toggle.

## 5. Measurements (thresholds fixed before the runs)

1. All options off = M6, bit-exact; M6 tests unchanged.
2. Ceiling: output sample peak ≤ ceiling in every style, link, release, threshold (drums +18 dB).
3. True peak: with TRUE PEAK on, output inter-sample peaks (8× independent interpolator) ≤ ceiling + 0.3 dB;
   off: the same signal shows an overshoot > 0.5 dB (the option must matter).
4. Threshold −6 dB on drums: output loudness (integrated LUFS) at least +3 LU over the input at the same
   ceiling.
5. AUTO release: on a bass line, the gain-reduction modulation (std of GR over 1 s windows) lower than with
   the fixed release of the same time, by ≥ 20 %.
6. STAKHANOV: the samples ducked > 0.5 dB before a single transient are ≤ 30 % of GLASNOST's.
7. OCTOBER: integrated loudness ≥ GLASNOST + 1 LU at the same settings; 3rd harmonic higher.
8. LINK 0 on a left-only burst leaves the right channel untouched (bit-exact delayed); LINK 1 ducks both.
9. MONITOR: UNITY output level within 0.5 dB of the input for small signals; DELTA = delayed input − output.
10. Latency reported = measured; block sizes identical; 0 allocations; CPU.
11. Whole-layer chain: a compressor on the MIX chain = the same compressor on the plain layer signal
    (bit-exact vs an independent render); latency of the layer = nodes + mix; state round trip; the "+"
    menu lists every module in all three places.
