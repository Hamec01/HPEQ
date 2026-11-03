# Genre preset library

HamloZastava 0.2.0: 200 new recipes after the original seven host programs.
Four genres, 50 recipes each: 16 Clean, 14 Drum, 8 Duck, 12 Rhythm.

Reference BPM: Boom Bap 88, Trap 144, Techno 132, DnB 174. Synced patterns
follow the host tempo. Free patterns retain milliseconds. Duck requires a kick
routed to External Key. All recipes use full processing Mix and unity Output;
adapt Threshold to the recording. Loading a recipe clears key audition.

## Measured depth

| Role | Gentle | Medium | Strong | Other acceptance |
|---|---|---|---|---|
| Clean pause rejection | 4–12 dB | 12–24 dB | 24–48 dB | Wanted loss ≤1 dB; quiet detail ≤2 dB; first20ms ≤3 dB |
| Drum post-envelope rejection | 4–12 dB | 12–24 dB | 24–48 dB | Exactly8onsets; first20ms loss ≤1 dB |
| Duck key-active attenuation | 1–3 dB | 3–6 dB | 6–12 dB | Recovery loss ≤1.5 dB |
| Rhythm full/floor contrast | 4–12 dB | 12–24 dB | 24–48 dB | Full-step loss ≤0.2 dB |

Targets use controlled arrangements constructed from owner snippets. Separate
natural excerpts check peak/finite output for every recipe. These checks do not
certify correct automatic threshold choices on every recording. All200compiled
recipes undergo fresh/edited project restoration with bit-identical audio.
See [methods](S5_METHODS.md) and [release evidence](reports/s5/REPORT.md).

IDs are permanent; host programs are0-based. Every recipe differs in active
processing values or pattern, beyond Mix/Output and reference BPM.
The JSON catalog is authoritative; export the C++ header after edits. Calibration
proposals are not shipping evidence: rebuild and run `check` without overrides.

## Catalog

### Boom Bap

