# HamloUdarnik v2 engine (2026-10-27 .. 28)

Design and targets: [docs/UDARNIK_V2.md](../UDARNIK_V2.md), fixed before the baseline. Measured by `HamloUdarnik_Lab` on the
owner's clips (kick / snare / hats / perc / drum bus, active RMS -18 dBFS): [before](v2/before.md) = engine CLASSIC (the
HPEQ laws, bit-identical to HamloUdarnik 0.1.0), [after](v2/after.md) = engine V2. Both runs: the same, corrected tool.

**Checks passing: before 7 / 25, after 24 / 25.**

| # | Check | Before (CLASSIC) | After (V2) | Target |
|---|---|---|---|---|
| U1 | SNAP +12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 3.0 / 3.3 / 3.0 / 3.3 / 3.2 dB (no) | 8.1 / 12.0 / 12.0 / 10.8 / 12.0 dB | >= +8 dB |
| U1 | SNAP +12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 4.4 / 8.0 / 14.6 / 15.1 / 7.5 dB (no) | 8.9 / 11.9 / 12.3 / 10.8 / 12.4 dB | >= +8 dB |
| U1 | SNAP +12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 8.0 / 12.0 / 10.9 / 11.6 / 7.4 dB (no) | 8.1 / 12.0 / 12.0 / 10.8 / 12.0 dB | >= +8 dB |
| U2 | SNAP +6 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 1.7 / 1.9 / 1.8 / 2.0 / 1.8 dB (no) | 4.0 / 6.0 / 6.0 / 5.6 / 6.0 dB | >= +4 dB |
| U2 | SNAP +6 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 3.6 / 5.8 / 10.3 / 11.1 / 4.6 dB (no) | 4.5 / 6.0 / 6.3 / 5.7 / 6.2 dB | >= +4 dB |
| U2 | SNAP +6 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 4.0 / 6.0 / 5.5 / 5.8 / 3.7 dB (no) | 4.0 / 6.0 / 6.0 / 5.6 / 6.0 dB | >= +4 dB |
| U3 | SNAP -12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | -4.5 / -6.3 / -6.0 / -6.5 / -5.3 dB (no) | -8.0 / -12.0 / -12.0 / -11.8 / -12.0 dB | <= -8 dB |
| U3 | SNAP -12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | -9.7 / -13.9 / -13.2 / -14.1 / -8.0 dB | -8.0 / -12.0 / -12.0 / -11.8 / -12.0 dB | <= -8 dB |
| U3 | SNAP -12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | -8.0 / -12.0 / -10.9 / -11.6 / -7.2 dB (no) | -8.0 / -12.0 / -12.0 / -11.8 / -12.0 dB | <= -8 dB |
| U4 | BODY -12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | -0.1 / -0.1 / -0.2 / -0.1 / -0.0 dB (no) | -8.1 / -12.0 / -12.0 / -10.8 / -12.0 dB | <= -8 dB |
| U4 | BODY -12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | -0.2 / -0.1 / -0.5 / -0.3 / -0.0 dB (no) | -8.1 / -12.0 / -12.0 / -10.8 / -12.0 dB | <= -8 dB |
| U4 | BODY -12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | -0.2 / -0.1 / -0.5 / -0.3 / -0.0 dB (no) | -8.1 / -12.0 / -12.0 / -10.8 / -12.0 dB | <= -8 dB |
| U4 | BODY +12 dB, PRAVDA: contrast kick / snare / hats / perc / bus | 0.7 / 0.4 / 0.9 / 0.3 / 0.0 dB (no) | 8.0 / 12.0 / 12.0 / 11.8 / 12.0 dB | >= +8 dB |
| U4 | BODY +12 dB, UDARNIK: contrast kick / snare / hats / perc / bus | 0.9 / 0.5 / 1.2 / 0.8 / 0.0 dB (no) | 8.0 / 12.0 / 12.0 / 11.8 / 12.0 dB | >= +8 dB |
| U4 | BODY +12 dB, PERESTROIKA: contrast kick / snare / hats / perc / bus | 1.0 / 0.5 / 1.2 / 0.9 / 0.0 dB (no) | 8.0 / 12.0 / 12.0 / 11.8 / 12.0 dB | >= +8 dB |
| U5 | Level independence: drum bus at -6 vs -30 dBFS RMS, SNAP +9 (PRAVDA): contrast | 2.58 vs 2.58 dB | 9.00 vs 9.00 dB | within 1 dB |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, PRAVDA: level change; residual | 2.28 dB; -10.4 dB (no) | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, UDARNIK: level change; residual | 0.00 dB; -333.7 dB | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB |
| U6 | Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, PERESTROIKA: level change; residual | 0.00 dB; -333.7 dB | 0.00 dB; -333.7 dB | <= 0.5 dB; <= -50 dB |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, PRAVDA: output peak | 11.00 dBFS (no) | 0.00 dBFS | <= 0 dBFS |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, UDARNIK: output peak | -2.20 dBFS | -1.00 dBFS | <= 0 dBFS |
| U7 | SNAP +12 on the drum bus at -1 dBFS peak, protection on, PERESTROIKA: output peak | 11.00 dBFS (no) | 0.00 dBFS | <= 0 dBFS |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, PRAVDA: output - input - latency | 0.000 ms | 0.000 ms | within 0.1 ms |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, UDARNIK: output - input - latency | -1.312 ms (no) | -1.167 ms (no) | within 0.1 ms |
| U8 | Onset (first sample at 25 % of the peak), SNAP +12, PERESTROIKA: output - input - latency | 0.000 ms | 0.000 ms | within 0.1 ms |

