# R12: stronger, varied RISE and DRUM FILL

2026-11-04. Parent5e89bae. Owner rates existing transitions10/100 and requests
improvement using examples in E:/DRUMS. No subjective score is claimed by tests.
Work in isolated clean checkout E:/HamloRevolutionTransitions,branch
feature/transition-v2. Managed-worktree creation was unavailable for this chat's
E:/ workspace; a shared local clone preserves the original's parked R11 edits.
No HamloVerb,HPEQ,installer,installed-copy replacement or push in this task.

Before production changes:analyze local riser/fill references for onset patterns,
energy/brightness development,accents and phrase endings. They are listening/
measurement references,not sounds embedded in the plugin. Keep originals intact;
record source hashes and prepare reproducible input fixtures. Existing renderer
and binary retained as baseline. Same lab before/after,new capability n/a before.

New offline generator:source-transient selection,at least six musical recipe
families per kind,variable phrase structure/accents/rolls/gaps and endings.
RISE develops density,pitch,timbre and space into the downbeat; FILL reconstructs
actual source attacks instead of repeatedly grabbing an arbitrary last1/16.
Clear recipe selector plus Auto; retain the four performance knobs,1/2/4 bars,
load/capture,preview,WAV export and file drag. Classic remains an explicit recipe
and exact old renderer. Existing normal playback/programs/parameters unaffected.

Acceptance fixed before measurements:
- Legacy renderer PCM exact for the same source/settings/seed; classic existing
  transition tests stay valid. Normal playback225 old-program/state hashes exact.
- New render lengths exact for bars1/2/4,44.1/48/96kHz,tempos70/95/120/140/175;
  deterministic,finite,silence stays silent; ends at zero with a soft boundary.
- At least12 structurally distinct source/timing plans in16 Auto seeds,each kind;
  at least4 distinct timing/source plans in8 seeds per fixed recipe. Gain-only
  variation does not qualify. Record audio onset-envelope similarity as well.
- RISE last-quarter energy at least6dB above first and brightness increase on
  at least90% of prepared drum/percussion probes with default controls. Report
  exceptions rather than relax targets. Pitch/swell/roll/sweep remain effective.
- DRUM FILL edits spread across its final bar,with purposeful pauses/accents and
  alternating attack sources. Earlier bars retain at least75% source for context.
  Default output must materially differ from input after matching level.
- Independent8x true-peak on prepared demos<=-1dBTP,without using level change
  alone as improvement evidence. Save level-matched before/after audio examples.
- Native tests,allocation/normal-playback checks,pluginval5-10,Steinberg537,
  screenshots of recipe controls,validated exact VST3 delivery/ZIP/SHA256.

Reference material and new statistical diagnostics complement owner listening;
they cannot certify taste or claim that musical quality reached100/100.

First control fixture used default STOP.3 and expected that particular seed to
change. It failed because the ending was not selected. Corrected the control
test to0 versus1 (the same intended effective-range goal); no threshold moved.
Initial binary/log retained. The lab method is unchanged. First independent
diversity analysis included FILL's unchanged earlier bars; retained that metric
and added edited-final-bar and Legacy comparisons. No existing result removed.

Release review found that using the grain reader's.94 anti-alias cutoff for
metering could hide near-Nyquist raw peaks. Final meter measures raw samples plus
full-band8-phase interpolation instead of4 phases with that cutoff. Added hot
near-Nyquist/impulse regressions; the independent8x ceiling target stays-1dBTP.
Original passing prepared-material analysis remains available as an intermediate.
Also bound each grain to available source samples before applying its envelope,
so late source attacks cannot fall abruptly into zero-padding mid-grain.