| Program / permanent ID | Name | Role / depth | Material | Purpose |
|---|---|---|---|---|
| 7 / `za.bb.clean.01` | BB Close Voice | Clean / Gentle | vocal_lead1.wav | Preserve close voice detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 8 / `za.bb.clean.02` | BB Breath Keeper | Clean / Gentle | vocal_lead2.wav | Preserve breath keeper detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 9 / `za.bb.clean.03` | BB Verse Cuts | Clean / Strong | vocal_lead3.wav | Preserve verse cuts detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 10 / `za.bb.clean.04` | BB Backing Pocket | Clean / Medium | vocal_back1.wav | Preserve backing pocket detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 11 / `za.bb.clean.05` | BB Stack Hush | Clean / Strong | vocal_back2.wav | Preserve stack hush detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 12 / `za.bb.clean.06` | BB Airy Harmony | Clean / Gentle | vocal_back3.wav | Preserve airy harmony detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 13 / `za.bb.clean.07` | BB Guitar Stops | Clean / Strong | guitar.wav | Preserve guitar stops detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 14 / `za.bb.clean.08` | BB String Decay | Clean / Gentle | guitar.wav | Preserve string decay detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 15 / `za.bb.clean.09` | BB Key Room | Clean / Medium | keys.wav | Preserve key room detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 16 / `za.bb.clean.10` | BB Chop Floor | Clean / Strong | keys2.wav | Preserve chop floor detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 17 / `za.bb.clean.11` | BB Bass Pause | Clean / Medium | bass1.wav | Preserve bass pause detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 18 / `za.bb.clean.12` | BB Sub Reset | Clean / Strong | bass2.wav | Preserve sub reset detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 19 / `za.bb.clean.13` | BB Brush Bus | Clean / Gentle | drumbus1.wav | Preserve brush bus detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 20 / `za.bb.clean.14` | BB Tight Room | Clean / Strong | drumbus1.wav | Preserve tight room detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 21 / `za.bb.clean.15` | BB Silk Lead | Clean / Medium | vocal_lead1.wav | Preserve silk lead detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 22 / `za.bb.clean.16` | BB Held Sample | Clean / Medium | keys2.wav | Preserve held sample detail while lowering pauses in Boom Bap; adapt threshold to the recording. |
| 23 / `za.bb.drum.01` | BB Kick Frame | Drum / Medium | kick.wav | Boom Bap kick frame: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 24 / `za.bb.drum.02` | BB Kick Cushion | Drum / Gentle | kick.wav | Boom Bap kick cushion: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 25 / `za.bb.drum.03` | BB Kick Staccato | Drum / Strong | kick.wav | Boom Bap kick staccato: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 26 / `za.bb.drum.04` | BB Sub Body | Drum / Medium | kick.wav | Boom Bap sub body: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 27 / `za.bb.drum.05` | BB Snare Rim | Drum / Strong | snare.wav | Boom Bap snare rim: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 28 / `za.bb.drum.06` | BB Snare Velvet | Drum / Gentle | snare.wav | Boom Bap snare velvet: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 29 / `za.bb.drum.07` | BB Snare Pocket | Drum / Medium | snare.wav | Boom Bap snare pocket: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 30 / `za.bb.drum.08` | BB Clap Snap | Drum / Strong | snare.wav | Boom Bap clap snap: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 31 / `za.bb.drum.09` | BB Hat Lace | Drum / Gentle | hihat.wav | Boom Bap hat lace: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 32 / `za.bb.drum.10` | BB Hat Blips | Drum / Strong | hihat.wav | Boom Bap hat blips: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 33 / `za.bb.drum.11` | BB Hat Dust | Drum / Medium | hihat.wav | Boom Bap hat dust: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 34 / `za.bb.drum.12` | BB Perc Flecks | Drum / Strong | perc.wav | Boom Bap perc flecks: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 35 / `za.bb.drum.13` | BB Perc Swing | Drum / Medium | perc.wav | Boom Bap perc swing: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 36 / `za.bb.drum.14` | BB Perc Bloom | Drum / Gentle | perc.wav | Boom Bap perc bloom: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 37 / `za.bb.duck.01` | BB Bass Step | Duck / Medium | bass1.wav | Route a kick to External Key; make bass1 yield and recover in the Boom Bap pocket at about 88 BPM. |
| 38 / `za.bb.duck.02` | BB Sub Handoff | Duck / Strong | bass2.wav | Route a kick to External Key; make bass2 yield and recover in the Boom Bap pocket at about 88 BPM. |
| 39 / `za.bb.duck.03` | BB Keys Breathe | Duck / Gentle | keys.wav | Route a kick to External Key; make keys yield and recover in the Boom Bap pocket at about 88 BPM. |
| 40 / `za.bb.duck.04` | BB Sample Lift | Duck / Medium | keys2.wav | Route a kick to External Key; make keys2 yield and recover in the Boom Bap pocket at about 88 BPM. |
| 41 / `za.bb.duck.05` | BB Synth Pump | Duck / Strong | synth.wav | Route a kick to External Key; make synth yield and recover in the Boom Bap pocket at about 88 BPM. |
| 42 / `za.bb.duck.06` | BB Soft Pad Wake | Duck / Gentle | synth.wav | Route a kick to External Key; make synth yield and recover in the Boom Bap pocket at about 88 BPM. |
| 43 / `za.bb.duck.07` | BB Voice Space | Duck / Gentle | vocal_lead1.wav | Route a kick to External Key; make vocal_lead1 yield and recover in the Boom Bap pocket at about 88 BPM. |
| 44 / `za.bb.duck.08` | BB Stack Dip | Duck / Medium | vocal_back1.wav | Route a kick to External Key; make vocal_back1 yield and recover in the Boom Bap pocket at about 88 BPM. |
| 45 / `za.bb.rhythm.01` | BB Quarter Posts | Rhythm / Gentle | synth.wav | Boom Bap quarter posts texture for synth; editable 16-step accents, floor and full openings. |
| 46 / `za.bb.rhythm.02` | BB Offbeat Mesh | Rhythm / Medium | keys.wav | Boom Bap offbeat mesh texture for keys; editable 16-step accents, floor and full openings. |
| 47 / `za.bb.rhythm.03` | BB Broken Glass | Rhythm / Strong | keys2.wav | Boom Bap broken glass texture for keys2; editable 16-step accents, floor and full openings. |
| 48 / `za.bb.rhythm.04` | BB Triplet Hint | Rhythm / Gentle | synth.wav | Boom Bap triplet hint texture for synth; editable 32-step accents, floor and full openings. |
| 49 / `za.bb.rhythm.05` | BB Voice Chops | Rhythm / Medium | vocal_lead1.wav | Boom Bap voice chops texture for vocal_lead1; editable 16-step accents, floor and full openings. |
| 50 / `za.bb.rhythm.06` | BB Half Windows | Rhythm / Strong | synth.wav | Boom Bap half windows texture for synth; editable 16-step accents, floor and full openings. |
| 51 / `za.bb.rhythm.07` | BB Double Teeth | Rhythm / Gentle | keys.wav | Boom Bap double teeth texture for keys; editable 16-step accents, floor and full openings. |
| 52 / `za.bb.rhythm.08` | BB Late Answer | Rhythm / Medium | vocal_back1.wav | Boom Bap late answer texture for vocal_back1; editable 32-step accents, floor and full openings. |
| 53 / `za.bb.rhythm.09` | BB Small Echoes | Rhythm / Strong | keys2.wav | Boom Bap small echoes texture for keys2; editable 16-step accents, floor and full openings. |
| 54 / `za.bb.rhythm.10` | BB Long Lanes | Rhythm / Gentle | synth.wav | Boom Bap long lanes texture for synth; editable 32-step accents, floor and full openings. |
| 55 / `za.bb.rhythm.11` | BB Syllable Lace | Rhythm / Medium | vocal_lead2.wav | Boom Bap syllable lace texture for vocal_lead2; editable 16-step accents, floor and full openings. |
| 56 / `za.bb.rhythm.12` | BB Open Weave | Rhythm / Strong | synth.wav | Boom Bap open weave texture for synth; editable 32-step accents, floor and full openings. |

