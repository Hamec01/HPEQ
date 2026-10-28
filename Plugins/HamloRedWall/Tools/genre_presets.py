"""HamloRedWall genre presets (docs/GENRE_PRESETS.md): the recipes, written out to Source/Plugin/GenrePresets.inc.

python Tools/genre_presets.py [calibration.tsv]
calibration.tsv (from `HamloRedWall_PresetLab calibrate`): "CAL<TAB>genre/group/name<TAB>loudness dB<TAB>GR" lines;
without it LOUDNESS is the depth's first guess. Every other value is designed here.
"""
import sys
from pathlib import Path

# HPDG's genre engines (Source/Engine/StyleDefaults.cpp): default tempo and the substyles the names come from
GENRES = {
    "Boom Bap": 90.0,  # Classic, Dusty, Jazzy, BoomBapGold, RussianUnderground, LofiRap
    "Trap": 140.0,     # ATLClassic, DarkTrap, CloudTrap, RageTrap, MemphisTrap, LuxuryTrap
    "Techno": 132.0,   # Peak Time, Hypnotic, Minimal, Detroit, Dub, Acid, Hard
    "DnB": 174.0,      # Modern, Roller, Liquid, Neurofunk, Jump-Up, Breakbeat
}
GROUPS = {  # group -> the material it is calibrated on (HamloRedWall_PresetLab)
    "Kick": "kick", "Snare / Clap": "snare", "Hats / Cymbals": "hats", "Percussion": "perc", "Drum Bus": "drums",
    "808 / Bass": "bass", "Vocals": "vocal", "Keys / Piano": "keys", "Samples / Melody": "samples", "Master": "mix",
}
STYLE = {"G": 0, "S": 1, "O": 2}
STYLE_NAME = {"G": "GLASNOST", "S": "STAKHANOV", "O": "OCTOBER"}
DEPTH = {"g": "Gentle", "m": "Medium", "h": "Heavy"}
GUESS = {"g": -3.0, "m": -6.0, "h": -10.0}
BEATS = {"r16": 0.25, "r8": 0.5, "r4": 1.0}
# single hits (kick, snare, hats, percussion): the limiter must let go inside the hit, or it only turns the whole hit
# down - "hit" is this release per genre (the faster the tempo, the shorter); the second calibration run showed it
# look-ahead per genre: the faster the tempo, the shorter, so the attack of the hit stays (boom bap rounder)
LOOK_SCALE = {"Boom Bap": 1.2, "Trap": 1.0, "Techno": 0.85, "DnB": 0.7}
HIT_RELEASE = {"Boom Bap": 30, "Trap": 22, "Techno": 20, "DnB": 15}
NOTE_NAME = {"r16": "1/16", "r8": "1/8", "r4": "1/4"}

