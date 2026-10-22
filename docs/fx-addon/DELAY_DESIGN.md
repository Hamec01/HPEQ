# HamloEcho — design note (module pass 3: Delay)

Owner, 2026-10-23: "delay по типу Timeless; дизайн опираясь на HamloComp". Reference: FabFilter Timeless
(public feature descriptions; the plugin itself was not available on this machine for a side-by-side).

## 1. Function parity

| Function | Timeless | HamloEcho | Notes |
|---|---|---|---|
| Separate L / R delay times, free or tempo sync, link | yes | **Time L, Time R**, FREE / SYNC each, **LINK** | 1 ms – 4 s; sync 1/32 … 1/1 with dotted and triplets |
| Feedback and cross-feedback (ping-pong) | yes | **Feedback**, **Cross**, **PING-PONG** input | cross 0 = two delays, 1 = full ping-pong |
| Tape-style or smooth time changes | yes | **TIME MODE: TAPE / FADE** | TAPE glides (pitch bends like tape), FADE cross-fades (no pitch) |
| Filters in the feedback path, resonant, drawn and draggable | yes (several, many shapes) | **Low cut**, **High cut** (12 dB/oct, resonance), **Band** (bell: freq, gain, Q) | on the CRT: three draggable nodes; the curve after 1…4 repeats |
| Drive / saturation | yes | **Drive** + **CHARACTER** | Clean / Tape / BBD / Lo-Fi (+ Classic = the M6 delay) |
| Diffusion | yes | **Diffusion** | allpass smear in the loop: echoes melt into a reverb |
| Pitch shifting in the loop | yes | **Pitch** −12 … +12 semitones per repeat | two-grain shifter |
| Modulation (wobble) | yes (LFOs, matrix) | **Mod rate, Mod depth** (+ character flutter) | fixed targets (time), no free matrix |
| Lo-Fi | yes | **Lo-Fi** (sample rate + bits) | |
| Width, ducking, freeze | yes | **Width**, **Duck**, **FREEZE** | freeze = lossless loop, input closed |
| Mix | yes | module **Mix** (the send amount) | |

Not copied: Timeless's free modulation matrix (XLFOs, envelope generators, MIDI triggers) and its
six free filter slots — HPEQ already places the delay on an EQ band; the fixed set above covers the use.

## 2. Engine (written for HPEQ, D3)

```
in -> (input gate: freeze) -> L/R lines (4 s) -> read (time + wobble; TAPE glide or FADE two-head cross-fade)
   -> loop: low cut -> band -> high cut -> diffusion -> pitch -> character (drive / tape / BBD / lo-fi) -> safety clip
   -> echo: width -> ducking -> wet
   feedback: L <- fb * ((1 - cross) eL + cross eR), R likewise; PING-PONG feeds the input to the left line only
```

- Characters: **Clean** (only the safety clip, transparent below 0 dBFS); **Tape** (soft asymmetric
  saturation, wow + flutter, gentle loss of highs); **BBD** (dark bandwidth, rounder clipping, chorus-like
  wobble); **Lo-Fi** (reduced sample rate and bits). Drive lowers the clip ceiling; small signals keep
  their level (no jump when drive moves).
- A safety clip (2·tanh(x/2)) keeps any feedback / resonance / band boost bounded.
- Freeze cross-fades the loop to the raw echo with gain 1 (lossless) and closes the input.
- Classic keeps the M6 code path bit-exact (projects saved before this pass have no `character`).
- Control-rate filter design on the running sample clock; no allocation in `process()`; the pool keeps 8.

## 3. Page (HamloEcho, Black Steel like HamloComp and HamloVerb)

Nameplate **HamloEcho · MODEL ЭХО-2 · № 0421**, amber LCD (character, times, feedback). Left: **CHARACTER**
rotary selector (Classic, Clean, Tape, BBD, Lo-Fi), **TIME MODE** lever (TAPE / FADE). Centre: green
phosphor CRT, two views — **ECHO** (the echo train: left echoes up, right echoes down, heights from the
feedback and the loop filters, ping-pong and cross visible, pitch steps labelled, a play head) and
**FILTER** (loop response with LOW / BAND / HIGH handles, and the response after 2, 3, 4 repeats as
fading traces). Right: **OUTPUT** needle meter, ON lamp, **PING-PONG** and **FREEZE** levers. Knobs: large —
Time L, Time R (FREE / SYNC under each, LINK between), Feedback, Cross, Drive, Width; small — Mod rate,
Mod depth, Diffusion, Pitch, Lo-Fi, Low cut, High cut, Band freq, Band gain, Band Q, Duck.

## 4. Measurements (thresholds fixed before the runs, see the pass report)

Classic bit-exact; echo positions L / R free and synced ±1 sample, link; feedback ratio ±0.5 dB (Clean,
filters open); cross 1 alternates sides (≥ 30 dB); loop filter response = the analytic biquads ±0.5 dB;
band +6 dB ±0.5 dB; Clean at −20 dBFS THD < −80 dB; drive raises THD; pitch ±12 → frequency ×2 / ×0.5
±2 %; modulation spreads a sine echo (sidebands ≥ −30 dB vs < −80 dB still); FADE time change keeps the
pitch ±1 %, TAPE bends it > 2 %; diffusion lowers the first echo's crest factor ≥ 10 dB; lo-fi 1 THD+N
> −40 dB; BBD darker than Clean (centroid of the 6th echo ≥ 30 % lower); freeze ±1 dB over 9 s; ducking;
block sizes identical; fuzz finite and bounded; 0 allocations; CPU per delay reported.
