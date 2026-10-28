# HamloComp: genre presets and the preset screen

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

## 3. HamloComp

Each recipe sets MODEL (VCA clean / FET fast and gritty / OPTO smooth, program-dependent / TUBE warm), SOURCE (DRUMS
for the drum groups, SAMPLES for bass, keys and samples, VOC for vocals, ALL for the master), ratio, knee, attack,
release (tempo notes as above), detector (RMS for leveling, peak for drums) and MIX; AUTO MAKEUP stays on (the preset
base). **THRESHOLD is calibrated** on the material, as the 35 presets of the presets pass were
(docs/COMPRESSOR_MODELS.md section 4): `HamloComp_PresetLab calibrate-genre` finds, per preset, the threshold that
gives the middle of its depth's band of average gain reduction (GENTLE 1 .. 3, MEDIUM 3 .. 6, HEAVY 6 .. 12 dB) on
its group's clips at -18 dBFS RMS (mixes -14); `Tools/genre_presets.py` writes it in. `check` then judges every
preset (P1 .. P5: band, peak, level change, unique): docs/reports/presets/genre_presets_check.md - 200 / 200 pass.

Disclosed: after the first calibration and check, 10 presets failed and were changed, then recalibrated with the
same method: Rage Crack, Hard Clap Smash and Neuro Snare Slam lost more than 12 dB of level (-14 dB), and became
parallel (MIX 60 %); ATL Piano Level, Minimal Keys Level, Liquid Rhodes Smooth, Acid Hat Grit, Peak Perc Pop,
Detroit Warm Master, Liquid Warm Master and Liquid Piano Breath had every value equal to a preset of another genre and
got their own attack / release. Material added to the tool: perc (perc.wav) and samples (guitar, synth, keys2).

## Sources

1. [Mastering The Mix: compression settings for a punchy mix](https://www.masteringthemix.com/blogs/learn/best-compression-settings-for-a-punchy-mix), [ProSoundWeb: five steps to punchy drum compression](https://www.prosoundweb.com/five-steps-to-punchy-drum-compression/), [Sonarworks: drum compression tips](https://www.sonarworks.com/blog/learn/killer-drum-tracks-compression-tips), [EDMProd: three ways to compress drums](https://www.edmprod.com/three-different-ways-to-compress-drums/)
2. [Sage Audio: how to mix trap](https://www.sageaudio.com/articles/how-to-mix-trap), [Violet Recording: mixing trap drums](https://violetrecording.com/how-to-mix-trap-drums/), [Rys Up Audio: trap vocals](https://rysupaudio.com/blogs/news/how-to-mix-trap-vocals), [Antares: vocals in dense trap beats](https://www.antarestech.com/blog/how-to-make-your-vocals-sit-in-dense-hip-hop-trap-beats)
3. [Attack Magazine: dark techno rumble](https://www.attackmagazine.com/technique/beat-dissected/dark-techno-rumble/), [MusicRadar: rumbling techno kick](https://musicradar.com/how-to/rumbling-techno-kick)
4. [MusicRadar: clean up and enhance a sampled breakbeat](https://musicradar.com/tuition/tech/how-to-clean-up-and-enhance-a-sampled-breakbeat-621106), [Gearspace: cracking those transients](https://gearspace.com/threads/cracking-those-transients.544384/)
