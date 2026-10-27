# HamloUdarnik v2 engine - work in progress (2026-10-27)

Design and targets: [docs/UDARNIK_V2.md](../UDARNIK_V2.md) (fixed before the baseline). Measured by `HamloUdarnik_Lab`
on the owner's clips (kick / snare / hats / perc / drum bus at -18 dBFS RMS): [before](v2/before.md) (engine CLASSIC =
the HPEQ laws), [after](v2/after.md) (engine V2). Both runs use the same, corrected tool (see "Disclosed").

## What is in

- `Source/DSP/UdarnikV2.*`: a differential-envelope shaper (SPL principle): hits found by the attack envelope
  standing out of the slow one (level independent); weights from the hit's phases (attack window 12 ms x SPEED, held
  open while the attack still rises; body until the next hit / 500 ms x SPEED), so the knobs reach their value;
  UDARNIK peak-neutral (the attack's boost soft-limited under the hit's own peak, the body gives back SNAP / 2);
  PERESTROIKA: RISE = attack length, DECAY divides the decay's slope; look-ahead compensates the detector's lag.
- Plug-in: `engine` (CLASSIC / V2, new instances V2) and `protect` (soft-limits only what the shaper added over the
  input above -3 dBFS, never over 0 dBFS); a state without `engine` loads as CLASSIC without protection; HPEQ's 6
  presets stay CLASSIC. Page 1180 x 820 with the ENGINE / PROTECT strip ([panel.png](v2/panel.png)).

## Before / after (attack or body contrast, dB; kick / snare / hats / perc / drum bus)

| Check | Before (CLASSIC) | After (V2) |
|---|---|---|
| U1 SNAP +12, PRAVDA | 3.0 / 3.3 / 3.0 / 3.3 / 3.2 | **8.0 / 11.8 / 10.9 / 10.7** / 3.2 |
| U1 SNAP +12, UDARNIK | 4.4 / 8.0 / 14.6 / 15.1 / 7.5 | 6.1 / 9.2 / 10.6 / 10.4 / 4.2 |
| U1 SNAP +12, PERESTROIKA | 8.0 / 12.0 / 10.9 / 11.6 / 7.4 | 8.0 / 11.8 / 10.9 / 10.7 / 3.2 |
| U4 BODY -12 (every mode, about) | -0.1 .. -0.5 on all | **-8.1 / -11.9 / -11.0 / -10.8** / -3.1 |
| U4 BODY +12 (every mode, about) | 0.0 .. 1.2 on all | **8.0 / 11.8 / 10.5 / 10.4** / 6.9 |
| U6 steady tone, PRAVDA | +2.28 dB, residual -10.4 dB | 0.00 dB, identity |
| U7 output peak with PROTECT, PRAVDA / PERESTROIKA | +11.0 dBFS | 0.00 dBFS |
| U8 onset, UDARNIK | -1.31 ms | -1.17 ms |
| checks passing | 7 / 25 | 9 / 25 |

## Open (next session)

1. **The drum bus**: V2's SNAP in UDARNIK / PERESTROIKA is weaker there than the CLASSIC core's event detection
   (3.2 .. 4.2 vs 7.4 .. 7.5 dB); BODY reaches -3.1 / +6.9 dB. Capping the attack window's rise extension at 5 ms made
   it worse (0.5 dB) and was reverted. Next: detect hits by the CLASSIC core's e1 / e10 onsets in V2.
2. Kick: SNAP -12 gives -7.9 dB (target -8), UDARNIK SNAP +12 6.1 dB.
3. U8 UDARNIK: the denser attack moves the 25 %-of-peak crossing 1.2 ms early (as CLASSIC UDARNIK did).
4. Not run on V2 yet: pluginval, Steinberg's validator; `dist/` still holds the M0 build; no V2 presets yet.

## Disclosed

1. Lab bugs found and fixed, then the "before" re-run with the fixed tool: U5 read the latency before the render
   (C++ argument order); the hit detector used an absolute -45 dBFS floor (now relative to the clip's peak); U1 .. U4
   and U8 were measured with PROTECT on (now off: the protection is its own check, U7); the body window now stops
   before any second attack riding on a hit's tail. With the fixed tool CLASSIC passes U5 (it was the lab's bug).
2. Tuned after measuring: the attack window 10 -> 12 ms, the detector lead 1 -> 2 ms, the slow attack 15 -> 6 ms.
3. UDARNIK: a 1 s loudness hold was tried and dropped (it moved steady tones and the onset); peak-neutral instead.
4. A V2 test first compared a steady tone from 0.3 s, inside the body of the tone's own start; now from 0.7 s.

## Checks now

`HamloUdarnik_Tests` 225 / 225; `HamloUdarnik_Reference` (CLASSIC vs HPEQ): 168 cases, 104 bit-identical, worst -133.3 dB.
