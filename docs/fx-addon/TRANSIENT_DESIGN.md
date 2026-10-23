# HamloUdarnik — design note (module pass 6: Transient)

Owner, 2026-10-23: transient pass after the briefing, built on the research note "HP Transient Morph
Engine" (owner's file, 2026-10-23), "in our Soviet Hamlo style". Owner accepted the scope and names:
**HamloUdarnik · MODEL UD-1 · № 1961** (udarnik = drummer and shock worker; 1961 = Gagarin), modes
**PRAVDA / UDARNIK / PERESTROIKA**, look-ahead **SPUTNIK / VOSTOK / SOYUZ**.

## 1. What we take from the research, and what not

The research proposes five ideas; its own choice for the first prototype is *Peak-Neutral Punch +
Transient Morphing*, then *confidence-aware processing*. This pass takes those three and a measurable
part of *Groove Guardian*:

| Research idea | This pass | How |
|---|---|---|
| Peak-Neutral Punch (§4) | **UDARNIK** mode | attack gain with a peak cap (output peak ≤ the input's local peak) and an energy-neutral body gain (§4.2 of the research: `α²E_a + β²E_b = E_a + E_b`) |
| Transient Morphing (§3) | **PERESTROIKA** mode | RISE: the attack envelope time-scaled by r (look-ahead); DECAY: the decay, in dB below the hit's peak, divided by k (an exponential decay τ becomes exactly kτ) |
| Confidence (§2.3) | event confidence 0…1 | envelope scales ~1 ms (onset), ~10 ms (how far the hit rises over the level just before it) and 2 s (accent reference); **not** called a Bayesian probability (the research itself says a sigmoid is not one without a trained model) |
| Groove Guardian (§7) | **PRESERVE** | each hit is treated against its own reference, not against the memory of the previous accent (§3.6) |
| Multi-band click / punch / body (§2.1) | the HPEQ architecture | the module runs on a band (region or point) or on the whole layer; no STFT needed |
| Transient DNA, Context Punch, Resonance Sculpting (§5–6) | not in this pass | need source separation, a sidechain or modal analysis. A short DECAY on a point at 250 Hz already shortens a "box" ring without touching 60 Hz |

## 2. Function set

| Function | SPL TD / Kilohearts TS / Oxford TransMod / spiff | HamloUdarnik |
|---|---|---|
| Attack | Attack ±, Kilohearts Attack | **SNAP** ±24 dB (key `attackDb`, the M6 attack) |
| Sustain | Sustain ±, Kilohearts Sustain | **BODY** ±24 dB (`sustainDb`) |
| Detector speed | Kilohearts Speed, TransMod rise | **SPEED** 0…1 (`speed`, the M6 speed; also the event attack window 20 ms · (0.5 + speed)) |
| Sensitivity | TransMod Sensitivity, spiff Sens | **SENSITIVITY** 0…1 (`sensitivity`, 1 = every front counts) |
| Shape of the attack | — | **RISE** ×0.5…×2 (`rise`, PERESTROIKA) |
| Shape of the decay | Sustain (level only) | **DECAY** ×0.5…×2 (`decay`, PERESTROIKA) |
| Groove protection | — | **PRESERVE** 0…1 (`preserve`) |
| Look-ahead | TransMod / spiff latency modes | **SPUTNIK** 0 / **VOSTOK** 2 ms / **SOYUZ** 5 ms (`lookahead` 0/1/2), latency reported |
| Peak safety | Kilohearts clip, TransMod soft clip | UDARNIK soft ceiling at the hit's peak (sample peak guaranteed) |
| Solo the parts | Physion / SplitEQ listen | **LISTEN** FULL / ATTACK / BODY (`listen`) |
| Mix, output | yes | **MIX** = the slot mix; **OUTPUT** −24…+12 dB (`outputDb`) |
| Mode | — | **PRAVDA / UDARNIK / PERESTROIKA** (`style` 0/1/2) |

Every new key absent = the M6 transient shaper, bit-exact (old projects sound the same). A module added
from the UI starts with VOSTOK (2 ms look-ahead); everything else at its neutral value.

## 3. Laws

Notation: detector time n (input sample n), audio time m = n − L (L = look-ahead in samples, rounded
0 / 2 / 5 ms), ℓ = linked level max_c |x_c|, peak followers `e ← ℓ + (ℓ > e ? a : r)(e − ℓ)` with
`a, r = exp(−1/(T·fs))`. All gains are in dB until the last step; the audio is delayed by L.

**3.1 Detector (all modes).**
- M6 envelopes: fast (0.5 ms / 20 ms·s), slow (20 ms·s / 250 ms·s), s = 0.5 + speed;
  `d = 20 log10(fast / slow)`; `w_a = clamp(d / 6, 0, 1)`, `w_s = clamp(−d / 6, 0, 1)`.
- Scales: e1 (0.05 / 1 ms), e10 (1 / 10 ms); `n_s = 20 log10(e1 / e10)` (front sharpness).
- Onset: armed and `n_s ≥ θ_on = 6 + 18 (1 − σ)` dB and ℓ > −100 dBFS. Re-armed when `n_s < θ_on − 3`
  and 10 ms have passed. σ = SENSITIVITY.
- Event (onset t0): confidence `c = clamp((R − 6) / 6, 0, 1)`, R = how far e10 climbs within 5 ms over
  its value at the onset (dB) — stationary noise hardly climbs, a hit does; peak
  `P = max ℓ` over the first 10 ms; strength `S = 20 log10(P / A)`, A = accent reference
  (instant attack, 2 s release); gate `S ≥ 12 − 72 σ` (no gate at σ = 1).
- Event factor k: PRAVDA with σ = 1 → 1 (the M6 law); otherwise `k = c · gate`. Before the first event
  k = 0 (nothing to shape). Events that fail the gate do not take over from the previous event.

**3.2 Event windows (audio time).** Attack window `a(m)`: ramp 0 → 1 over [t0 − L/2, t0 − L/4]
(L = 0: over [t0, t0 + 0.25 ms]), 1 until t0 + 0.7 T_a, raised cosine to 0 at t0 + T_a,
`T_a = 20 ms · (0.5 + speed)`; the newest two events are combined by max. Body `b = 1 − a` after the
first event, weighted by the M6 sustain weight: `b = (1 − a) w_s` (a steady signal is not "body").

**3.3 PRAVDA** (the M6 law with the new options):
- `W_a(m) = max(w_a(m), pre(m))`, pre = the event pre-ramp (L > 0): ramp to `w_max` over
  [t0 − L/2, t0 − L/4], hold to t0, fade over L/2 (w_max = max w_a over the event's first L samples).
- `G = k (SNAP · W_a + BODY · w_s)`.
- PRESERVE p: at an onset, if `e10 < slow`: `slow ← slow^(1−p) · e10^p` — the hit's d is measured
  against the level just before it, not against the previous accent.

**3.4 UDARNIK** (Peak-Neutral Punch):
- `G = k (SNAP · a + BODY · b) + β · b_E`, b_E = the hit's whole body for the balance: `(1 − a) k`, held
  300 ms after the onset and faded out by 400 ms.
- β (energy-neutral body, research §4.2): one-pole sums (300 ms) of **K-weighted** (BS.1770) energies
  `A_in = a k Σx_K²`, `A_out = a k Σy_K²`, `B_in = b_E Σx_K²`; `β = 10 log10((A_in + B_in − A_out) / B_in)`
  clamped to [−12, +6] dB, smoothed 20 ms: what the attack gains, the body gives back, so the loudness stays.
- Peak-neutral shaping where `g = 10^(G/20) > 1`: `y = s γ tanh((g/γ) artanh(x / s))`, with
  s = peak reference `max(H^(1−p) · P_ev^p, E, ℓ(m))` (H = peak hold of ℓ, 60 ms + 200 ms release; P_ev = the
  hit's own peak; PRESERVE p; E = sliding max of ℓ over [m + L − 10 ms, m + L]) and true-peak guard
  `γ = g^(−0.1)` (the shaped peak sits 0.1 × boost dB under s). Small samples get exactly g; `|y| < s γ ≤ s`:
  **the output sample peak never exceeds the input's own peak within the last 60 ms + L**. At g → 1 the
  shaper tends to y = x (continuous); where g ≤ 1, y = g x.

**3.5 PERESTROIKA** (Transient Morph), `G = k (SNAP · a + BODY · b + G_rise + G_decay)`:
- DECAY K: `E` as above; `P_d` = max E since the event's onset; after the peak (E < P_d − 0.5 dB):
  `G_decay = (1/K − 1) · max(20 log10(E / P_d), −60)`, clamped [−36, +24] dB. For a decay
  `E = P e^{−t/τ}` the output is `P e^{−t/(Kτ)}`: τ becomes exactly Kτ.
- RISE r: E_r = rectified mean of ℓ over 0.5 ms, centred (needs 0.25 ms of look-ahead), stored at ~12 kHz.
  From t0, source time `u = t0 + (m − t0) / r` (r < 1: at most the look-ahead horizon); target
  `T = E_r(u)` (r > 1) or its running maximum (r < 1: the target holds the peak until the source gets
  there); `G_rise = 20 log10(T / E_r(m))` clamped ±24 dB: the attack envelope becomes
  `E_r(t0 + (t − t0)/r)`, rise time × r. Ends (r > 1) when E_r(u) falls 1 dB under its maximum or
  (r < 1) when the source is within 0.1 dB of the peak, or after 20 ms of source time; then G_rise fades
  to 0 over 2 ms. r < 1 needs the look-ahead: rises up to `L r / (1 − r)` (SOYUZ, r = 0.5: 5 ms) are scaled
  fully. The 0.5 ms envelope is clean for the click (bands above ~2 kHz); on low bands it ripples, so RISE
  is meant for the click; DECAY works on any band.

**3.6 Common tail.** G smoothed by a 0.1 ms one-pole (UDARNIK: before the cap, so the guarantee
holds). LISTEN with split w (PRAVDA: W_a, else a): ATTACK `y w`, BODY `y − y w`
(ATTACK + BODY = FULL). OUTPUT `10^(O/20)` last. All neutral (SNAP = BODY = 0, RISE = DECAY = 1,
OUTPUT 0) = the input delayed by L exactly, in every mode.

**3.7 Meters.** IMPACT = `10 log10(A_out/A_in) − 10 log10(B_out/B_in)` over 300 ms with split w (how much
the attack/body contrast grew), gain range of the block, events (count, confidence, strength), and the
last hit's input and output envelope (64 points × 2.5 ms) for the page.

Memory: rings of L + a 80 ms envelope history at ~12 kHz per slot (allocated in prepare).

Snapshot encoding (LayerSnapshot): rise and decay as log2 of the factor, sensitivity as `1 − sensitivity`, so
a zero-filled parameter block is the M6 shaper.

## 4. Page (HamloUdarnik, Black Steel)

Nameplate "HamloUdarnik · MODEL UD-1 · № 1961"; amber LCDs IMPACT (dB) and HIT (confidence %, strength
dB). Ten brass knobs: SNAP, BODY, SPEED, RISE, DECAY, SENS, PRESERVE, MIX, OUTPUT and the mode knob
titled with the mode (PRAVDA / UDARNIK / PERESTROIKA; no red labels). Levers LOOK-AHEAD
(SPUTNIK / VOSTOK / SOYUZ) and LISTEN (FULL / ATTACK / BODY). Green CRT: left, the gain curve of the last
4 s with hit marks (brightness = confidence); right, "LAST HIT" — the input envelope and the output
envelope of the last hit. IMPACT needle meter, ON lamp and toggle. Design coordinates 1180 × 700.

## 5. Measurements (thresholds fixed before the runs)

| # | Test | Pass |
|---|---|---|
| T1 | keys absent = M6 bit-exact; neutral settings in each mode and look-ahead = input delayed by L, bit-exact | exact |
| T2 | latency reported = measured (0 / 2 / 5 ms); blocks 1 / 64 / 512 identical; 0 allocations | exact |
| T3 | Look-ahead, PRAVDA SNAP +12 dB on hits: gain on the hit's first sample | VOSTOK, SOYUZ ≥ 90 % of 12 dB (SPUTNIK reported) |
| T4a | Pink noise −20 dBFS without hits: UDARNIK and PERESTROIKA (SNAP +12, BODY −12, σ = 1), PRAVDA σ = 0.5 | every 50 ms RMS window changes ≤ 1 dB |
| T4b | Hits 20 dB over that noise, PERESTROIKA SNAP +12 (RISE = DECAY = 1) | attack-window (first 10 ms) energy gain ≥ 11 dB |
| T4c | Bleed hits 30 dB under the accents, σ = 0.5, SNAP +12 | bleed changes ≤ 1 dB; accents ≥ 11 dB |
| T5 | UDARNIK SNAP +6, SOYUZ, drums | attack/tail contrast (0–30 ms vs 30–300 ms after each hit) ≥ +3 dB; output sample peak ≤ input sample peak; true peak ≤ input true peak + 0.1 dB; loudness within ±0.5 LU |
| T6 | PERESTROIKA, SOYUZ, hits `A(1 − e^{−t/1 ms}) e^{−t/60 ms}` on a 4 kHz carrier (and the decay at 200 Hz) | DECAY ×0.5 / ×2: fitted τ = K·τ ± 10 %; RISE ×0.5 / ×2: 10–90 % rise time = r × input ± 15 % |
| T7 | PRESERVE: accent 0 dB, ghost −12 dB 100 ms later (τ = 15 ms), SNAP +6 in PRAVDA and UDARNIK | p = 1: ghost/accent attack energy ratio changes ≤ 0.5 dB (p = 0 reported) |
| T8 | LISTEN: ATTACK + BODY = FULL | ≤ 1e−6 |
| T9 | Onset not moved (SNAP +6, all modes, RISE 1): output onset vs input onset + L | ≤ 0.1 ms |
| T10 | OUTPUT −6 / +6 dB = output × 10^(O/20) | ≤ 1e−6 relative |
| T11 | Page in the real editor; T11 through the VST3 binary; CPU (bench) and memory (HPEQ_MEM_FS) reported | pass |
