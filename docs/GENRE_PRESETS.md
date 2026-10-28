# HamloRedWall: genre presets and the preset screen

Owner (2026-10-28): presets per HPDG genre - Boom Bap, Trap, Techno, DnB - 50 each, "for kicks, snares, hats, drum
bus mastering, vocals, samples, everything", sorted into groups; a small screen like the CRT with the presets and
left / right arrows, at the same place on the left of every Hamlo plug-in; "the presets must be really good, not just
different values".

## 1. The preset screen

`Source/UI/PresetScreen.h` (the same file in HamloComp, HamloUdarnik and HamloRedWall): a green CRT at the design
rectangle 56, 20, 336 x 116 of the 1180 x 820 stage - top left on every page (the LCDs that sat there on HamloUdarnik
and HamloRedWall moved right of the nameplate). Lines: GENRE v and GROUP v (click: choose), the preset name between
the arrows (previous / next inside the genre and group, also the mouse wheel; click the name: a menu of every preset
of the filter, in genre / group sub-menus), the preset's note and its place (12 / 50). A `*` after the name: a knob
moved since the preset was loaded. The host's program list carries the same presets as "Genre / Group / Name"; the
loaded program is saved with the project. HPEQ's presets and the earlier ones are the genre "Factory".

## 2. Genres and groups

| Genre | Tempo (HPDG default) | Names come from the HPDG substyles |
|---|---|---|
| Boom Bap | 90 BPM | Classic, Dusty, Jazzy, (Boom Bap) Gold, (Russian) Underground, Lofi |
| Trap | 140 BPM | ATL, Dark, Cloud, Rage, Memphis, Luxury |
| Techno | 132 BPM | Peak (Time), Hypnotic, Minimal, Detroit, Dub, Acid, Hard |
| DnB | 174 BPM | Modern, Roller, Liquid, Neuro(funk), Jump-Up, Break(beat) |

Per genre: Kick 6, Snare / Clap 6, Hats / Cymbals 5, Percussion 4, Drum Bus 7, 808 / Bass 5, Vocals 5, Keys / Piano 4,
Samples / Melody 4, Master 4 = 50.

Rules used for every recipe (the references below; numbers are starting points the sources agree on):
- **Release follows the tempo**: where a preset says 1/16, 1/8 or 1/4 its release is that note at the genre's tempo
  (1/16 = 167 ms at 90 BPM, 107 ms at 140, 114 ms at 132, 86 ms at 174): the gain is back before the next hit, or
  it breathes with the groove on purpose [1, 3].
- **Kick / snare**: a slower attack (10 .. 25 ms) lets the transient through - punch; a very fast attack and release
  squash the stick and lift the body - fat; a forward kick or snare is 3 .. 6 dB of gain reduction, light control
  1 .. 2 dB [1]. Crush and smash presets are parallel (MIX < 100 %), the clean hit stays on top [2, 4].
- **Genre character**: Boom Bap leans on FET / tube colour, slower attacks and fuller bodies (the sampler sound);
  Trap keeps the kick and clap snapping over a long 808 that is held even note to note, vocals hit hard (6 .. 10 dB
  on the loud ones) [2]; Techno glues the bus and lets the kick / rumble group breathe on the quarter [3]; DnB is
  fast - short releases, snappy snares, breaks fattened in parallel, transients shaped before the bus [4].
- Every recipe has its own purpose, written in its note on the screen; two presets of one group never share all
  values (checked).

## 3. HamloRedWall

Each recipe sets STYLE (GLASNOST transparent, STAKHANOV fast with less ducking, OCTOBER soft clip first),
LOOKAHEAD (short on drums so the attack stays, 3 .. 8 ms on bass and full mixes), RELEASE (tempo notes, or AUTO on
sustained material and masters), the colour stage (DRIVE for edge, TUBE for weight - docs/REDWALL_V2.md), LINK, MIX
(parallel smash on the bus), LIMIT -1 dB and TRUE PEAK on the masters. **LOUDNESS is calibrated** on the material:
`HamloRedWall_PresetLab calibrate` finds, per preset, the LOUDNESS that gives the middle of its depth's band of average
gain reduction - a limiter's bands, lower than a compressor's: GENTLE 0.3 .. 1.5, MEDIUM 1.5 .. 3, HEAVY 3 .. 6 dB -
on its group's clips at -18 dBFS RMS (mixes -14); `check` judges band, the output under LIMIT (MIX 100 %), level
change and uniqueness: docs/reports/presets/genre_presets_check.md.