### Trap

| Program / permanent ID | Name | Role / depth | Material | Purpose |
|---|---|---|---|---|
| 57 / `za.tr.clean.01` | TR Close Voice | Clean / Gentle | vocal_lead1.wav | Preserve close voice detail while lowering pauses in Trap; adapt threshold to the recording. |
| 58 / `za.tr.clean.02` | TR Breath Keeper | Clean / Gentle | vocal_lead2.wav | Preserve breath keeper detail while lowering pauses in Trap; adapt threshold to the recording. |
| 59 / `za.tr.clean.03` | TR Verse Cuts | Clean / Strong | vocal_lead3.wav | Preserve verse cuts detail while lowering pauses in Trap; adapt threshold to the recording. |
| 60 / `za.tr.clean.04` | TR Backing Pocket | Clean / Medium | vocal_back1.wav | Preserve backing pocket detail while lowering pauses in Trap; adapt threshold to the recording. |
| 61 / `za.tr.clean.05` | TR Stack Hush | Clean / Strong | vocal_back2.wav | Preserve stack hush detail while lowering pauses in Trap; adapt threshold to the recording. |
| 62 / `za.tr.clean.06` | TR Airy Harmony | Clean / Gentle | vocal_back3.wav | Preserve airy harmony detail while lowering pauses in Trap; adapt threshold to the recording. |
| 63 / `za.tr.clean.07` | TR Guitar Stops | Clean / Strong | guitar.wav | Preserve guitar stops detail while lowering pauses in Trap; adapt threshold to the recording. |
| 64 / `za.tr.clean.08` | TR String Decay | Clean / Gentle | guitar.wav | Preserve string decay detail while lowering pauses in Trap; adapt threshold to the recording. |
| 65 / `za.tr.clean.09` | TR Key Room | Clean / Medium | keys.wav | Preserve key room detail while lowering pauses in Trap; adapt threshold to the recording. |
| 66 / `za.tr.clean.10` | TR Chop Floor | Clean / Strong | keys2.wav | Preserve chop floor detail while lowering pauses in Trap; adapt threshold to the recording. |
| 67 / `za.tr.clean.11` | TR Bass Pause | Clean / Medium | bass1.wav | Preserve bass pause detail while lowering pauses in Trap; adapt threshold to the recording. |
| 68 / `za.tr.clean.12` | TR Sub Reset | Clean / Strong | bass2.wav | Preserve sub reset detail while lowering pauses in Trap; adapt threshold to the recording. |
| 69 / `za.tr.clean.13` | TR Brush Bus | Clean / Gentle | drumbus1.wav | Preserve brush bus detail while lowering pauses in Trap; adapt threshold to the recording. |
| 70 / `za.tr.clean.14` | TR Tight Room | Clean / Strong | drumbus1.wav | Preserve tight room detail while lowering pauses in Trap; adapt threshold to the recording. |
| 71 / `za.tr.clean.15` | TR Silk Lead | Clean / Medium | vocal_lead1.wav | Preserve silk lead detail while lowering pauses in Trap; adapt threshold to the recording. |
| 72 / `za.tr.clean.16` | TR Held Sample | Clean / Medium | keys2.wav | Preserve held sample detail while lowering pauses in Trap; adapt threshold to the recording. |
| 73 / `za.tr.drum.01` | TR Kick Frame | Drum / Medium | kick.wav | Trap kick frame: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 74 / `za.tr.drum.02` | TR Kick Cushion | Drum / Gentle | kick.wav | Trap kick cushion: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 75 / `za.tr.drum.03` | TR Kick Staccato | Drum / Strong | kick.wav | Trap kick staccato: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 76 / `za.tr.drum.04` | TR Sub Body | Drum / Medium | kick.wav | Trap sub body: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 77 / `za.tr.drum.05` | TR Snare Rim | Drum / Strong | snare.wav | Trap snare rim: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 78 / `za.tr.drum.06` | TR Snare Velvet | Drum / Gentle | snare.wav | Trap snare velvet: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 79 / `za.tr.drum.07` | TR Snare Pocket | Drum / Medium | snare.wav | Trap snare pocket: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 80 / `za.tr.drum.08` | TR Clap Snap | Drum / Strong | snare.wav | Trap clap snap: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 81 / `za.tr.drum.09` | TR Hat Lace | Drum / Gentle | hihat.wav | Trap hat lace: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 82 / `za.tr.drum.10` | TR Hat Blips | Drum / Strong | hihat.wav | Trap hat blips: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 83 / `za.tr.drum.11` | TR Hat Dust | Drum / Medium | hihat.wav | Trap hat dust: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 84 / `za.tr.drum.12` | TR Perc Flecks | Drum / Strong | perc.wav | Trap perc flecks: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 85 / `za.tr.drum.13` | TR Perc Swing | Drum / Medium | perc.wav | Trap perc swing: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 86 / `za.tr.drum.14` | TR Perc Bloom | Drum / Gentle | perc.wav | Trap perc bloom: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 87 / `za.tr.duck.01` | TR Bass Step | Duck / Medium | bass1.wav | Route a kick to External Key; make bass1 yield and recover in the Trap pocket at about 144 BPM. |
| 88 / `za.tr.duck.02` | TR Sub Handoff | Duck / Strong | bass2.wav | Route a kick to External Key; make bass2 yield and recover in the Trap pocket at about 144 BPM. |
| 89 / `za.tr.duck.03` | TR Keys Breathe | Duck / Gentle | keys.wav | Route a kick to External Key; make keys yield and recover in the Trap pocket at about 144 BPM. |
| 90 / `za.tr.duck.04` | TR Sample Lift | Duck / Medium | keys2.wav | Route a kick to External Key; make keys2 yield and recover in the Trap pocket at about 144 BPM. |
| 91 / `za.tr.duck.05` | TR Synth Pump | Duck / Strong | synth.wav | Route a kick to External Key; make synth yield and recover in the Trap pocket at about 144 BPM. |
| 92 / `za.tr.duck.06` | TR Soft Pad Wake | Duck / Gentle | synth.wav | Route a kick to External Key; make synth yield and recover in the Trap pocket at about 144 BPM. |
| 93 / `za.tr.duck.07` | TR Voice Space | Duck / Gentle | vocal_lead1.wav | Route a kick to External Key; make vocal_lead1 yield and recover in the Trap pocket at about 144 BPM. |
| 94 / `za.tr.duck.08` | TR Stack Dip | Duck / Medium | vocal_back1.wav | Route a kick to External Key; make vocal_back1 yield and recover in the Trap pocket at about 144 BPM. |
| 95 / `za.tr.rhythm.01` | TR Quarter Posts | Rhythm / Gentle | synth.wav | Trap quarter posts texture for synth; editable 16-step accents, floor and full openings. |
| 96 / `za.tr.rhythm.02` | TR Offbeat Mesh | Rhythm / Medium | keys.wav | Trap offbeat mesh texture for keys; editable 16-step accents, floor and full openings. |
| 97 / `za.tr.rhythm.03` | TR Broken Glass | Rhythm / Strong | keys2.wav | Trap broken glass texture for keys2; editable 16-step accents, floor and full openings. |
| 98 / `za.tr.rhythm.04` | TR Triplet Hint | Rhythm / Gentle | synth.wav | Trap triplet hint texture for synth; editable 32-step accents, floor and full openings. |
| 99 / `za.tr.rhythm.05` | TR Voice Chops | Rhythm / Medium | vocal_lead1.wav | Trap voice chops texture for vocal_lead1; editable 16-step accents, floor and full openings. |
| 100 / `za.tr.rhythm.06` | TR Half Windows | Rhythm / Strong | synth.wav | Trap half windows texture for synth; editable 16-step accents, floor and full openings. |
| 101 / `za.tr.rhythm.07` | TR Double Teeth | Rhythm / Gentle | keys.wav | Trap double teeth texture for keys; editable 16-step accents, floor and full openings. |
| 102 / `za.tr.rhythm.08` | TR Late Answer | Rhythm / Medium | vocal_back1.wav | Trap late answer texture for vocal_back1; editable 32-step accents, floor and full openings. |
| 103 / `za.tr.rhythm.09` | TR Small Echoes | Rhythm / Strong | keys2.wav | Trap small echoes texture for keys2; editable 16-step accents, floor and full openings. |
| 104 / `za.tr.rhythm.10` | TR Long Lanes | Rhythm / Gentle | synth.wav | Trap long lanes texture for synth; editable 32-step accents, floor and full openings. |
| 105 / `za.tr.rhythm.11` | TR Syllable Lace | Rhythm / Medium | vocal_lead2.wav | Trap syllable lace texture for vocal_lead2; editable 16-step accents, floor and full openings. |
| 106 / `za.tr.rhythm.12` | TR Open Weave | Rhythm / Strong | synth.wav | Trap open weave texture for synth; editable 32-step accents, floor and full openings. |