# (group, name, depth, style, lookahead ms, release ms | r16 / r8 / r4 (tempo) | "auto", drive, tube, extras)
# Tracks: LIMIT -1 dB, TRUE PEAK off; Master: TRUE PEAK on. Drums: short look-ahead keeps the attack; bass and
# full mixes: long look-ahead and long / auto release so the low end is not distorted (docs/REDWALL_V2.md R3).
K, S_, H, P, B, BA, V, KE, SA, M = GROUPS.keys()
RECIPES = {
    "Boom Bap": [
        (K, "Classic Knock Catch", "g", "G", 1.0, "r16", 0.0, 0.2, {}),
        (K, "Dusty Tube Thump", "m", "G", 2.0, "r16", 0.0, 0.6, {}),
        (K, "Gold Clip Punch", "m", "O", 0.5, 40, 0.1, 0.2, {}),
        (K, "Underground Drive Knock", "m", "S", 0.5, 30, 0.45, 0.2, {}),
        (K, "Lofi Crunch", "m", "O", 0.3, "r16", 0.6, 0.5, {}),
        (K, "Jazzy Soft Ceiling", "g", "G", 3.0, "auto", 0.0, 0.1, {}),
        (S_, "Classic Crack Catch", "g", "G", 1.0, "r16", 0.0, 0.1, {}),
        (S_, "Dusty Fat Snare", "m", "G", 2.0, "r16", 0.1, 0.7, {}),
        (S_, "Gold Snare Clip", "m", "O", 0.5, 30, 0.1, 0.2, {}),
        (S_, "Underground Snare Grit", "m", "S", 0.5, 30, 0.5, 0.3, {}),
        (S_, "Lofi Snare Crush", "h", "O", 0.3, "r16", 0.7, 0.4, {}),
        (S_, "Jazzy Brush Tame", "g", "G", 3.0, "auto", 0.0, 0.2, {}),
        (H, "Classic Hat Peak Tame", "g", "G", 1.0, 30, 0.0, 0.0, {"link": 0.5}),
        (H, "Dusty Hat Warm", "g", "G", 2.0, 40, 0.0, 0.5, {}),
        (H, "Gold Hat Sizzle", "m", "S", 0.5, 20, 0.3, 0.0, {}),
        (H, "Underground Hat Dirt", "m", "O", 0.3, 20, 0.5, 0.2, {}),
        (H, "Lofi Hat Crunch", "h", "O", 0.3, 30, 0.7, 0.5, {}),
        (P, "Classic Perc Catch", "g", "G", 1.0, 30, 0.0, 0.1, {}),
        (P, "Dusty Perc Warm", "m", "G", 2.0, 50, 0.0, 0.6, {}),
        (P, "Gold Perc Pop", "m", "O", 0.5, 30, 0.2, 0.1, {}),
        (P, "Underground Perc Grit", "m", "S", 0.5, 30, 0.4, 0.2, {}),
        (B, "Classic Bus Ceiling", "g", "G", 2.0, "auto", 0.0, 0.2, {}),
        (B, "Dusty Tape Bus", "m", "G", 3.0, "auto", 0.1, 0.7, {}),
        (B, "Jazzy Bus Air", "g", "G", 5.0, "auto", 0.0, 0.15, {}),
        (B, "Gold Bus Slam", "m", "S", 1.0, "r16", 0.2, 0.3, {}),
        (B, "Underground Bus Grit", "m", "S", 1.0, "r16", 0.45, 0.4, {}),
        (B, "Lofi SP Bus Crunch", "h", "O", 0.5, "r8", 0.6, 0.6, {}),
        (B, "Knock Parallel Smash", "h", "O", 0.5, "r16", 0.5, 0.4, {"mix": 0.5}),
        (BA, "Classic Bass Ceiling", "g", "G", 5.0, "auto", 0.0, 0.2, {}),
        (BA, "Dusty Bass Tube", "m", "G", 5.0, "auto", 0.0, 0.6, {}),
        (BA, "Jazzy Upright Warm", "g", "G", 8.0, "auto", 0.0, 0.4, {}),
        (BA, "Gold Bass Drive", "m", "S", 3.0, 120, 0.3, 0.3, {}),
        (BA, "Underground Sub Weight", "m", "G", 8.0, "r8", 0.0, 0.5, {}),
        (V, "Classic Vocal Peak Catch", "g", "G", 2.0, "auto", 0.0, 0.1, {}),
        (V, "Dusty Vocal Tube", "m", "G", 2.0, "auto", 0.0, 0.5, {}),
        (V, "Gold Vocal Presence", "m", "S", 1.0, "auto", 0.25, 0.2, {}),
        (V, "Underground Raw Vocal", "m", "S", 1.0, 60, 0.35, 0.2, {}),
        (V, "Lofi Vocal Crunch", "h", "O", 0.5, 80, 0.6, 0.5, {}),
        (KE, "Classic Rhodes Ceiling", "g", "G", 3.0, "auto", 0.0, 0.3, {}),
        (KE, "Dusty Piano Warmth", "m", "G", 3.0, "auto", 0.0, 0.6, {}),
        (KE, "Jazzy Keys Air", "g", "G", 5.0, "auto", 0.0, 0.2, {}),
        (KE, "Gold Keys Edge", "m", "S", 2.0, "auto", 0.25, 0.2, {}),
        (SA, "Classic Soul Sample Glue", "m", "G", 3.0, "auto", 0.0, 0.4, {}),
        (SA, "Dusty Vinyl Sample", "m", "G", 3.0, "auto", 0.15, 0.7, {}),
        (SA, "Lofi Sample Crush", "h", "O", 1.0, "r8", 0.55, 0.6, {}),
        (SA, "Jazzy Loop Ceiling", "g", "G", 5.0, "auto", 0.0, 0.2, {}),
        (M, "Classic Boom Bap Master", "m", "G", 2.0, "auto", 0.0, 0.25, {}),
        (M, "Dusty Warm Master", "m", "G", 3.0, "auto", 0.05, 0.5, {}),
        (M, "Gold Loud Master", "h", "S", 1.5, "auto", 0.1, 0.3, {}),
        (M, "Lofi Tape Master", "m", "O", 2.0, "auto", 0.2, 0.6, {}),
    ],
    "Trap": [
        (K, "ATL Kick Clip", "m", "O", 0.5, 30, 0.1, 0.1, {}),
        (K, "Dark Kick Weight", "m", "G", 1.0, "r16", 0.0, 0.5, {}),
        (K, "Cloud Kick Soft Catch", "g", "G", 2.0, "auto", 0.0, 0.2, {}),
        (K, "Rage Kick Slam", "h", "O", 0.3, 20, 0.5, 0.2, {}),
        (K, "Memphis Kick Dirt", "m", "S", 0.5, 30, 0.5, 0.3, {}),
        (K, "Luxury Kick Clean Peak", "g", "G", 1.0, "r16", 0.0, 0.1, {}),
        (S_, "ATL Clap Clip", "m", "O", 0.5, 30, 0.1, 0.1, {}),
        (S_, "Dark Snare Weight", "m", "G", 1.0, "r16", 0.0, 0.5, {}),
        (S_, "Cloud Clap Smooth", "g", "G", 2.0, "auto", 0.0, 0.3, {}),
        (S_, "Rage Snare Smash", "h", "O", 0.3, 20, 0.5, 0.2, {}),
        (S_, "Memphis Snare Grit", "m", "S", 0.5, 30, 0.55, 0.3, {}),
        (S_, "Luxury Clap Clean", "g", "G", 1.0, 30, 0.0, 0.1, {}),
        (H, "ATL Hat Roll Even", "g", "S", 0.3, 15, 0.0, 0.0, {"link": 0.5}),
        (H, "Dark Hat Tame", "g", "G", 1.0, 30, 0.0, 0.3, {}),
        (H, "Rage Hat Sizzle", "m", "S", 0.3, 15, 0.4, 0.0, {}),
        (H, "Memphis Hat Dirt", "m", "O", 0.3, 20, 0.45, 0.1, {}),
        (H, "Luxury Hat Shine", "g", "G", 0.5, 20, 0.15, 0.0, {}),
        (P, "ATL Perc Pop", "m", "O", 0.5, 30, 0.2, 0.1, {}),
        (P, "Dark Perc Weight", "g", "G", 1.0, 40, 0.0, 0.12, {}),
        (P, "Rage Perc Crunch", "h", "O", 0.3, 20, 0.6, 0.2, {}),
        (P, "Luxury Perc Clean", "g", "G", 1.0, 30, 0.0, 0.1, {}),
        (B, "ATL Bus Ceiling", "m", "G", 2.0, "auto", 0.0, 0.2, {}),
        (B, "Dark Bus Weight", "m", "G", 2.0, "auto", 0.0, 0.6, {}),
        (B, "Cloud Bus Air", "g", "G", 4.0, "auto", 0.0, 0.2, {}),
        (B, "Rage Bus Slam", "h", "O", 0.5, "r16", 0.4, 0.3, {}),
        (B, "Memphis Bus Grit", "m", "S", 1.0, "r16", 0.5, 0.4, {}),
        (B, "Luxury Bus Punch", "m", "S", 1.0, "r16", 0.15, 0.2, {}),
        (B, "Dark Parallel Crush", "h", "O", 0.3, "r16", 0.6, 0.4, {"mix": 0.5}),
        (BA, "ATL 808 Clip", "m", "O", 3.0, "r8", 0.1, 0.2, {}),
        (BA, "Dark 808 Weight", "m", "G", 5.0, "auto", 0.0, 0.6, {}),
        (BA, "Rage 808 Distort", "m", "O", 2.0, "r8", 0.6, 0.3, {}),
        (BA, "Memphis 808 Grit", "m", "S", 3.0, "r8", 0.4, 0.4, {}),
        (BA, "Luxury Sub Ceiling", "g", "G", 8.0, "auto", 0.0, 0.2, {}),
        (V, "ATL Vocal Ceiling", "m", "G", 2.0, "auto", 0.0, 0.1, {}),
        (V, "Dark Vocal Tube", "m", "G", 2.0, "auto", 0.0, 0.5, {}),
        (V, "Cloud Vocal Soft", "g", "G", 3.0, "auto", 0.0, 0.3, {}),
        (V, "Rage Vocal Distort", "h", "O", 0.5, 60, 0.6, 0.2, {}),
        (V, "Luxury Vocal Presence", "m", "S", 1.0, "auto", 0.2, 0.15, {}),
        (KE, "ATL Piano Ceiling", "g", "G", 3.0, "auto", 0.0, 0.2, {}),
        (KE, "Dark Keys Weight", "m", "G", 3.0, "auto", 0.0, 0.6, {}),
        (KE, "Cloud Pad Air", "g", "G", 5.0, "auto", 0.0, 0.3, {}),
        (KE, "Luxury Bell Shine", "g", "S", 1.0, "auto", 0.05, 0.0, {}),
        (SA, "ATL Loop Glue", "m", "G", 3.0, "auto", 0.0, 0.3, {}),
        (SA, "Dark Melody Weight", "m", "G", 3.0, "auto", 0.05, 0.6, {}),
        (SA, "Cloud Guitar Soft", "g", "G", 4.0, "auto", 0.0, 0.3, {}),
        (SA, "Rage Synth Grit", "h", "O", 1.0, "r16", 0.55, 0.2, {}),
        (M, "ATL Trap Master", "h", "S", 1.5, "auto", 0.05, 0.2, {}),
        (M, "Dark Master Weight", "m", "G", 2.0, "auto", 0.0, 0.5, {}),
        (M, "Rage Loud Master", "h", "O", 1.0, "auto", 0.2, 0.3, {}),
        (M, "Luxury Clean Master", "m", "G", 3.0, "auto", 0.0, 0.15, {}),
    ],
    "Techno": [
        (K, "Peak Kick Clip", "m", "O", 0.5, 30, 0.15, 0.1, {}),
        (K, "Hypnotic Kick Catch", "g", "G", 1.0, "r16", 0.0, 0.2, {}),
        (K, "Minimal Kick Clean", "g", "G", 1.0, "r16", 0.0, 0.0, {}),
        (K, "Detroit Kick Warm", "m", "G", 1.0, "r16", 0.0, 0.6, {}),
        (K, "Hard Kick Distort", "m", "O", 0.3, 20, 0.7, 0.3, {}),
        (K, "Acid Kick Drive", "m", "S", 0.5, 30, 0.45, 0.2, {}),
        (S_, "Peak Clap Clip", "m", "O", 0.5, 30, 0.15, 0.1, {}),
        (S_, "Minimal Clap Catch", "g", "G", 1.0, 30, 0.0, 0.0, {}),
        (S_, "Detroit Clap Warm", "m", "G", 1.0, "r16", 0.0, 0.5, {}),
        (S_, "Acid Clap Bite", "m", "S", 0.5, 30, 0.45, 0.1, {}),
        (S_, "Hard Clap Smash", "h", "O", 0.3, 20, 0.6, 0.2, {}),
        (S_, "Dub Snare Soft", "g", "G", 2.0, "auto", 0.0, 0.4, {}),
        (H, "Peak Hat Drive", "m", "S", 0.3, 15, 0.35, 0.0, {}),
        (H, "Hypnotic Hat Even", "g", "G", 0.5, 20, 0.0, 0.1, {}),
        (H, "Minimal Hat Tick", "g", "G", 0.5, 15, 0.0, 0.0, {}),
        (H, "Acid Hat Grit", "m", "O", 0.3, 15, 0.4, 0.05, {}),
        (H, "Hard Ride Crunch", "h", "O", 0.3, 20, 0.5, 0.3, {}),
        (P, "Peak Perc Pop", "m", "O", 0.5, 30, 0.2, 0.1, {}),
        (P, "Hypnotic Perc Catch", "g", "G", 1.0, 30, 0.0, 0.1, {}),
        (P, "Minimal Click Clean", "g", "G", 0.5, 20, 0.0, 0.0, {}),
        (P, "Detroit Conga Warm", "m", "G", 1.0, 40, 0.0, 0.5, {}),
        (B, "Peak Bus Slam", "m", "S", 1.0, "r16", 0.2, 0.2, {}),
        (B, "Hypnotic Bus Pump", "h", "S", 1.0, "r8", 0.1, 0.2, {}),
        (B, "Minimal Bus Ceiling", "g", "G", 2.0, "auto", 0.0, 0.1, {}),
        (B, "Detroit Bus Warm", "m", "G", 2.0, "auto", 0.0, 0.6, {}),
        (B, "Hard Bus Distort", "h", "O", 0.5, "r16", 0.6, 0.4, {}),
        (B, "Dub Bus Air", "g", "G", 4.0, "auto", 0.0, 0.15, {}),
        (B, "Hard Parallel Crush", "h", "O", 0.3, "r16", 0.7, 0.5, {"mix": 0.4}),
        (BA, "Peak Bass Ceiling", "m", "G", 5.0, "auto", 0.0, 0.2, {}),
        (BA, "Hypnotic Rumble Weight", "m", "G", 8.0, "r4", 0.0, 0.6, {}),
        (BA, "Acid 303 Drive", "m", "S", 2.0, "r16", 0.5, 0.1, {}),
        (BA, "Detroit Bass Warm", "g", "G", 5.0, "auto", 0.0, 0.5, {}),
        (BA, "Hard Rumble Distort", "m", "O", 3.0, "r8", 0.6, 0.4, {}),
        (V, "Peak Vocal Chop Catch", "m", "G", 1.0, "auto", 0.0, 0.1, {}),
        (V, "Hypnotic Whisper Ceiling", "g", "G", 2.0, "auto", 0.0, 0.3, {}),
        (V, "Detroit Soul Vocal Warm", "m", "G", 2.0, "auto", 0.0, 0.5, {}),
        (V, "Dub Vocal Soft", "g", "G", 3.0, "auto", 0.0, 0.3, {}),
        (V, "Hard Shout Distort", "h", "O", 0.5, 50, 0.6, 0.2, {}),
        (KE, "Detroit Chord Warm", "m", "G", 2.0, "auto", 0.0, 0.5, {}),
        (KE, "Dub Chord Ceiling", "g", "G", 3.0, "auto", 0.0, 0.3, {}),
        (KE, "Peak Stab Edge", "m", "S", 1.0, "r16", 0.3, 0.1, {}),
        (KE, "Minimal Keys Clean", "g", "G", 3.0, "auto", 0.0, 0.0, {}),
        (SA, "Acid Lead Drive", "m", "S", 1.0, "r16", 0.5, 0.1, {}),
        (SA, "Hypnotic Loop Ceiling", "g", "G", 2.0, "auto", 0.0, 0.2, {}),
        (SA, "Peak Synth Glue", "m", "G", 2.0, "auto", 0.1, 0.3, {}),
        (SA, "Hard Noise Crunch", "h", "O", 0.5, "r16", 0.7, 0.3, {}),
        (M, "Peak Club Master", "h", "S", 1.5, "auto", 0.1, 0.2, {}),
        (M, "Hypnotic Master", "m", "G", 2.0, "auto", 0.0, 0.3, {}),
        (M, "Detroit Warm Master", "m", "G", 3.0, "auto", 0.0, 0.5, {}),
        (M, "Hard Loud Master", "h", "O", 1.0, "auto", 0.25, 0.3, {}),
    ],
    "DnB": [
        (K, "Modern Kick Clip", "m", "O", 0.5, 20, 0.1, 0.1, {}),
        (K, "Roller Kick Catch", "g", "G", 0.5, "r16", 0.0, 0.2, {}),
        (K, "Liquid Kick Soft", "g", "G", 1.0, "r16", 0.0, 0.3, {}),
        (K, "Neuro Kick Distort", "m", "O", 0.3, 15, 0.6, 0.2, {}),
        (K, "Jump-Up Kick Knock", "m", "S", 0.5, 20, 0.35, 0.2, {}),
        (K, "Break Kick Weight", "m", "G", 1.0, "r16", 0.0, 0.5, {}),
        (S_, "Modern Snare Clip", "m", "O", 0.5, 20, 0.1, 0.1, {}),
        (S_, "Roller Snare Catch", "g", "G", 0.5, "r16", 0.0, 0.1, {}),
        (S_, "Liquid Snare Smooth", "g", "G", 1.0, 30, 0.0, 0.3, {}),
        (S_, "Neuro Snare Smash", "h", "O", 0.3, 15, 0.6, 0.2, {}),
        (S_, "Jump-Up Snare Smack", "m", "S", 0.5, 20, 0.4, 0.2, {}),
        (S_, "Break Snare Fat", "m", "G", 1.0, "r16", 0.1, 0.6, {}),
        (H, "Modern Shuffle Even", "g", "S", 0.3, 10, 0.0, 0.0, {}),
        (H, "Roller Hat Drive", "m", "S", 0.3, 15, 0.3, 0.0, {}),
        (H, "Liquid Ride Soft", "g", "G", 1.0, 30, 0.0, 0.2, {}),
        (H, "Neuro Hat Grit", "m", "O", 0.3, 10, 0.35, 0.0, {}),
        (H, "Break Cymbal Tame", "g", "G", 1.0, 30, 0.0, 0.3, {}),
        (P, "Modern Perc Pop", "m", "O", 0.5, 20, 0.2, 0.1, {}),
        (P, "Liquid Shaker Soft", "g", "G", 1.0, 30, 0.0, 0.08, {}),
        (P, "Neuro Perc Crunch", "h", "O", 0.3, 15, 0.6, 0.2, {}),
        (P, "Break Perc Glue", "g", "G", 1.0, "r16", 0.0, 0.15, {}),
        (B, "Modern Bus Slam", "m", "S", 1.0, "r16", 0.15, 0.2, {}),
        (B, "Roller Bus Catch", "g", "G", 1.0, "r16", 0.0, 0.2, {}),
        (B, "Liquid Bus Air", "g", "G", 3.0, "auto", 0.0, 0.12, {}),
        (B, "Neuro Bus Distort", "h", "O", 0.5, "r16", 0.5, 0.3, {}),
        (B, "Amen Break Crunch", "h", "O", 0.5, "r16", 0.4, 0.5, {}),
        (B, "Break Tape Weight", "m", "G", 2.0, "auto", 0.1, 0.7, {}),
        (B, "Jump-Up Parallel Smash", "h", "O", 0.3, "r16", 0.6, 0.4, {"mix": 0.5}),
        (BA, "Modern Reese Ceiling", "m", "G", 3.0, "auto", 0.05, 0.2, {}),
        (BA, "Roller Sub Weight", "m", "G", 8.0, "r8", 0.0, 0.5, {}),
        (BA, "Liquid Bass Warm", "g", "G", 5.0, "auto", 0.0, 0.5, {}),
        (BA, "Neuro Bass Distort", "m", "O", 2.0, "r16", 0.6, 0.3, {}),
        (BA, "Jump-Up Wobble Grit", "m", "S", 2.0, "r16", 0.5, 0.3, {}),
        (V, "Modern MC Ceiling", "m", "S", 1.0, "auto", 0.1, 0.1, {}),
        (V, "Liquid Vocal Silk", "g", "G", 2.0, "auto", 0.0, 0.3, {}),
        (V, "Roller Vocal Chop", "m", "G", 1.0, "auto", 0.0, 0.2, {}),
        (V, "Jump-Up MC Distort", "h", "O", 0.5, 40, 0.5, 0.2, {}),
        (V, "Liquid Backing Air", "g", "G", 3.0, "auto", 0.0, 0.2, {"material": "backing"}),
        (KE, "Liquid Rhodes Warm", "g", "G", 3.0, "auto", 0.0, 0.5, {}),
        (KE, "Liquid Piano Ceiling", "m", "G", 3.0, "auto", 0.0, 0.2, {}),
        (KE, "Modern Stab Edge", "m", "S", 1.0, "r16", 0.3, 0.1, {}),
        (KE, "Roller Pad Air", "g", "G", 5.0, "auto", 0.0, 0.3, {}),
        (SA, "Liquid Strings Warm", "g", "G", 4.0, "auto", 0.0, 0.5, {}),
        (SA, "Neuro Synth Distort", "h", "O", 1.0, "r16", 0.6, 0.2, {}),
        (SA, "Break Loop Glue", "m", "G", 2.0, "auto", 0.0, 0.3, {}),
        (SA, "Jump-Up Hoover Grit", "m", "S", 1.0, "r16", 0.5, 0.3, {}),
        (M, "Modern DnB Master", "h", "S", 1.5, "auto", 0.1, 0.2, {}),
        (M, "Liquid Warm Master", "m", "G", 3.0, "auto", 0.0, 0.4, {}),
        (M, "Neuro Loud Master", "h", "O", 1.0, "auto", 0.25, 0.3, {}),
        (M, "Roller Master", "m", "S", 2.0, "auto", 0.05, 0.2, {}),
    ],
}


