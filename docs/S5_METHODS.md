# S5: genre library and VST-only acceptance

2026-11-02; parent5765c5e. Written before library/calibration measurements.
Owner excludes the installer for this stage. Work only in HamloShirota, without
subagents, other products, push or subtree. Preserve all earlier accepted evidence.

## Library

Append200 programs after the original seven,207total. Four genres x50: Boom Bap,
Trap, Techno, DnB. Per genre: Kick6, Snare/Clap6, Hats/Cymbals5, Percussion4,
Drum Bus7,808/Bass5, Vocals5, Keys/Piano4, Samples/Melody4, Master4. Every recipe
has permanent ID, genre/group, descriptive note, source material, tempo and depth.
Different Mix/Output alone is not unique; compare all actual sound controls with
UI-only page/node and restart excluded. Preserve old program numbers/parameters.
The shared preset screen only receives the new metadata; its implementation stays
unchanged. Modern Default is neutral; original Legacy Default stays compatible.

## Depth, material and calibration

Shirota is a spatial processor with a direct component, so reverb wet/dry energy
does not describe its widening. Define spatial change as residual energy:
10log10(sum((Y/g - delayed X)^2)/sum(X^2)), where g is the documented Output gain,
Y is the actual plug-in output, X is prepared input, and delay is reported PDC.
Integrate the whole10s input plus1s silent tail, both channels. This measures
actual spatial change including phase/pan, not a perceptual loudness score.
Gentle:-24..-16dB; Medium:-16..-8dB; Heavy:-8..0dB. Target centers:-20/-12/-4.
Output trim cannot manufacture depth. Calibrate Spatial Mix within0..1 using a
full-strength render and linear blend, then verify by a fresh actual VST processor
render. All other recipe controls are authored for musical function, not randomized.
Unreachable targets fail and require an explicit recipe revision, not relaxed limits.

Use the owner's WAVs from the AGENTS material directory. Resample to48k stereo,
select the highest-energy10s window (100ms stride), pad short sources, target
-18dBFS RMS (-14for mixes) capped at -6dBTP. Record original/normalized hashes.
Per-preset Output may only lower peaks to <=-1dBTP (8x polyphase measurement),
with that gain removed for the depth calculation. Gain is not recipe uniqueness.
Verify rendered output finite, residual in range, peak <=-1dBTP, state/selected
program roundtrip and identical roundtrip audio. Synthetic deterministic stereo
material supplements validation; owner audition remains separately pending.

Caches bind normalized DSP/routing/parameter sources, bank, build configuration,
method/tool, recipe excluding calibrated Mix/Output, input bytes and tempo.
Record the actual renderer executable hash for each native invocation. Sound
identity excludes the generated preset table so calibrating that table cannot
invalidate unchanged DSP results. Never reuse a completed cache with changed
inputs/method/DSP; verify every output hash. Preserve interrupted attempt folders
and resume completed per-preset records. Table generation is deterministic.

## Other acceptance

| ID | Target |
|---|---|
| P01 | Before catalog7/new0(n/a), after207/new200; old first7names/order unchanged |
| P02 | 200/200 metadata, parameter ranges, unique IDs/names/recipes, genre/group counts |
| P03 | 200/200 depth, finite, <=-1dBTP, state/program/audio roundtrip |
| P04 | Native tests,160M0audio/state/latency, final integration/allocation/block checks pass |
| P05 | Full integrated binaural impulses at8/32/44.1/48/88.2/96/176.4/192k,3profiles,0/90/180/270deg: magnitude error<=0.5dB,100Hz..min(16k,.4fs), source above-40dB; report correlation ITD error<=1sample versus independently resampled native48k reference; no claim384k |
| P06 | pluginval5..10/Steinberg pass on the final delivered VST; entire bundle and licenses copied, DLL SHA matches |
| P07 | Genre/group/note and modified state visible in inspected QUICK/STEREO/SPACE/MOTION snapshots |
| P08 | Production owner-material A/Bs and three-profile front/left/back/right comparison, common gain per material, <=-1dBTP |

Repeat CPU means/outliers without claiming FL deadline safety. Owner FL/K712
listening cannot be substituted by offline tests; disclose as pending. Version
0.2.0 identifies the VST build, not certification of unperformed live-host checks.
No installer is required. Final handoff/report must name every failed/open goal.