Values are the mean attack (U1 .. U3) or body (U4) contrast over the hits, dB, kick / snare / hats / perc / drum bus.

## What V2 is (`Source/DSP/UdarnikV2.*`, every law in the header)

- **Hits** are found by the CLASSIC core's fast onset detector (peak envelopes 0.05 / 1 ms vs 1 / 10 ms, ratio over
  6 + 18 (1 - SENS) + 6 (1 - PRESERVE) dB): it also sees an attack riding on the previous hit's tail - this is what fixed
  the drum bus (SNAP +12: 3.2 -> 12.0 dB).
- **Weights from the hit's phases**, not from envelope ratios that rarely reach 1: attack window 15 ms x SPEED (held
  open while the attack envelope still rises), body to the next hit or 500 ms x SPEED. So SNAP / BODY deliver about
  their value; steady tones never start a hit and pass bit-exactly.
- **UDARNIK** is peak-neutral: the attack's boost is soft-limited under the hit's own sample peak, and what the attack
  could not get there the body gives back, so the contrast still follows SNAP (kick: 6.1 -> 8.9 dB) while the peak
  does not rise. **PERESTROIKA**: RISE = how long the attack lasts, DECAY divides the decay's slope.
- **PROTECT** (plug-in): only what the shaper added over the input sample and over -3 dBFS is soft-limited into the
  room up to 0 dBFS; without a boost the output is the input exactly.
- **ENGINE** CLASSIC / V2 and PROTECT on the page's bottom strip ([panel.png](v2/panel.png)); new instances V2 + PROTECT;
  a state without `engine` loads CLASSIC without PROTECT; HPEQ's 6 presets stay CLASSIC; **12 V2 presets** added (Kick
  Punch V2, Kick Long Boom, Snare Crack V2, Snare Fat Body, Tight Drums, Room Killer, Drum Bus Punch V2, Soft Hits, Hats
  Shorter, Long Decay Bloom, Snappy Perc, Lo-Fi Dull Attack): on the test loop at -1 dBFS peak all stay <= 0 dBFS.

## Not met

- **U8, UDARNIK**: the 25 %-of-peak crossing comes 1.17 ms early (CLASSIC UDARNIK: 1.31 ms). A peak-neutral attack is
  denser - its low samples rise more than its peak - so the crossing moves; the first sample does not move. Removing the
  detector lead (2 -> 0.5 ms) did not change it and cost the kick (reverted).

## Disclosed

1. Lab bugs found and fixed, the "before" re-run with the fixed tool: U5 read the latency before the render (argument
   order); the hit detector's floor was absolute (now -45 dB re the clip's peak); U1 .. U4 / U8 were measured with
   PROTECT on (now off: the protection is U7); the body window stops before a second attack riding on a hit's tail.
   With the fixed tool CLASSIC passes U5.
2. Tuned after measuring (the targets did not change): attack window 10 -> 12 -> 15 ms, detector lead 1 -> 2 ms, slow
   attack 15 -> 6 ms; the hit detector replaced by the CLASSIC core's (from 9 / 25 to 22 / 25); UDARNIK's fixed body
   give-back (SNAP / 2) replaced by the measured shortfall (22 -> 24 / 25).
3. Tried and dropped: a 1 s loudness hold for UDARNIK (moved steady tones and the onset); a 5 ms cap on the attack
   window's rise extension (bus 3.2 -> 0.5 dB); lead 0.5 ms (kick under 8 dB).
4. A V2 test first compared a steady tone from 0.3 s, inside the body of the tone's own start; now from 0.7 s.

## Checks

| Check | Result |
|---|---|
| `HamloUdarnik_Tests` | 243 / 243 |
| `HamloUdarnik_Reference` (CLASSIC vs HPEQ's HamloUdarnik.vst3) | 168 cases, 104 bit-identical, worst -133.3 dB |
| pluginval v1.0.4, strictness 5 .. 10 (10 with Steinberg's validator) | SUCCESS |
| Steinberg validator `-e` | 537 passed, 0 failed |

Listening in FL Studio: the owner's.