### Techno

| Program / permanent ID | Name | Role / depth | Material | Purpose |
|---|---|---|---|---|
| 107 / `za.tc.clean.01` | TC Close Voice | Clean / Gentle | vocal_lead1.wav | Preserve close voice detail while lowering pauses in Techno; adapt threshold to the recording. |
| 108 / `za.tc.clean.02` | TC Breath Keeper | Clean / Gentle | vocal_lead2.wav | Preserve breath keeper detail while lowering pauses in Techno; adapt threshold to the recording. |
| 109 / `za.tc.clean.03` | TC Verse Cuts | Clean / Strong | vocal_lead3.wav | Preserve verse cuts detail while lowering pauses in Techno; adapt threshold to the recording. |
| 110 / `za.tc.clean.04` | TC Backing Pocket | Clean / Medium | vocal_back1.wav | Preserve backing pocket detail while lowering pauses in Techno; adapt threshold to the recording. |
| 111 / `za.tc.clean.05` | TC Stack Hush | Clean / Strong | vocal_back2.wav | Preserve stack hush detail while lowering pauses in Techno; adapt threshold to the recording. |
| 112 / `za.tc.clean.06` | TC Airy Harmony | Clean / Gentle | vocal_back3.wav | Preserve airy harmony detail while lowering pauses in Techno; adapt threshold to the recording. |
| 113 / `za.tc.clean.07` | TC Guitar Stops | Clean / Strong | guitar.wav | Preserve guitar stops detail while lowering pauses in Techno; adapt threshold to the recording. |
| 114 / `za.tc.clean.08` | TC String Decay | Clean / Gentle | guitar.wav | Preserve string decay detail while lowering pauses in Techno; adapt threshold to the recording. |
| 115 / `za.tc.clean.09` | TC Key Room | Clean / Medium | keys.wav | Preserve key room detail while lowering pauses in Techno; adapt threshold to the recording. |
| 116 / `za.tc.clean.10` | TC Chop Floor | Clean / Strong | keys2.wav | Preserve chop floor detail while lowering pauses in Techno; adapt threshold to the recording. |
| 117 / `za.tc.clean.11` | TC Bass Pause | Clean / Medium | bass1.wav | Preserve bass pause detail while lowering pauses in Techno; adapt threshold to the recording. |
| 118 / `za.tc.clean.12` | TC Sub Reset | Clean / Strong | bass2.wav | Preserve sub reset detail while lowering pauses in Techno; adapt threshold to the recording. |
| 119 / `za.tc.clean.13` | TC Brush Bus | Clean / Gentle | drumbus1.wav | Preserve brush bus detail while lowering pauses in Techno; adapt threshold to the recording. |
| 120 / `za.tc.clean.14` | TC Tight Room | Clean / Strong | drumbus1.wav | Preserve tight room detail while lowering pauses in Techno; adapt threshold to the recording. |
| 121 / `za.tc.clean.15` | TC Silk Lead | Clean / Medium | vocal_lead1.wav | Preserve silk lead detail while lowering pauses in Techno; adapt threshold to the recording. |
| 122 / `za.tc.clean.16` | TC Held Sample | Clean / Medium | keys2.wav | Preserve held sample detail while lowering pauses in Techno; adapt threshold to the recording. |
| 123 / `za.tc.drum.01` | TC Kick Frame | Drum / Medium | kick.wav | Techno kick frame: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 124 / `za.tc.drum.02` | TC Kick Cushion | Drum / Gentle | kick.wav | Techno kick cushion: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 125 / `za.tc.drum.03` | TC Kick Staccato | Drum / Strong | kick.wav | Techno kick staccato: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 126 / `za.tc.drum.04` | TC Sub Body | Drum / Medium | kick.wav | Techno sub body: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 127 / `za.tc.drum.05` | TC Snare Rim | Drum / Strong | snare.wav | Techno snare rim: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 128 / `za.tc.drum.06` | TC Snare Velvet | Drum / Gentle | snare.wav | Techno snare velvet: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 129 / `za.tc.drum.07` | TC Snare Pocket | Drum / Medium | snare.wav | Techno snare pocket: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 130 / `za.tc.drum.08` | TC Clap Snap | Drum / Strong | snare.wav | Techno clap snap: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 131 / `za.tc.drum.09` | TC Hat Lace | Drum / Gentle | hihat.wav | Techno hat lace: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 132 / `za.tc.drum.10` | TC Hat Blips | Drum / Strong | hihat.wav | Techno hat blips: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 133 / `za.tc.drum.11` | TC Hat Dust | Drum / Medium | hihat.wav | Techno hat dust: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 134 / `za.tc.drum.12` | TC Perc Flecks | Drum / Strong | perc.wav | Techno perc flecks: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 135 / `za.tc.drum.13` | TC Perc Swing | Drum / Medium | perc.wav | Techno perc swing: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 136 / `za.tc.drum.14` | TC Perc Bloom | Drum / Gentle | perc.wav | Techno perc bloom: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 137 / `za.tc.duck.01` | TC Bass Step | Duck / Medium | bass1.wav | Route a kick to External Key; make bass1 yield and recover in the Techno pocket at about 132 BPM. |
| 138 / `za.tc.duck.02` | TC Sub Handoff | Duck / Strong | bass2.wav | Route a kick to External Key; make bass2 yield and recover in the Techno pocket at about 132 BPM. |
| 139 / `za.tc.duck.03` | TC Keys Breathe | Duck / Gentle | keys.wav | Route a kick to External Key; make keys yield and recover in the Techno pocket at about 132 BPM. |
| 140 / `za.tc.duck.04` | TC Sample Lift | Duck / Medium | keys2.wav | Route a kick to External Key; make keys2 yield and recover in the Techno pocket at about 132 BPM. |
| 141 / `za.tc.duck.05` | TC Synth Pump | Duck / Strong | synth.wav | Route a kick to External Key; make synth yield and recover in the Techno pocket at about 132 BPM. |
| 142 / `za.tc.duck.06` | TC Soft Pad Wake | Duck / Gentle | synth.wav | Route a kick to External Key; make synth yield and recover in the Techno pocket at about 132 BPM. |
| 143 / `za.tc.duck.07` | TC Voice Space | Duck / Gentle | vocal_lead1.wav | Route a kick to External Key; make vocal_lead1 yield and recover in the Techno pocket at about 132 BPM. |
| 144 / `za.tc.duck.08` | TC Stack Dip | Duck / Medium | vocal_back1.wav | Route a kick to External Key; make vocal_back1 yield and recover in the Techno pocket at about 132 BPM. |
| 145 / `za.tc.rhythm.01` | TC Quarter Posts | Rhythm / Gentle | synth.wav | Techno quarter posts texture for synth; editable 16-step accents, floor and full openings. |
| 146 / `za.tc.rhythm.02` | TC Offbeat Mesh | Rhythm / Medium | keys.wav | Techno offbeat mesh texture for keys; editable 16-step accents, floor and full openings. |
| 147 / `za.tc.rhythm.03` | TC Broken Glass | Rhythm / Strong | keys2.wav | Techno broken glass texture for keys2; editable 16-step accents, floor and full openings. |
| 148 / `za.tc.rhythm.04` | TC Triplet Hint | Rhythm / Gentle | synth.wav | Techno triplet hint texture for synth; editable 32-step accents, floor and full openings. |
| 149 / `za.tc.rhythm.05` | TC Voice Chops | Rhythm / Medium | vocal_lead1.wav | Techno voice chops texture for vocal_lead1; editable 16-step accents, floor and full openings. |
| 150 / `za.tc.rhythm.06` | TC Half Windows | Rhythm / Strong | synth.wav | Techno half windows texture for synth; editable 16-step accents, floor and full openings. |
| 151 / `za.tc.rhythm.07` | TC Double Teeth | Rhythm / Gentle | keys.wav | Techno double teeth texture for keys; editable 16-step accents, floor and full openings. |
| 152 / `za.tc.rhythm.08` | TC Late Answer | Rhythm / Medium | vocal_back1.wav | Techno late answer texture for vocal_back1; editable 32-step accents, floor and full openings. |
| 153 / `za.tc.rhythm.09` | TC Small Echoes | Rhythm / Strong | keys2.wav | Techno small echoes texture for keys2; editable 16-step accents, floor and full openings. |
| 154 / `za.tc.rhythm.10` | TC Long Lanes | Rhythm / Gentle | synth.wav | Techno long lanes texture for synth; editable 32-step accents, floor and full openings. |
| 155 / `za.tc.rhythm.11` | TC Syllable Lace | Rhythm / Medium | vocal_lead2.wav | Techno syllable lace texture for vocal_lead2; editable 16-step accents, floor and full openings. |
| 156 / `za.tc.rhythm.12` | TC Open Weave | Rhythm / Strong | synth.wav | Techno open weave texture for synth; editable 32-step accents, floor and full openings. |

