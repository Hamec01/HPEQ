# HamloVerb genre presets

**200/200 independently checked**, after complete calibration on the final DSP. Default and the seven original Factory programs remain unchanged (208 programs total).

Each genre has Vocal10, Drums12, Samples/Keys10, Synth8, Atmosphere/FX10.
The genre voicing and tempo derive from the owner's musical workflow; recipes are authored in Tools/genre_presets.py, not randomized.

Depth means complete-render wet/dry energy: Gentle -24..-16 dB, Medium -16..-8 dB, Heavy -8..+6 dB.
Calibration uses normalized owner clips, includes tails, and verifies actual Insert and Send renders, 8x true peak, state recall, finite output and unique sound settings.
MIX and Output alone do not count toward uniqueness. Cached calibration is rejected if source/DSP/material/recipe identity changes.

[Full independent check](reports/presets/check_all.md) · [Calibration](reports/presets/calibrate_all.md)

Material buttons filter only. LOAD START loads a recipe. Mix Lock preserves Insert MIX; routing survives preset loads; Send remains wet only.

| Program | ID | Genre | Group | Name | BPM | Depth | Insert MIX | Purpose |
|---:|---|---|---|---|---:|---|---:|---|
| 8 | bb-01 | Boom Bap | Vocal | BB Classic Close Booth | 90 | Gentle | 43.24% | Clean 0.8-beat tail, 90 BPM; vocal lead1 |
| 9 | bb-02 | Boom Bap | Vocal | BB Dusty Warm Lead | 90 | Medium | 58.67% | Warm 2.3-beat tail, 90 BPM; vocal lead2 |
| 10 | bb-03 | Boom Bap | Vocal | BB Jazzy Velvet Phrase | 90 | Medium | 65.53% | Velvet 3.1-beat tail, 90 BPM; vocal lead3 |
| 11 | bb-04 | Boom Bap | Vocal | BB Gold Bright Plate | 90 | Medium | 58.15% | Crisp 1.9-beat tail, 90 BPM; vocal lead1 |
| 12 | bb-05 | Boom Bap | Vocal | BB Underground Distant Reply | 90 | Heavy | 86.27% | Deep 4.2-beat tail, 90 BPM; vocal back1 |
| 13 | bb-06 | Boom Bap | Vocal | BB Lofi Breath Halo | 90 | Medium | 61.02% | Bloom 5-beat tail, 90 BPM; vocal back2; shimmer +12 st |
| 14 | bb-07 | Boom Bap | Vocal | BB Classic Hook Chamber | 90 | Gentle | 40.60% | Clean 1.5-beat tail, 90 BPM; vocal lead2 |
| 15 | bb-08 | Boom Bap | Vocal | BB Dusty Backing Silk | 90 | Medium | 59.95% | Velvet 3.5-beat tail, 90 BPM; vocal back3 |
| 16 | bb-09 | Boom Bap | Vocal | BB Jazzy Cut Phrase | 90 | Medium | 61.58% | Crisp 2-beat tail, 90 BPM; vocal lead3; gated |
| 17 | bb-10 | Boom Bap | Vocal | BB Gold Reverse Adlib | 90 | Heavy | 89.60% | Deep 2.7-beat tail, 90 BPM; vocal back1; reverse wet fragments |
| 18 | bb-11 | Boom Bap | Drums | BB Underground Snare Snap | 90 | Medium | 41.95% | Crisp 0.7-beat tail, 90 BPM; snare |
| 19 | bb-12 | Boom Bap | Drums | BB Lofi Snare Steel | 90 | Medium | 38.20% | Crisp 1.3-beat tail, 90 BPM; snare |
| 20 | bb-13 | Boom Bap | Drums | BB Classic Snare Velvet | 90 | Medium | 44.10% | Velvet 1.7-beat tail, 90 BPM; snare |
| 21 | bb-14 | Boom Bap | Drums | BB Dusty Gated Backbeat | 90 | Heavy | 65.52% | Crisp 2.1-beat tail, 90 BPM; snare; gated |
| 22 | bb-15 | Boom Bap | Drums | BB Jazzy Snare Uplift | 90 | Heavy | 88.04% | Crisp 1.8-beat tail, 90 BPM; snare; rising swell |
| 23 | bb-16 | Boom Bap | Drums | BB Gold Kick Room | 90 | Gentle | 73.44% | Clean 0.45-beat tail, 90 BPM; kick |
| 24 | bb-17 | Boom Bap | Drums | BB Underground Hat Air | 90 | Gentle | 23.37% | Clean 0.65-beat tail, 90 BPM; hihat |
| 25 | bb-18 | Boom Bap | Drums | BB Lofi Metal Wash | 90 | Medium | 45.13% | Velvet 2.4-beat tail, 90 BPM; hihat |
| 26 | bb-19 | Boom Bap | Drums | BB Classic Percussion Yard | 90 | Medium | 49.67% | Warm 1.1-beat tail, 90 BPM; perc |
| 27 | bb-20 | Boom Bap | Drums | BB Dusty Break Room | 90 | Gentle | 59.90% | Clean 0.85-beat tail, 90 BPM; drumbus1 |
| 28 | bb-21 | Boom Bap | Drums | BB Jazzy Drum Warehouse | 90 | Medium | 82.87% | Deep 2.8-beat tail, 90 BPM; drumbus1 |
| 29 | bb-22 | Boom Bap | Drums | BB Gold Reverse Break | 90 | Heavy | 92.04% | Crisp 1.5-beat tail, 90 BPM; drumbus1; reverse wet fragments |
| 30 | bb-23 | Boom Bap | Samples / Keys | BB Underground Piano Wood | 90 | Medium | 37.03% | Warm 2.3-beat tail, 90 BPM; keys |
| 31 | bb-24 | Boom Bap | Samples / Keys | BB Lofi Piano Glass | 90 | Medium | 34.55% | Clean 2.8-beat tail, 90 BPM; keys2 |
| 32 | bb-25 | Boom Bap | Samples / Keys | BB Classic Rhodes Silk | 90 | Medium | 29.08% | Velvet 3.4-beat tail, 90 BPM; keys |
| 33 | bb-26 | Boom Bap | Samples / Keys | BB Dusty Chopped Room | 90 | Gentle | 18.40% | Clean 0.9-beat tail, 90 BPM; keys2 |
| 34 | bb-27 | Boom Bap | Samples / Keys | BB Jazzy Dust Chamber | 90 | Gentle | 15.34% | Warm 1.7-beat tail, 90 BPM; guitar |
| 35 | bb-28 | Boom Bap | Samples / Keys | BB Gold Guitar Plate | 90 | Medium | 36.68% | Crisp 2.4-beat tail, 90 BPM; guitar |
| 36 | bb-29 | Boom Bap | Samples / Keys | BB Underground Keys Horizon | 90 | Heavy | 72.19% | Deep 4.8-beat tail, 90 BPM; keys2 |
| 37 | bb-30 | Boom Bap | Samples / Keys | BB Lofi Melody Bloom | 90 | Heavy | 66.07% | Bloom 5.5-beat tail, 90 BPM; keys |
| 38 | bb-31 | Boom Bap | Samples / Keys | BB Classic Reverse Chops | 90 | Heavy | 76.60% | Warm 2.5-beat tail, 90 BPM; guitar; reverse wet fragments |
| 39 | bb-32 | Boom Bap | Samples / Keys | BB Dusty Piano Octave | 90 | Medium | 33.92% | Velvet 4.2-beat tail, 90 BPM; keys2; shimmer +12 st |
| 40 | bb-33 | Boom Bap | Synth | BB Jazzy Pluck Space | 90 | Gentle | 15.43% | Clean 1.2-beat tail, 90 BPM; synth |
| 41 | bb-34 | Boom Bap | Synth | BB Gold Lead Plate | 90 | Medium | 32.16% | Crisp 2.1-beat tail, 90 BPM; synth |
| 42 | bb-35 | Boom Bap | Synth | BB Underground Warm Arp | 90 | Medium | 34.22% | Warm 2.7-beat tail, 90 BPM; synth |
| 43 | bb-36 | Boom Bap | Synth | BB Lofi Pad Velvet | 90 | Heavy | 61.71% | Velvet 5.2-beat tail, 90 BPM; synth |
| 44 | bb-37 | Boom Bap | Synth | BB Classic Deep Chord | 90 | Heavy | 66.47% | Deep 6.5-beat tail, 90 BPM; synth |
| 45 | bb-38 | Boom Bap | Synth | BB Dusty Bloom Fifth | 90 | Heavy | 64.87% | Bloom 6-beat tail, 90 BPM; synth; shimmer +7 st |
| 46 | bb-39 | Boom Bap | Synth | BB Jazzy Pulse Gate | 90 | Medium | 29.56% | Crisp 2-beat tail, 90 BPM; synth; gated |
| 47 | bb-40 | Boom Bap | Synth | BB Gold Reverse Sequence | 90 | Heavy | 75.45% | Deep 3.3-beat tail, 90 BPM; synth; reverse wet fragments |
| 48 | bb-41 | Boom Bap | Atmosphere / FX | BB Underground Endless Sky | 90 | Heavy | 62.16% | Bloom 12-beat tail, 90 BPM; keys2; shimmer +12 st |
| 49 | bb-42 | Boom Bap | Atmosphere / FX | BB Lofi Low Orbit | 90 | Heavy | 64.34% | Deep 8-beat tail, 90 BPM; synth; shimmer -12 st |
| 50 | bb-43 | Boom Bap | Atmosphere / FX | BB Classic Silver Cloud | 90 | Heavy | 55.72% | Velvet 9-beat tail, 90 BPM; vocal back2; shimmer +24 st |
| 51 | bb-44 | Boom Bap | Atmosphere / FX | BB Dusty Warm Fog | 90 | Heavy | 66.65% | Warm 7-beat tail, 90 BPM; guitar |
| 52 | bb-45 | Boom Bap | Atmosphere / FX | BB Jazzy Distant Station | 90 | Heavy | 63.19% | Deep 10-beat tail, 90 BPM; keys |
| 53 | bb-46 | Boom Bap | Atmosphere / FX | BB Gold Velvet Lift | 90 | Heavy | 82.70% | Velvet 6-beat tail, 90 BPM; vocal back3; rising swell |
| 54 | bb-47 | Boom Bap | Atmosphere / FX | BB Underground Reverse Horizon | 90 | Heavy | 70.44% | Bloom 8.5-beat tail, 90 BPM; keys2; reverse wet fragments |
| 55 | bb-48 | Boom Bap | Atmosphere / FX | BB Lofi Fractured Room | 90 | Medium | 36.17% | Clean 3.8-beat tail, 90 BPM; perc; gated |
| 56 | bb-49 | Boom Bap | Atmosphere / FX | BB Classic Octave Rain | 90 | Medium | 26.73% | Crisp 5-beat tail, 90 BPM; hihat; shimmer +12 st |
| 57 | bb-50 | Boom Bap | Atmosphere / FX | BB Dusty Afterglow | 90 | Heavy | 56.93% | Bloom 14-beat tail, 90 BPM; synth |
| 58 | trap-01 | Trap | Vocal | TRAP ATL Close Booth | 140 | Gentle | 46.14% | Clean 0.8-beat tail, 140 BPM; vocal lead1 |
| 59 | trap-02 | Trap | Vocal | TRAP Dark Warm Lead | 140 | Medium | 62.92% | Warm 2.3-beat tail, 140 BPM; vocal lead2 |
| 60 | trap-03 | Trap | Vocal | TRAP Cloud Velvet Phrase | 140 | Medium | 69.71% | Velvet 3.1-beat tail, 140 BPM; vocal lead3 |
| 61 | trap-04 | Trap | Vocal | TRAP Rage Bright Plate | 140 | Medium | 62.01% | Crisp 1.9-beat tail, 140 BPM; vocal lead1 |
| 62 | trap-05 | Trap | Vocal | TRAP Memphis Distant Reply | 140 | Heavy | 89.64% | Deep 4.2-beat tail, 140 BPM; vocal back1 |
| 63 | trap-06 | Trap | Vocal | TRAP Luxury Breath Halo | 140 | Medium | 67.16% | Bloom 5-beat tail, 140 BPM; vocal back2; shimmer +12 st |
| 64 | trap-07 | Trap | Vocal | TRAP ATL Hook Chamber | 140 | Gentle | 43.67% | Clean 1.5-beat tail, 140 BPM; vocal lead2 |
| 65 | trap-08 | Trap | Vocal | TRAP Dark Backing Silk | 140 | Medium | 64.69% | Velvet 3.5-beat tail, 140 BPM; vocal back3 |
| 66 | trap-09 | Trap | Vocal | TRAP Cloud Cut Phrase | 140 | Medium | 67.00% | Crisp 2-beat tail, 140 BPM; vocal lead3; gated |
| 67 | trap-10 | Trap | Vocal | TRAP Rage Reverse Adlib | 140 | Heavy | 92.27% | Deep 2.7-beat tail, 140 BPM; vocal back1; reverse wet fragments |
| 68 | trap-11 | Trap | Drums | TRAP Memphis Snare Snap | 140 | Medium | 44.45% | Crisp 0.7-beat tail, 140 BPM; snare |
| 69 | trap-12 | Trap | Drums | TRAP Luxury Snare Steel | 140 | Medium | 41.44% | Crisp 1.3-beat tail, 140 BPM; snare |
| 70 | trap-13 | Trap | Drums | TRAP ATL Snare Velvet | 140 | Medium | 47.18% | Velvet 1.7-beat tail, 140 BPM; snare |
| 71 | trap-14 | Trap | Drums | TRAP Dark Gated Backbeat | 140 | Heavy | 69.86% | Crisp 2.1-beat tail, 140 BPM; snare; gated |
| 72 | trap-15 | Trap | Drums | TRAP Cloud Snare Uplift | 140 | Heavy | 88.94% | Crisp 1.8-beat tail, 140 BPM; snare; rising swell |
| 73 | trap-16 | Trap | Drums | TRAP Rage Kick Room | 140 | Gentle | 74.59% | Clean 0.45-beat tail, 140 BPM; kick |
| 74 | trap-17 | Trap | Drums | TRAP Memphis Hat Air | 140 | Gentle | 23.88% | Clean 0.65-beat tail, 140 BPM; hihat |
| 75 | trap-18 | Trap | Drums | TRAP Luxury Metal Wash | 140 | Medium | 46.79% | Velvet 2.4-beat tail, 140 BPM; hihat |
| 76 | trap-19 | Trap | Drums | TRAP ATL Percussion Yard | 140 | Medium | 50.47% | Warm 1.1-beat tail, 140 BPM; perc |
| 77 | trap-20 | Trap | Drums | TRAP Dark Break Room | 140 | Gentle | 63.93% | Clean 0.85-beat tail, 140 BPM; drumbus1 |
| 78 | trap-21 | Trap | Drums | TRAP Cloud Drum Warehouse | 140 | Medium | 88.13% | Deep 2.8-beat tail, 140 BPM; drumbus1 |
| 79 | trap-22 | Trap | Drums | TRAP Rage Reverse Break | 140 | Heavy | 93.96% | Crisp 1.5-beat tail, 140 BPM; drumbus1; reverse wet fragments |
| 80 | trap-23 | Trap | Samples / Keys | TRAP Memphis Piano Wood | 140 | Medium | 40.82% | Warm 2.3-beat tail, 140 BPM; keys |
| 81 | trap-24 | Trap | Samples / Keys | TRAP Luxury Piano Glass | 140 | Medium | 41.20% | Clean 2.8-beat tail, 140 BPM; keys2 |
| 82 | trap-25 | Trap | Samples / Keys | TRAP ATL Rhodes Silk | 140 | Medium | 35.15% | Velvet 3.4-beat tail, 140 BPM; keys |
| 83 | trap-26 | Trap | Samples / Keys | TRAP Dark Chopped Room | 140 | Gentle | 26.11% | Clean 0.9-beat tail, 140 BPM; keys2 |
| 84 | trap-27 | Trap | Samples / Keys | TRAP Cloud Dust Chamber | 140 | Gentle | 16.39% | Warm 1.7-beat tail, 140 BPM; guitar |
| 85 | trap-28 | Trap | Samples / Keys | TRAP Rage Guitar Plate | 140 | Medium | 38.26% | Crisp 2.4-beat tail, 140 BPM; guitar |
| 86 | trap-29 | Trap | Samples / Keys | TRAP Memphis Keys Horizon | 140 | Heavy | 80.47% | Deep 4.8-beat tail, 140 BPM; keys2 |
| 87 | trap-30 | Trap | Samples / Keys | TRAP Luxury Melody Bloom | 140 | Heavy | 69.26% | Bloom 5.5-beat tail, 140 BPM; keys |
| 88 | trap-31 | Trap | Samples / Keys | TRAP ATL Reverse Chops | 140 | Heavy | 77.71% | Warm 2.5-beat tail, 140 BPM; guitar; reverse wet fragments |
| 89 | trap-32 | Trap | Samples / Keys | TRAP Dark Piano Octave | 140 | Medium | 41.38% | Velvet 4.2-beat tail, 140 BPM; keys2; shimmer +12 st |
| 90 | trap-33 | Trap | Synth | TRAP Cloud Pluck Space | 140 | Gentle | 17.64% | Clean 1.2-beat tail, 140 BPM; synth |
| 91 | trap-34 | Trap | Synth | TRAP Rage Lead Plate | 140 | Medium | 36.48% | Crisp 2.1-beat tail, 140 BPM; synth |
| 92 | trap-35 | Trap | Synth | TRAP Memphis Warm Arp | 140 | Medium | 38.34% | Warm 2.7-beat tail, 140 BPM; synth |
| 93 | trap-36 | Trap | Synth | TRAP Luxury Pad Velvet | 140 | Heavy | 66.04% | Velvet 5.2-beat tail, 140 BPM; synth |
| 94 | trap-37 | Trap | Synth | TRAP ATL Deep Chord | 140 | Heavy | 72.46% | Deep 6.5-beat tail, 140 BPM; synth |
| 95 | trap-38 | Trap | Synth | TRAP Dark Bloom Fifth | 140 | Heavy | 69.47% | Bloom 6-beat tail, 140 BPM; synth; shimmer +7 st |
| 96 | trap-39 | Trap | Synth | TRAP Cloud Pulse Gate | 140 | Medium | 34.14% | Crisp 2-beat tail, 140 BPM; synth; gated |
| 97 | trap-40 | Trap | Synth | TRAP Rage Reverse Sequence | 140 | Heavy | 80.47% | Deep 3.3-beat tail, 140 BPM; synth; reverse wet fragments |
| 98 | trap-41 | Trap | Atmosphere / FX | TRAP Memphis Endless Sky | 140 | Heavy | 72.27% | Bloom 12-beat tail, 140 BPM; keys2; shimmer +12 st |
| 99 | trap-42 | Trap | Atmosphere / FX | TRAP Luxury Low Orbit | 140 | Heavy | 70.48% | Deep 8-beat tail, 140 BPM; synth; shimmer -12 st |
| 100 | trap-43 | Trap | Atmosphere / FX | TRAP ATL Silver Cloud | 140 | Heavy | 64.32% | Velvet 9-beat tail, 140 BPM; vocal back2; shimmer +24 st |
| 101 | trap-44 | Trap | Atmosphere / FX | TRAP Dark Warm Fog | 140 | Heavy | 70.10% | Warm 7-beat tail, 140 BPM; guitar |
| 102 | trap-45 | Trap | Atmosphere / FX | TRAP Cloud Distant Station | 140 | Heavy | 68.45% | Deep 10-beat tail, 140 BPM; keys |
| 103 | trap-46 | Trap | Atmosphere / FX | TRAP Rage Velvet Lift | 140 | Heavy | 88.98% | Velvet 6-beat tail, 140 BPM; vocal back3; rising swell |
| 104 | trap-47 | Trap | Atmosphere / FX | TRAP Memphis Reverse Horizon | 140 | Heavy | 79.06% | Bloom 8.5-beat tail, 140 BPM; keys2; reverse wet fragments |
| 105 | trap-48 | Trap | Atmosphere / FX | TRAP Luxury Fractured Room | 140 | Medium | 39.07% | Clean 3.8-beat tail, 140 BPM; perc; gated |
| 106 | trap-49 | Trap | Atmosphere / FX | TRAP ATL Octave Rain | 140 | Medium | 29.19% | Crisp 5-beat tail, 140 BPM; hihat; shimmer +12 st |
| 107 | trap-50 | Trap | Atmosphere / FX | TRAP Dark Afterglow | 140 | Heavy | 63.81% | Bloom 14-beat tail, 140 BPM; synth |
| 108 | techno-01 | Techno | Vocal | TECHNO Peak Close Booth | 132 | Gentle | 45.49% | Clean 0.8-beat tail, 132 BPM; vocal lead1 |
| 109 | techno-02 | Techno | Vocal | TECHNO Hypnotic Warm Lead | 132 | Medium | 61.85% | Warm 2.3-beat tail, 132 BPM; vocal lead2 |
| 110 | techno-03 | Techno | Vocal | TECHNO Minimal Velvet Phrase | 132 | Medium | 68.62% | Velvet 3.1-beat tail, 132 BPM; vocal lead3 |
| 111 | techno-04 | Techno | Vocal | TECHNO Detroit Bright Plate | 132 | Medium | 61.15% | Crisp 1.9-beat tail, 132 BPM; vocal lead1 |
| 112 | techno-05 | Techno | Vocal | TECHNO Dub Distant Reply | 132 | Heavy | 88.63% | Deep 4.2-beat tail, 132 BPM; vocal back1 |
| 113 | techno-06 | Techno | Vocal | TECHNO Acid Breath Halo | 132 | Medium | 65.52% | Bloom 5-beat tail, 132 BPM; vocal back2; shimmer +12 st |
| 114 | techno-07 | Techno | Vocal | TECHNO Hard Hook Chamber | 132 | Gentle | 43.19% | Clean 1.5-beat tail, 132 BPM; vocal lead2 |
| 115 | techno-08 | Techno | Vocal | TECHNO Peak Backing Silk | 132 | Medium | 63.59% | Velvet 3.5-beat tail, 132 BPM; vocal back3 |
| 116 | techno-09 | Techno | Vocal | TECHNO Hypnotic Cut Phrase | 132 | Medium | 65.29% | Crisp 2-beat tail, 132 BPM; vocal lead3; gated |
| 117 | techno-10 | Techno | Vocal | TECHNO Minimal Reverse Adlib | 132 | Heavy | 91.45% | Deep 2.7-beat tail, 132 BPM; vocal back1; reverse wet fragments |
| 118 | techno-11 | Techno | Drums | TECHNO Detroit Snare Snap | 132 | Medium | 43.94% | Crisp 0.7-beat tail, 132 BPM; snare |
| 119 | techno-12 | Techno | Drums | TECHNO Dub Snare Steel | 132 | Medium | 40.93% | Crisp 1.3-beat tail, 132 BPM; snare |
| 120 | techno-13 | Techno | Drums | TECHNO Acid Snare Velvet | 132 | Medium | 46.67% | Velvet 1.7-beat tail, 132 BPM; snare |
| 121 | techno-14 | Techno | Drums | TECHNO Hard Gated Backbeat | 132 | Heavy | 69.06% | Crisp 2.1-beat tail, 132 BPM; snare; gated |
| 122 | techno-15 | Techno | Drums | TECHNO Peak Snare Uplift | 132 | Heavy | 88.69% | Crisp 1.8-beat tail, 132 BPM; snare; rising swell |
| 123 | techno-16 | Techno | Drums | TECHNO Hypnotic Kick Room | 132 | Gentle | 74.38% | Clean 0.45-beat tail, 132 BPM; kick |
| 124 | techno-17 | Techno | Drums | TECHNO Minimal Hat Air | 132 | Gentle | 23.64% | Clean 0.65-beat tail, 132 BPM; hihat |
| 125 | techno-18 | Techno | Drums | TECHNO Detroit Metal Wash | 132 | Medium | 47.39% | Velvet 2.4-beat tail, 132 BPM; hihat |
| 126 | techno-19 | Techno | Drums | TECHNO Dub Percussion Yard | 132 | Medium | 51.16% | Warm 1.1-beat tail, 132 BPM; perc |
| 127 | techno-20 | Techno | Drums | TECHNO Acid Break Room | 132 | Gentle | 63.05% | Clean 0.85-beat tail, 132 BPM; drumbus1 |
| 128 | techno-21 | Techno | Drums | TECHNO Hard Drum Warehouse | 132 | Medium | 86.63% | Deep 2.8-beat tail, 132 BPM; drumbus1 |
| 129 | techno-22 | Techno | Drums | TECHNO Peak Reverse Break | 132 | Heavy | 93.44% | Crisp 1.5-beat tail, 132 BPM; drumbus1; reverse wet fragments |
| 130 | techno-23 | Techno | Samples / Keys | TECHNO Hypnotic Piano Wood | 132 | Medium | 40.39% | Warm 2.3-beat tail, 132 BPM; keys |
| 131 | techno-24 | Techno | Samples / Keys | TECHNO Minimal Piano Glass | 132 | Medium | 39.23% | Clean 2.8-beat tail, 132 BPM; keys2 |
| 132 | techno-25 | Techno | Samples / Keys | TECHNO Detroit Rhodes Silk | 132 | Medium | 32.98% | Velvet 3.4-beat tail, 132 BPM; keys |
| 133 | techno-26 | Techno | Samples / Keys | TECHNO Dub Chopped Room | 132 | Gentle | 23.61% | Clean 0.9-beat tail, 132 BPM; keys2 |
| 134 | techno-27 | Techno | Samples / Keys | TECHNO Acid Dust Chamber | 132 | Gentle | 16.05% | Warm 1.7-beat tail, 132 BPM; guitar |
| 135 | techno-28 | Techno | Samples / Keys | TECHNO Hard Guitar Plate | 132 | Medium | 37.56% | Crisp 2.4-beat tail, 132 BPM; guitar |
| 136 | techno-29 | Techno | Samples / Keys | TECHNO Peak Keys Horizon | 132 | Heavy | 77.88% | Deep 4.8-beat tail, 132 BPM; keys2 |
| 137 | techno-30 | Techno | Samples / Keys | TECHNO Hypnotic Melody Bloom | 132 | Heavy | 68.62% | Bloom 5.5-beat tail, 132 BPM; keys |
| 138 | techno-31 | Techno | Samples / Keys | TECHNO Minimal Reverse Chops | 132 | Heavy | 77.43% | Warm 2.5-beat tail, 132 BPM; guitar; reverse wet fragments |
| 139 | techno-32 | Techno | Samples / Keys | TECHNO Detroit Piano Octave | 132 | Medium | 38.55% | Velvet 4.2-beat tail, 132 BPM; keys2; shimmer +12 st |
| 140 | techno-33 | Techno | Synth | TECHNO Dub Pluck Space | 132 | Gentle | 16.87% | Clean 1.2-beat tail, 132 BPM; synth |
| 141 | techno-34 | Techno | Synth | TECHNO Acid Lead Plate | 132 | Medium | 35.40% | Crisp 2.1-beat tail, 132 BPM; synth |
| 142 | techno-35 | Techno | Synth | TECHNO Hard Warm Arp | 132 | Medium | 37.21% | Warm 2.7-beat tail, 132 BPM; synth |
| 143 | techno-36 | Techno | Synth | TECHNO Peak Pad Velvet | 132 | Heavy | 64.88% | Velvet 5.2-beat tail, 132 BPM; synth |
| 144 | techno-37 | Techno | Synth | TECHNO Hypnotic Deep Chord | 132 | Heavy | 71.18% | Deep 6.5-beat tail, 132 BPM; synth |
| 145 | techno-38 | Techno | Synth | TECHNO Minimal Bloom Fifth | 132 | Heavy | 68.24% | Bloom 6-beat tail, 132 BPM; synth; shimmer +7 st |
| 146 | techno-39 | Techno | Synth | TECHNO Detroit Pulse Gate | 132 | Medium | 33.30% | Crisp 2-beat tail, 132 BPM; synth; gated |
| 147 | techno-40 | Techno | Synth | TECHNO Dub Reverse Sequence | 132 | Heavy | 79.08% | Deep 3.3-beat tail, 132 BPM; synth; reverse wet fragments |
| 148 | techno-41 | Techno | Atmosphere / FX | TECHNO Acid Endless Sky | 132 | Heavy | 68.70% | Bloom 12-beat tail, 132 BPM; keys2; shimmer +12 st |
| 149 | techno-42 | Techno | Atmosphere / FX | TECHNO Hard Low Orbit | 132 | Heavy | 68.97% | Deep 8-beat tail, 132 BPM; synth; shimmer -12 st |
| 150 | techno-43 | Techno | Atmosphere / FX | TECHNO Peak Silver Cloud | 132 | Heavy | 61.88% | Velvet 9-beat tail, 132 BPM; vocal back2; shimmer +24 st |
| 151 | techno-44 | Techno | Atmosphere / FX | TECHNO Hypnotic Warm Fog | 132 | Heavy | 68.61% | Warm 7-beat tail, 132 BPM; guitar |
| 152 | techno-45 | Techno | Atmosphere / FX | TECHNO Minimal Distant Station | 132 | Heavy | 66.70% | Deep 10-beat tail, 132 BPM; keys |
| 153 | techno-46 | Techno | Atmosphere / FX | TECHNO Detroit Velvet Lift | 132 | Heavy | 87.99% | Velvet 6-beat tail, 132 BPM; vocal back3; rising swell |
| 154 | techno-47 | Techno | Atmosphere / FX | TECHNO Dub Reverse Horizon | 132 | Heavy | 76.61% | Bloom 8.5-beat tail, 132 BPM; keys2; reverse wet fragments |
| 155 | techno-48 | Techno | Atmosphere / FX | TECHNO Acid Fractured Room | 132 | Medium | 39.59% | Clean 3.8-beat tail, 132 BPM; perc; gated |
| 156 | techno-49 | Techno | Atmosphere / FX | TECHNO Hard Octave Rain | 132 | Medium | 29.70% | Crisp 5-beat tail, 132 BPM; hihat; shimmer +12 st |
| 157 | techno-50 | Techno | Atmosphere / FX | TECHNO Peak Afterglow | 132 | Heavy | 61.94% | Bloom 14-beat tail, 132 BPM; synth |
| 158 | dnb-01 | DnB | Vocal | DNB Modern Close Booth | 174 | Gentle | 47.25% | Clean 0.8-beat tail, 174 BPM; vocal lead1 |
| 159 | dnb-02 | DnB | Vocal | DNB Roller Warm Lead | 174 | Medium | 64.12% | Warm 2.3-beat tail, 174 BPM; vocal lead2 |
| 160 | dnb-03 | DnB | Vocal | DNB Liquid Velvet Phrase | 174 | Medium | 70.44% | Velvet 3.1-beat tail, 174 BPM; vocal lead3 |
| 161 | dnb-04 | DnB | Vocal | DNB Neuro Bright Plate | 174 | Medium | 63.15% | Crisp 1.9-beat tail, 174 BPM; vocal lead1 |
| 162 | dnb-05 | DnB | Vocal | DNB Jump-Up Distant Reply | 174 | Heavy | 90.88% | Deep 4.2-beat tail, 174 BPM; vocal back1 |
| 163 | dnb-06 | DnB | Vocal | DNB Break Breath Halo | 174 | Medium | 68.69% | Bloom 5-beat tail, 174 BPM; vocal back2; shimmer +12 st |
| 164 | dnb-07 | DnB | Vocal | DNB Modern Hook Chamber | 174 | Gentle | 44.99% | Clean 1.5-beat tail, 174 BPM; vocal lead2 |
| 165 | dnb-08 | DnB | Vocal | DNB Roller Backing Silk | 174 | Medium | 66.35% | Velvet 3.5-beat tail, 174 BPM; vocal back3 |
| 166 | dnb-09 | DnB | Vocal | DNB Liquid Cut Phrase | 174 | Medium | 68.86% | Crisp 2-beat tail, 174 BPM; vocal lead3; gated |
| 167 | dnb-10 | DnB | Vocal | DNB Neuro Reverse Adlib | 174 | Heavy | 93.16% | Deep 2.7-beat tail, 174 BPM; vocal back1; reverse wet fragments |
| 168 | dnb-11 | DnB | Drums | DNB Jump-Up Snare Snap | 174 | Medium | 45.20% | Crisp 0.7-beat tail, 174 BPM; snare |
| 169 | dnb-12 | DnB | Drums | DNB Break Snare Steel | 174 | Medium | 42.71% | Crisp 1.3-beat tail, 174 BPM; snare |
| 170 | dnb-13 | DnB | Drums | DNB Modern Snare Velvet | 174 | Medium | 48.34% | Velvet 1.7-beat tail, 174 BPM; snare |
| 171 | dnb-14 | DnB | Drums | DNB Roller Gated Backbeat | 174 | Heavy | 71.32% | Crisp 2.1-beat tail, 174 BPM; snare; gated |
| 172 | dnb-15 | DnB | Drums | DNB Liquid Snare Uplift | 174 | Heavy | 89.31% | Crisp 1.8-beat tail, 174 BPM; snare; rising swell |
| 173 | dnb-16 | DnB | Drums | DNB Neuro Kick Room | 174 | Gentle | 74.96% | Clean 0.45-beat tail, 174 BPM; kick |
| 174 | dnb-17 | DnB | Drums | DNB Jump-Up Hat Air | 174 | Gentle | 24.13% | Clean 0.65-beat tail, 174 BPM; hihat |
| 175 | dnb-18 | DnB | Drums | DNB Break Metal Wash | 174 | Medium | 48.17% | Velvet 2.4-beat tail, 174 BPM; hihat |
| 176 | dnb-19 | DnB | Drums | DNB Modern Percussion Yard | 174 | Medium | 51.56% | Warm 1.1-beat tail, 174 BPM; perc |
| 177 | dnb-20 | DnB | Drums | DNB Roller Break Room | 174 | Gentle | 64.89% | Clean 0.85-beat tail, 174 BPM; drumbus1 |
| 178 | dnb-21 | DnB | Drums | DNB Liquid Drum Warehouse | 174 | Medium | 89.96% | Deep 2.8-beat tail, 174 BPM; drumbus1 |
| 179 | dnb-22 | DnB | Drums | DNB Neuro Reverse Break | 174 | Heavy | 94.47% | Crisp 1.5-beat tail, 174 BPM; drumbus1; reverse wet fragments |
| 180 | dnb-23 | DnB | Samples / Keys | DNB Jump-Up Piano Wood | 174 | Medium | 42.45% | Warm 2.3-beat tail, 174 BPM; keys |
| 181 | dnb-24 | DnB | Samples / Keys | DNB Break Piano Glass | 174 | Medium | 43.10% | Clean 2.8-beat tail, 174 BPM; keys2 |
| 182 | dnb-25 | DnB | Samples / Keys | DNB Modern Rhodes Silk | 174 | Medium | 39.46% | Velvet 3.4-beat tail, 174 BPM; keys |
| 183 | dnb-26 | DnB | Samples / Keys | DNB Roller Chopped Room | 174 | Gentle | 28.15% | Clean 0.9-beat tail, 174 BPM; keys2 |
| 184 | dnb-27 | DnB | Samples / Keys | DNB Liquid Dust Chamber | 174 | Gentle | 16.69% | Warm 1.7-beat tail, 174 BPM; guitar |
| 185 | dnb-28 | DnB | Samples / Keys | DNB Neuro Guitar Plate | 174 | Medium | 39.20% | Crisp 2.4-beat tail, 174 BPM; guitar |
| 186 | dnb-29 | DnB | Samples / Keys | DNB Jump-Up Keys Horizon | 174 | Heavy | 83.14% | Deep 4.8-beat tail, 174 BPM; keys2 |
| 187 | dnb-30 | DnB | Samples / Keys | DNB Break Melody Bloom | 174 | Heavy | 71.87% | Bloom 5.5-beat tail, 174 BPM; keys |
| 188 | dnb-31 | DnB | Samples / Keys | DNB Modern Reverse Chops | 174 | Heavy | 82.59% | Warm 2.5-beat tail, 174 BPM; guitar; reverse wet fragments |
| 189 | dnb-32 | DnB | Samples / Keys | DNB Roller Piano Octave | 174 | Medium | 44.24% | Velvet 4.2-beat tail, 174 BPM; keys2; shimmer +12 st |
| 190 | dnb-33 | DnB | Synth | DNB Liquid Pluck Space | 174 | Gentle | 18.51% | Clean 1.2-beat tail, 174 BPM; synth |
| 191 | dnb-34 | DnB | Synth | DNB Neuro Lead Plate | 174 | Medium | 38.28% | Crisp 2.1-beat tail, 174 BPM; synth |
| 192 | dnb-35 | DnB | Synth | DNB Jump-Up Warm Arp | 174 | Medium | 40.44% | Warm 2.7-beat tail, 174 BPM; synth |
| 193 | dnb-36 | DnB | Synth | DNB Break Pad Velvet | 174 | Heavy | 68.16% | Velvet 5.2-beat tail, 174 BPM; synth |
| 194 | dnb-37 | DnB | Synth | DNB Modern Deep Chord | 174 | Heavy | 74.84% | Deep 6.5-beat tail, 174 BPM; synth |
| 195 | dnb-38 | DnB | Synth | DNB Roller Bloom Fifth | 174 | Heavy | 70.46% | Bloom 6-beat tail, 174 BPM; synth; shimmer +7 st |
| 196 | dnb-39 | DnB | Synth | DNB Liquid Pulse Gate | 174 | Medium | 36.00% | Crisp 2-beat tail, 174 BPM; synth; gated |
| 197 | dnb-40 | DnB | Synth | DNB Neuro Reverse Sequence | 174 | Heavy | 82.58% | Deep 3.3-beat tail, 174 BPM; synth; reverse wet fragments |
| 198 | dnb-41 | DnB | Atmosphere / FX | DNB Jump-Up Endless Sky | 174 | Heavy | 75.40% | Bloom 12-beat tail, 174 BPM; keys2; shimmer +12 st |
| 199 | dnb-42 | DnB | Atmosphere / FX | DNB Break Low Orbit | 174 | Heavy | 72.94% | Deep 8-beat tail, 174 BPM; synth; shimmer -12 st |
| 200 | dnb-43 | DnB | Atmosphere / FX | DNB Modern Silver Cloud | 174 | Heavy | 68.40% | Velvet 9-beat tail, 174 BPM; vocal back2; shimmer +24 st |
| 201 | dnb-44 | DnB | Atmosphere / FX | DNB Roller Warm Fog | 174 | Heavy | 71.76% | Warm 7-beat tail, 174 BPM; guitar |
| 202 | dnb-45 | DnB | Atmosphere / FX | DNB Liquid Distant Station | 174 | Heavy | 72.28% | Deep 10-beat tail, 174 BPM; keys |
| 203 | dnb-46 | DnB | Atmosphere / FX | DNB Neuro Velvet Lift | 174 | Heavy | 90.80% | Velvet 6-beat tail, 174 BPM; vocal back3; rising swell |
| 204 | dnb-47 | DnB | Atmosphere / FX | DNB Jump-Up Reverse Horizon | 174 | Heavy | 81.52% | Bloom 8.5-beat tail, 174 BPM; keys2; reverse wet fragments |
| 205 | dnb-48 | DnB | Atmosphere / FX | DNB Break Fractured Room | 174 | Medium | 39.21% | Clean 3.8-beat tail, 174 BPM; perc; gated |
| 206 | dnb-49 | DnB | Atmosphere / FX | DNB Modern Octave Rain | 174 | Medium | 30.76% | Crisp 5-beat tail, 174 BPM; hihat; shimmer +12 st |
| 207 | dnb-50 | DnB | Atmosphere / FX | DNB Roller Afterglow | 174 | Heavy | 66.55% | Bloom 14-beat tail, 174 BPM; synth |
