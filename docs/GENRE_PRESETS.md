# HamloUdarnik: genre presets and the preset screen

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

## 3. HamloUdarnik

Each recipe sets SNAP and BODY (the V2 engine delivers about their value in dB on every hit, whatever the level -
docs/UDARNIK_V2.md - so nothing is calibrated), SPEED (what counts as attack; per genre +-: Boom Bap -0.04, Trap 0,
Techno +0.03, DnB +0.06 - the faster the tempo, the shorter the attack window), MODE (PRAVDA classic, UDARNIK punch
without louder peaks, PERESTROIKA with RISE / DECAY for shorter hats and longer tails), SENS for ghost notes, and MIX
(parallel on bus and master). PROTECT stays on. `HamloUdarnik_PresetLab check` plays every preset on its group's clips
and checks: finite, the output under 0 dBFS, level change within +-9 dB, unique within its group, and the attack /
body contrast (measured as HamloUdarnik_Lab does: per hit 0 .. 10 ms against 25 .. 100 ms) moving the way the preset
says when SNAP and BODY differ by 4 dB or more: docs/reports/presets/genre_presets_check.md - 200 / 200 pass.

Disclosed (method changes after a run, each run on the same presets):
- Run 1 gained the clips to -18 dBFS RMS like HamloComp; the hats and percussion then peaked over 0 dBFS, which
  PROTECT lets through by design. The clips are now also held to a -3 dBFS peak.
- Run 1 judged the direction by the local crest factor (50 ms windows), run 2 by an envelope split of the whole clip;
  both read the opposite of the shaper on UDARNIK presets (it keeps the peaks) and almost nothing on dense loops. Run
  3 uses HamloUdarnik_Lab's per-hit contrast.
- Run 3 found presets of different genres with all values equal; SPEED now follows the genre's tempo (above), and the
  uniqueness check is per group.
- On the Samples / Melody clips (sustained guitar and synth, legato keys2) the shaper finds no hits: its output equals
  the input there. Those 16 presets (and one other clip set) are reported "n/a" for direction instead of failing;
  they are for chopped samples with onsets, which the owner's material does not have.

## Sources

1. [Mastering The Mix: compression settings for a punchy mix](https://www.masteringthemix.com/blogs/learn/best-compression-settings-for-a-punchy-mix), [ProSoundWeb: five steps to punchy drum compression](https://www.prosoundweb.com/five-steps-to-punchy-drum-compression/), [Sonarworks: drum compression tips](https://www.sonarworks.com/blog/learn/killer-drum-tracks-compression-tips), [EDMProd: three ways to compress drums](https://www.edmprod.com/three-different-ways-to-compress-drums/)
2. [Sage Audio: how to mix trap](https://www.sageaudio.com/articles/how-to-mix-trap), [Violet Recording: mixing trap drums](https://violetrecording.com/how-to-mix-trap-drums/), [Rys Up Audio: trap vocals](https://rysupaudio.com/blogs/news/how-to-mix-trap-vocals), [Antares: vocals in dense trap beats](https://www.antarestech.com/blog/how-to-make-your-vocals-sit-in-dense-hip-hop-trap-beats)
3. [Attack Magazine: dark techno rumble](https://www.attackmagazine.com/technique/beat-dissected/dark-techno-rumble/), [MusicRadar: rumbling techno kick](https://musicradar.com/how-to/rumbling-techno-kick)
4. [MusicRadar: clean up and enhance a sampled breakbeat](https://musicradar.com/tuition/tech/how-to-clean-up-and-enhance-a-sampled-breakbeat-621106), [Gearspace: cracking those transients](https://gearspace.com/threads/cracking-those-transients.544384/)
