# HamloZastava — design note (module pass 7: Gate / Expander)

Owner, 2026-10-23, after the briefing: "design the same as before, one style", go. Accepted: **HamloZastava ·
MODEL KPP-1 · № 1945** (zastava = frontier post, KPP = checkpoint: friends pass, strangers do not), modes
**ZASTAVA / PATRUL / TREVOGA**, look-ahead SPUTNIK / VOSTOK / SOYUZ as on HamloUdarnik. A key taken from
another band of the layer (bass gated by the kick band) is left for later: it needs links between engine
nodes.

## 1. Function set

| Function | Pro-G / DS201 / Ableton Gate | HamloZastava |
|---|---|---|
| Threshold, range, attack, hold, release | yes | THRESHOLD, RANGE, ATTACK, HOLD, RELEASE (M6 keys) |
| Return / hysteresis | Ableton Return, Pro-G | HYSTERESIS (M6) |
| Expander ratio | Pro-G Expand | RATIO (M6) |
| Look-ahead | Pro-G, Ableton | **LOOK-AHEAD** SPUTNIK 0 / VOSTOK 2 / SOYUZ 5 ms (`lookahead` 0/1/2), latency reported |
| Key filter | DS201 HP/LP, Pro-G | **KEY HP** 20 Hz (off) … 10 kHz, **KEY LP** 200 Hz … 20 kHz (off) (`keyHpHz`, `keyLpHz`) |
| Key listen | DS201, Pro-G | **KEY LISTEN** (`keyListen`) |
| Modes | Classic / Expand / Duck | **ZASTAVA** gate (the M6 law) / **PATRUL** soft expander / **TREVOGA** duck (`style` 0/1/2) |
| Flip | Ableton Flip | **FLIP** (`flip`): what the gate would remove |
| Mix, output | yes | **MIX** = the slot mix; **OUTPUT** −24…+12 dB (`outputDb`) |

Every new key absent = the M6 gate, bit-exact. Snapshot: KEY HP / LP are stored as 0 when off, so a
zero-filled parameter block is the M6 gate. A gate added from the UI starts with VOSTOK; everything else as
the M6 defaults.

## 2. Laws (detector time n, audio time m = n − L, L = 0 / 2 / 5 ms)

- **Key**: per channel `k_c = LP(HP(x_c))`, 2nd-order Butterworth (bilinear) high-pass and low-pass, each
  bypassed when off. Level ℓ = max_c |k_c|.
- **Detector and state** (the M6 law on the key): peak detector `D = max(ℓ, D · e^{−1/(10 ms · fs)})`,
  `x = 20 log10 D`; open when `x > T` (hold reloaded), closed when `x < T − hysteresis` and the hold is over.
- **Target attenuation** (dB ≥ 0):
  - ZASTAVA: `open ? 0 : min(range, (T − x)(ratio − 1))` (M6).
  - PATRUL: `clamp((T − x)(ratio − 1), 0, range)` — no open / closed state, no hold: a continuous downward
    expander.
  - TREVOGA (duck): `open ? range : 0` — the key above the threshold pushes the signal down by RANGE.
- **Smoothing**: one-pole in dB, ATTACK while the gate opens (attenuation falls; TREVOGA: while it ducks),
  RELEASE otherwise.
- **Look-ahead**: the attenuation applied to audio sample m is the minimum (TREVOGA: maximum) of the
  detector attenuations over [m, m + L]: the gate opens (ducks) L earlier and closes (comes back) on time.
- **Output**: `g = 10^(−A/20)`; normal `y = x_d g`; FLIP `y = x_d − x_d g` (gate + flip = input); KEY LISTEN
  `y = k_d` (the key, delayed by L); OUTPUT `10^(O/20)` last.
- **Meters**: attenuation (block maximum and current), key peak, open state, number of openings.

Latency = L. Memory: rings of L for the audio and the key and a deque of L + 1 per slot (prepare).

## 3. Page (HamloZastava, Black Steel — the same style as the other pages)

Nameplate "HamloZastava · MODEL KPP-1 · № 1945"; amber LCDs KEY (key peak dB) and STATE (OPEN / SHUT and
attenuation). Big brass knobs THRESHOLD, RANGE, ATTACK, HOLD, RELEASE and the mode knob titled with the mode;
small knobs RATIO, HYSTERESIS, KEY HP, KEY LP, MIX, OUTPUT. Levers LOOK-AHEAD, KEY LISTEN, FLIP. Green CRT:
key level of the last 4 s, the threshold line, closed stretches shaded. ATTENUATION needle meter, ON lamp and
toggle, 2–3 scratched pictures. Design coordinates 1180 × 760.

## 4. Measurements (thresholds fixed before the runs)

| # | Test | Pass |
|---|---|---|
| G1 | keys absent = M6 bit-exact (latency 0); RANGE 0 in every mode and look-ahead = input delayed by L | exact |
| G2 | latency reported = measured (0 / 96 / 240); blocks 1 / 64 / 512 identical; 0 allocations | exact |
| G3 | Look-ahead, ZASTAVA (range 40, attack 0.5 ms) on hits from silence: gain on the hit's first sample | VOSTOK and SOYUZ ≥ 90 % (−0.92 dB); SPUTNIK reported |
| G4 | Key filter: bass 60 Hz −6 dBFS steady + hats 8 kHz −20 dBFS, threshold −30, KEY HP 2 kHz | opens only on the hats (openings = number of hats ± 0, bass alone never opens); without the filter it stays open |
| G5 | No chatter on a 50 Hz sine around the threshold (±1 dB), every mode | ≤ 1 state change per second |
| G6 | TREVOGA range 12: during key hits the signal drops by 12 ± 0.5 dB; with the key quiet it returns to 0 ± 0.1 dB | pass |
| G7 | FLIP: gate output + flip output = input delayed by L | ≤ 1e−6 |
| G8 | PATRUL ratio 2, range 30: steady tone 10 dB under the threshold is lowered by 10 ± 0.5 dB | pass |
| G9 | KEY LISTEN = the key filter's output delayed by L (an independent filter) | ≤ 1e−5 |
| G10 | OUTPUT ±6 dB = output × 10^(O/20) | ≤ 1e−6 relative |
| G11 | Page in the real editor; T11 through the VST3 binary; CPU and memory reported | pass |
