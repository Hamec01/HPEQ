# Wet return level / 2026-11-04

Owner reports the reverb itself is barely audible. Screenshot: TRAP Memphis
Reverse Horizon, Bloom, Reverse Audio, Mix0.7906208, Output0, Early-9, Tail0,
Duck0.08. The displayed WET value is absolute wet RMS before mixing, not a
wet/dry ratio. Source level is not visible in the screenshot. KEEP DRY only
preserves the direct signal; it cannot increase the return.

Before changing audio, use the same WetLevelTests before/after. Measure the
actual wrapper in Send with/without Reverse on normalized owner keys2 material
and with six neutral characters on deterministic excitation, including tails.
Do not infer a broken Reverse or an absolute loudness target from one meter.

Acceptance for a dedicated WET LEVEL control:

- Append wetDb after all existing host parameters; range -24..+24dB, default0.
  Old projects/programs/parameter order remain unchanged. Legacy ignores it.
- Wet-only return trim after EQ/Duck/Width and before Insert/Send mixing,
  outside the tank/feedback. Smooth20ms on the sample clock; dry untouched.
- At +6/+12/+24dB, full-render Send energy gain within0.01dB of the setting.
  Insert with KEEP DRY agrees with dry+Mix*boostedSend within2e-6 per sample.
- MIX0/bypass and pre-predelay dry are unchanged; no new latency, allocation,
  block-size dependence, freeze growth or changes to Reverse timing.
- Missing old-state ID resets0 even on edited instances. New state restores;
  factory preset load resets trim0, and edits show the modified marker.
- Show WET LEVEL on Main and explain the difference from OUTPUT. Shimmer remains
  in Advanced/Effects. Meter reflects return trim and stays labeled wet.
- 208 old program PCM hashes and390 M0 renders exact; native suite passes;
  pluginval5..10, Steinberg validator, reviewed Main/Advanced/Legacy snapshots.
- Deliver verified VST3/ZIP/SHA256. No installer or installed-copy replacement.

This adds explicit control, not automatic makeup or hidden preset boost. Keep
200 original recipes/calibration at wetDb0. +24dB is user-selected amplification,
not a limiter; boosted audio may exceed0dBFS. Do not renormalize test renders to
hide gain changes. No claim about the owner's exact FL audio without that clip.
