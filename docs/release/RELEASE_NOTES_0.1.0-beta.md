# HPEQ 0.1.0 beta — release notes

VST3 audio effect for Windows x64. Every layer is **PRE-EQ → frequency-range FX regions → delta mix**;
up to 10 layers, Parallel or Serial.

## What is in this beta

- **PRE-EQ** (up to 24 points per layer): Bell, Low/High Shelf, Low/High Cut 6–96 dB/oct, Notch,
  Band-pass, Tilt, All-pass; Stereo / Left / Right / Mid / Side per point.
- **FX regions** (up to 8 per layer), each a chain of up to 6 processors on that frequency range:
  Compressor, Limiter (look-ahead, guaranteed sample-peak ceiling), Saturator (tanh / tube / hard,
  1x/2x/4x oversampling), Transient shaper, Gate/Expander, Stereo (width/balance), Reverb and Delay
  (wet-only sends, tempo-synced delay, ping-pong), Monsta FX (tempo-synced slice repeat / reverse /
  stutter / crush / gate), Gain. Region channel mode: Stereo, Linked, Dual Mono, L, R, M, S.
- **10 layers**: Parallel (each layer adds only its difference: ten empty or bypassed layers leave the
  sound untouched) or Serial (1 → 2 → … → 10). Per layer: On, Mute, Solo, Gain, Mix, Pan, Width, which
  part of the signal it processes (Stereo / L / R / Mid / Side), Copy / Paste / Duplicate / Clear.
- **Latency compensation**: look-ahead and oversampling are reported to the host; dry signal, other
  regions and other layers are aligned to it.
- **Analysers from the real audio**: input/output waveform, spectrum, difference spectrum, the exact
  PRE-EQ curve, meters, loudness (momentary / short-term / integrated LUFS, BS.1770-4) and true peak
  (dBTP).
- Undo / redo, A/B, all values typeable.

## Measured (Windows 11, i5-10300H, 48 kHz, 512-sample blocks)

| | |
|---|---|
| CPU, 10 layers × 1 compressor region | p50 0.44 ms, p99 7.4 % of the block time |
| CPU, 5 layers × 5 compressor regions | p50 0.87 ms, p99 12.6 % |
| CPU, 10 layers × 5 compressor regions | p50 1.76 ms, p99 24.7 % |
| CPU, 10 layers × 5 regions × [comp, saturator 2x, transient, limiter] | p99 110–118 % — **does not run in real time** |
| Memory per instance (committed) | 45 MB at 44.1 kHz, 48 MB at 48 kHz, 79 MB at 96 kHz, 143 MB at 192 kHz |
| Loudness accuracy (EBU Tech 3341 cases 1–5, synthesized) | within 0.04 LU |
| True peak (fs/4 sine at 45°) | 0.11 dBTP for 0 dBTP (tolerance +0.2 / −0.4) |

## Known limitations

- **Heavy projects**: oversampled saturators are the most expensive processors; 10 layers full of
  heavy chains exceed real time. Muted layers keep running (seamless un-mute) and cost full CPU;
  bypassed layers cost nothing.
- Bypassing a **layer** cuts reverb/delay tails after a 20 ms fade (bypassing a region keeps them).
- Switching Parallel ↔ Serial, or changing oversampling / look-ahead, changes the latency; the host
  re-aligns and the sound jumps at that moment.
- Layer parameter changes are smoothed from the host block boundary, so automation is not exactly
  block-size independent (static settings are).
- Parallel layers can exceed 0 dBFS: there is no hidden limiter. Watch the meters.
- True peak at 176.4/192 kHz is measured without oversampling (as BS.1770 allows at those rates).
- FX regions and layer settings are not host-automatable yet (only Output Gain and Bypass).
- Pools shared by the whole project: 16 reverbs, 8 delays, 8 Monsta FX; an extra one is silent and
  marked in the inspector.
- Not code-signed. Windows only; no macOS/Linux build.
- Validated with pluginval 1.0.4 at strictness 5–10 (all passed) and through the VST3 binary in a JUCE
  host (state, latency, sound at 6 sample rates × 6 block sizes, 100 open/close cycles). **Not yet
  tested in DAWs other than the owner's FL Studio check of an earlier build.**