def fmt(v):
    return ("%.2f" % v).rstrip("0").rstrip(".")


def main():
    cal = {}
    if len(sys.argv) > 1:
        for line in Path(sys.argv[1]).read_text(encoding="utf-8").splitlines():
            parts = line.split("\t")
            if len(parts) >= 3 and parts[0] == "CAL":
                cal[parts[1]] = float(parts[2])
    out = ["// Generated by Tools/genre_presets.py - edit the recipes there. docs/GENRE_PRESETS.md",
           "// genre / group / name, mix, values, genre, group, note, material, depth"]
    names = set()
    for genre, bpm in GENRES.items():
        recipes = RECIPES[genre]
        assert len(recipes) == 50, (genre, len(recipes))
        for group, name, depth, style, look, rel, drive, tube, extra in recipes:
            key = f"{genre}/{group}/{name}"
            assert key not in names, key
            names.add(key)
            master = group == M
            values = {"thresholdDb": cal.get(key, GUESS[depth]), "ceilingDb": -1.0, "style": STYLE[style],
                      "lookaheadMs": round(max(0.1, look * LOOK_SCALE[genre]), 2), "truePeak": 1 if master else 0, "link": extra.get("link", 1.0),
                      "drive": drive, "tube": tube}
            if rel == "r16" and group in (K, S_, H, P):
                rel = HIT_RELEASE[genre]
            if rel == "auto" and group in (K, S_, H, P):
                rel = HIT_RELEASE[genre] * 2
            if isinstance(rel, (int, float)) and group in (K, S_, H, P):
                rel = min(rel, 2 * HIT_RELEASE[genre])
            if rel == "auto":
                values["autoRelease"] = 1
                values["releaseMs"] = 60.0
                release = "auto release"
            else:
                ms = 60000.0 / bpm * BEATS[rel] if isinstance(rel, str) else float(rel)
                values["autoRelease"] = 0
                values["releaseMs"] = round(ms)
                release = f"release {round(ms)} ms" + (f" ({NOTE_NAME[rel]} at {fmt(bpm)} BPM)" if isinstance(rel, str) else "")
            colour = []
            if drive > 0:
                colour.append(f"drive {round(drive * 100)} %")
            if tube > 0:
                colour.append(f"tube {round(tube * 100)} %")
            note = f"{STYLE_NAME[style]}, {release}" + (", " + ", ".join(colour) if colour else "")
            mix = extra.get("mix", 1.0)
            if mix < 1.0:
                note += f", parallel {round(mix * 100)} %"
            material = extra.get("material", GROUPS[group])
            vals = ", ".join(f'{{ "{k}", {fmt(v) if isinstance(v, float) else v} }}' for k, v in values.items())
            out.append(f'{{ "{name}", {float(mix)}f, {{ {vals} }}, "{genre}", "{group}", "{note}", "{material}", Preset::{DEPTH[depth]} }},')
    Path(__file__).resolve().parent.parent.joinpath("Source/Plugin/GenrePresets.inc").write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"{len(names)} genre presets, {len(cal)} calibrated")


if __name__ == "__main__":
    main()