### DnB

| Program / permanent ID | Name | Role / depth | Material | Purpose |
|---|---|---|---|---|
| 157 / `za.db.clean.01` | DB Close Voice | Clean / Gentle | vocal_lead1.wav | Preserve close voice detail while lowering pauses in DnB; adapt threshold to the recording. |
| 158 / `za.db.clean.02` | DB Breath Keeper | Clean / Gentle | vocal_lead2.wav | Preserve breath keeper detail while lowering pauses in DnB; adapt threshold to the recording. |
| 159 / `za.db.clean.03` | DB Verse Cuts | Clean / Strong | vocal_lead3.wav | Preserve verse cuts detail while lowering pauses in DnB; adapt threshold to the recording. |
| 160 / `za.db.clean.04` | DB Backing Pocket | Clean / Medium | vocal_back1.wav | Preserve backing pocket detail while lowering pauses in DnB; adapt threshold to the recording. |
| 161 / `za.db.clean.05` | DB Stack Hush | Clean / Strong | vocal_back2.wav | Preserve stack hush detail while lowering pauses in DnB; adapt threshold to the recording. |
| 162 / `za.db.clean.06` | DB Airy Harmony | Clean / Gentle | vocal_back3.wav | Preserve airy harmony detail while lowering pauses in DnB; adapt threshold to the recording. |
| 163 / `za.db.clean.07` | DB Guitar Stops | Clean / Strong | guitar.wav | Preserve guitar stops detail while lowering pauses in DnB; adapt threshold to the recording. |
| 164 / `za.db.clean.08` | DB String Decay | Clean / Gentle | guitar.wav | Preserve string decay detail while lowering pauses in DnB; adapt threshold to the recording. |
| 165 / `za.db.clean.09` | DB Key Room | Clean / Medium | keys.wav | Preserve key room detail while lowering pauses in DnB; adapt threshold to the recording. |
| 166 / `za.db.clean.10` | DB Chop Floor | Clean / Strong | keys2.wav | Preserve chop floor detail while lowering pauses in DnB; adapt threshold to the recording. |
| 167 / `za.db.clean.11` | DB Bass Pause | Clean / Medium | bass1.wav | Preserve bass pause detail while lowering pauses in DnB; adapt threshold to the recording. |
| 168 / `za.db.clean.12` | DB Sub Reset | Clean / Strong | bass2.wav | Preserve sub reset detail while lowering pauses in DnB; adapt threshold to the recording. |
| 169 / `za.db.clean.13` | DB Brush Bus | Clean / Gentle | drumbus1.wav | Preserve brush bus detail while lowering pauses in DnB; adapt threshold to the recording. |
| 170 / `za.db.clean.14` | DB Tight Room | Clean / Strong | drumbus1.wav | Preserve tight room detail while lowering pauses in DnB; adapt threshold to the recording. |
| 171 / `za.db.clean.15` | DB Silk Lead | Clean / Medium | vocal_lead1.wav | Preserve silk lead detail while lowering pauses in DnB; adapt threshold to the recording. |
| 172 / `za.db.clean.16` | DB Held Sample | Clean / Medium | keys2.wav | Preserve held sample detail while lowering pauses in DnB; adapt threshold to the recording. |
| 173 / `za.db.drum.01` | DB Kick Frame | Drum / Medium | kick.wav | DnB kick frame: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 174 / `za.db.drum.02` | DB Kick Cushion | Drum / Gentle | kick.wav | DnB kick cushion: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 175 / `za.db.drum.03` | DB Kick Staccato | Drum / Strong | kick.wav | DnB kick staccato: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 176 / `za.db.drum.04` | DB Sub Body | Drum / Medium | kick.wav | DnB sub body: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 177 / `za.db.drum.05` | DB Snare Rim | Drum / Strong | snare.wav | DnB snare rim: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 178 / `za.db.drum.06` | DB Snare Velvet | Drum / Gentle | snare.wav | DnB snare velvet: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 179 / `za.db.drum.07` | DB Snare Pocket | Drum / Medium | snare.wav | DnB snare pocket: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 180 / `za.db.drum.08` | DB Clap Snap | Drum / Strong | snare.wav | DnB clap snap: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 181 / `za.db.drum.09` | DB Hat Lace | Drum / Gentle | hihat.wav | DnB hat lace: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 182 / `za.db.drum.10` | DB Hat Blips | Drum / Strong | hihat.wav | DnB hat blips: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 183 / `za.db.drum.11` | DB Hat Dust | Drum / Medium | hihat.wav | DnB hat dust: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 184 / `za.db.drum.12` | DB Perc Flecks | Drum / Strong | perc.wav | DnB perc flecks: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 185 / `za.db.drum.13` | DB Perc Swing | Drum / Medium | perc.wav | DnB perc swing: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 186 / `za.db.drum.14` | DB Perc Bloom | Drum / Gentle | perc.wav | DnB perc bloom: preserve the hit, hold its body and shorten the following tail; retain quieter hits. |
| 187 / `za.db.duck.01` | DB Bass Step | Duck / Medium | bass1.wav | Route a kick to External Key; make bass1 yield and recover in the DnB pocket at about 174 BPM. |
| 188 / `za.db.duck.02` | DB Sub Handoff | Duck / Strong | bass2.wav | Route a kick to External Key; make bass2 yield and recover in the DnB pocket at about 174 BPM. |
| 189 / `za.db.duck.03` | DB Keys Breathe | Duck / Gentle | keys.wav | Route a kick to External Key; make keys yield and recover in the DnB pocket at about 174 BPM. |
| 190 / `za.db.duck.04` | DB Sample Lift | Duck / Medium | keys2.wav | Route a kick to External Key; make keys2 yield and recover in the DnB pocket at about 174 BPM. |
| 191 / `za.db.duck.05` | DB Synth Pump | Duck / Strong | synth.wav | Route a kick to External Key; make synth yield and recover in the DnB pocket at about 174 BPM. |
| 192 / `za.db.duck.06` | DB Soft Pad Wake | Duck / Gentle | synth.wav | Route a kick to External Key; make synth yield and recover in the DnB pocket at about 174 BPM. |
| 193 / `za.db.duck.07` | DB Voice Space | Duck / Gentle | vocal_lead1.wav | Route a kick to External Key; make vocal_lead1 yield and recover in the DnB pocket at about 174 BPM. |
| 194 / `za.db.duck.08` | DB Stack Dip | Duck / Medium | vocal_back1.wav | Route a kick to External Key; make vocal_back1 yield and recover in the DnB pocket at about 174 BPM. |
| 195 / `za.db.rhythm.01` | DB Quarter Posts | Rhythm / Gentle | synth.wav | DnB quarter posts texture for synth; editable 16-step accents, floor and full openings. |
| 196 / `za.db.rhythm.02` | DB Offbeat Mesh | Rhythm / Medium | keys.wav | DnB offbeat mesh texture for keys; editable 16-step accents, floor and full openings. |
| 197 / `za.db.rhythm.03` | DB Broken Glass | Rhythm / Strong | keys2.wav | DnB broken glass texture for keys2; editable 16-step accents, floor and full openings. |
| 198 / `za.db.rhythm.04` | DB Triplet Hint | Rhythm / Gentle | synth.wav | DnB triplet hint texture for synth; editable 32-step accents, floor and full openings. |
| 199 / `za.db.rhythm.05` | DB Voice Chops | Rhythm / Medium | vocal_lead1.wav | DnB voice chops texture for vocal_lead1; editable 16-step accents, floor and full openings. |
| 200 / `za.db.rhythm.06` | DB Half Windows | Rhythm / Strong | synth.wav | DnB half windows texture for synth; editable 16-step accents, floor and full openings. |
| 201 / `za.db.rhythm.07` | DB Double Teeth | Rhythm / Gentle | keys.wav | DnB double teeth texture for keys; editable 16-step accents, floor and full openings. |
| 202 / `za.db.rhythm.08` | DB Late Answer | Rhythm / Medium | vocal_back1.wav | DnB late answer texture for vocal_back1; editable 32-step accents, floor and full openings. |
| 203 / `za.db.rhythm.09` | DB Small Echoes | Rhythm / Strong | keys2.wav | DnB small echoes texture for keys2; editable 16-step accents, floor and full openings. |
| 204 / `za.db.rhythm.10` | DB Long Lanes | Rhythm / Gentle | synth.wav | DnB long lanes texture for synth; editable 32-step accents, floor and full openings. |
| 205 / `za.db.rhythm.11` | DB Syllable Lace | Rhythm / Medium | vocal_lead2.wav | DnB syllable lace texture for vocal_lead2; editable 16-step accents, floor and full openings. |
| 206 / `za.db.rhythm.12` | DB Open Weave | Rhythm / Strong | synth.wav | DnB open weave texture for synth; editable 32-step accents, floor and full openings. |
