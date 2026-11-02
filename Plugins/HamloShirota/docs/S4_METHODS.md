# S4: production integration acceptance

2026-11-02; parent d1ef423. Goals recorded before production sound changes.
Work only in HamloShirota; no subagents, other products, subtree or push.
Keep S1/S2/S3 archives and accepted source files immutable. Production adaptations
are separate classes. This stage integrates the VST/editor; 200 genre programs
and final packages follow in the next stage, without renumbering the seven old
programs. Product text remains English; shared PresetScreen stays unchanged.

## Routing/PDC decision

Legacy keeps its original arithmetic, parameter prefix, initial ramps and fixed
host PDC round(2.5ms*fs). New processing must fit that same reference delay, so
engine switching does not alter PDC or call host notifications in audio.
STEREO/SPACE uses the S2 reference D=2*round(32*fs/48000). The remaining reference
R=round(0.0025*fs)-D is used by the production motion renderer.

Prepare a separate minimum-phase horizontal bank from the licensed S3 magnitude
data, with measured ITD restored around reference R. Keep native S3 filters and
auditions intact. Own real-cepstrum spectral factorization; report frequency
error and ITD versus S3. Published method context:
https://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.minimum_phase.html
No source file/runtime download. Embed data and preserve license/changed-data
notice. Resampling/data preparation only in prepare; audio uses prepared memory.

New chain: S2 Stereo/Space -> Stereo or Binaural position -> aligned global
Mix/Output/Bypass/Mono audition. Legacy is a separate branch. New controls default
neutral with engine Legacy. Old states reset all new controls even on an edited
instance. Existing11spec parameters plus bypass retain their order/ranges/IDs;
new IDs append after bypass. Exact original serialization is retained when all
new controls are neutral; a modern state includes new settings/program/view.

## Acceptance before the first run

| ID | Required evidence |
|---|---|
| I01 | Original12parameter prefix and seven programs unchanged; 160M0raw/state/latency cases bit-identical at original block schedules. Old states reset all new controls on fresh and edited instances |
| I02 | New neutral/bypass/Mix0 output equals fixed-PDC delayed input, including initial prepare; mono/stereo and mono-to-stereo supported |
| I03 | New static/event output bit-identical for blocks1/64/127/512/4096 with identical parameter/tempo/path events. No audio/setter allocations; prepared256 accepts4096 |
| I04 | Production bank magnitude error<=0.5dB at100Hz..16kHz where source>-40dB of its maximum, ITD error<=1sample at native48k. Report transformations and tail loss |
| I05 | Rates8/32/44.1/48/88.2/96/176.4/192k prepare/process finite, fixed PDC correct; no external data file required |
| I06 | Width0 removes generated/original Side with Space off; width2/4 gains6.0206/12.0412dB +/-0.02; mono bass and band routing cover generated Side. Stereo/binaural emitters preserve source direction |
| I07 | Space size/predelay changes use prepared crossfaded static heads; mono-bass coefficient changes use dual prepared filters with a ramp. No delay-read pitch sweep on a held tone; event output finite and within old/new peak-sum+0.01 |
| I08 | State roundtrip includes every new parameter, all16nodes, selected program/view and modified mark. Program load resets modern settings; Default modern path neutral |
| I09 | Native tests, pluginval5..10 and Steinberg validator pass against this integration binary; explicit new-mode checks supplement validators defaulting to Legacy |
| I10 | Combined48k/256modern path mean<=25%block period in three runs. Record wall-time outliers and thread work separately; do not infer deadline safety from means |
| I11 | Screenshot inspection of QUICK/STEREO/SPACE/MOTION/LEGACY at design size, with preset at56,20,336x116; typed controls, per-node holds/travel/arc/curve, contextual help, no overlap |
| I12 | Source/data/tool identity binds lab renders; identical before/after lab, missing functions n/a. Failed results retained, no relaxed goals hidden |

Legacy compatibility lab projects the original first12parameters rather than
requiring total parameter count12. Its160fixtures/signals/events/state-byte
comparisons are unchanged. Run this same projection before and after, and verify
the appended parameters independently. Never rewrite the original M0 archive.

## Experiment disclosures

The first25case integration run verified rates/allocations/block events. Before
measuring I07, add six timing fixtures (static before/after and transition for
Space timing and mono-bass). Isolate the wet997Hz tone by subtracting the known
direct coefficient, apply a200ms Hann window centered on the Space transition,
and require energy outside997+/-100Hz <=-30dB relative. This distinguishes a
moving-read pitch sweep from a crossfade of static heads. Peak limits remain
the pre-recorded old/new peak sum+0.01. Repeat the final before/after driver with
the frozen d1ef423 facade; the original run is retained, never relabeled.

Position has an explicit On switch, default Off, independent of the renderer
selection. Thus every new control can be neutral, including when Binaural is
selected; renderer selection alone is configuration. The UI enables Position
when selecting Binaural or turning Motion on. Restart is an automatable A/B
trigger, not a quick off/on pair that could vanish between callbacks. Editor
page/node selection persist but do not mark the audio recipe modified. Compact
Legacy states retain M0's omission of selected-program metadata; full modern
states preserve program/view/modified status. Travel Rate changes clock speed
without resetting phase; changing the actual node recipe restarts its path.

S4 code identities normalize text CRLF to LF; bank bytes remain exact. The same
generated fingerprint is embedded in both processor/modern DSP translation units
and checked before use; the lab checks its matching copy. Baseline uses the
frozen old processor/editor/variant and unchanged common Legacy objects, exposes
12parameters and identifies itself explicitly as the d1ef423 facade.