Result: 200 / 200 genre presets in their band, under LIMIT, unique per group (docs/reports/presets/genre_presets_check.md).

Single hits (kick, snare, hats, percussion) use a short release per genre - Boom Bap 30, Trap 22, Techno 20, DnB 15 ms
(at most twice that): with the tempo's 1/16 (167 ms at 90 BPM) the limiter only turned the whole hit down. Look-ahead
per genre x1.2 / 1.0 / 0.85 / 0.7 (Boom Bap / Trap / Techno / DnB): the faster the tempo, the shorter, so the attack stays.

Disclosed (method and recipe changes after runs, same presets and material):
- Run 1 calibrated on the limiter's average gain reduction: with DRIVE (relative to LIMIT) or OCTOBER the peaks are
  taken before the limiter, whose gain reduction read about 0 at any LOUDNESS. Run 2 used the crest-factor cut: on bass
  with a long release the limiter turns it down evenly (14 dB) while the crest factor barely moves. Since run 3 the
  depth is the larger of the two ('work'): GENTLE 0.5 .. 2, MEDIUM 2 .. 4, HEAVY 4 .. 8 dB.
- Run 2 found the hats and percussion, gained to -18 dBFS RMS, peaking over LIMIT: the clips are also held to a -3 dBFS peak.
- Run 2 showed the single-hit release (above) and many presets with every value equal across genres (the look-ahead
  rule above; uniqueness is checked per group).
- Run 3 found a bug in the colour stage, fixed in the DSP: switched on from the start, it faded in from the dry signal
  over 10 ms, and the limiter pushed that by LOUDNESS - at -30 dB the first 10 ms hit LIMIT. Now it starts full; the
  fade is only for switching on / off while playing (unit test; docs/REDWALL_V2.md lab R1 .. R10 unchanged:
  docs/reports/v2/after_presets.md). The test's first threshold (-9 dBFS, near LIMIT / 3.5) was loosened to -6: clipped
  drums overshoot to -6.7 dBFS through the oversampler and the DC blocker, still under LIMIT.
- Run 3, 19 presets outside their band, changed and recalibrated: 7 heavy DRIVE presets (Lofi Crunch, Rage 808 / Hard
  Rumble / Neuro Bass Distort, Hard / Neuro Kick Distort, Jump-Up Wobble Grit) reach the DRIVE cap at 3.5 .. 4 dB and are
  MEDIUM now; 12 percussion / hat / bus / keys presets took more than their band from TUBE / DRIVE alone at LOUDNESS 0
  and have less colour.
- Known, not changed here: DRIVE saturates relative to LIMIT, so with a lot of DRIVE the loud parts settle at about
  LIMIT / k (k = 1 + 2.5 DRIVE; -9 dB at 60 %) - more LOUDNESS then gives density, not level.

## Sources

1. [Mastering The Mix: compression settings for a punchy mix](https://www.masteringthemix.com/blogs/learn/best-compression-settings-for-a-punchy-mix), [ProSoundWeb: five steps to punchy drum compression](https://www.prosoundweb.com/five-steps-to-punchy-drum-compression/), [Sonarworks: drum compression tips](https://www.sonarworks.com/blog/learn/killer-drum-tracks-compression-tips), [EDMProd: three ways to compress drums](https://www.edmprod.com/three-different-ways-to-compress-drums/)
2. [Sage Audio: how to mix trap](https://www.sageaudio.com/articles/how-to-mix-trap), [Violet Recording: mixing trap drums](https://violetrecording.com/how-to-mix-trap-drums/), [Rys Up Audio: trap vocals](https://rysupaudio.com/blogs/news/how-to-mix-trap-vocals), [Antares: vocals in dense trap beats](https://www.antarestech.com/blog/how-to-make-your-vocals-sit-in-dense-hip-hop-trap-beats)
3. [Attack Magazine: dark techno rumble](https://www.attackmagazine.com/technique/beat-dissected/dark-techno-rumble/), [MusicRadar: rumbling techno kick](https://musicradar.com/how-to/rumbling-techno-kick)
4. [MusicRadar: clean up and enhance a sampled breakbeat](https://musicradar.com/tuition/tech/how-to-clean-up-and-enhance-a-sampled-breakbeat-621106), [Gearspace: cracking those transients](https://gearspace.com/threads/cracking-those-transients.544384/)
