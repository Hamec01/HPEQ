"""HamloComp genre presets (docs/GENRE_PRESETS.md): the recipes, written out to Source/Plugin/GenrePresets.inc.

python Tools/genre_presets.py [calibration.tsv]
calibration.tsv (from `HamloComp_PresetLab calibrate-genre`): "CAL<TAB>genre/group/name<TAB>threshold dB<TAB>GR"
lines; without it THRESHOLD is the depth's first guess. Every other value is designed here.
"""
import sys
from pathlib import Path

# HPDG's genre engines (Source/Engine/StyleDefaults.cpp): default tempo; the names come from their substyles
GENRES = {"Boom Bap": 90.0, "Trap": 140.0, "Techno": 132.0, "DnB": 174.0}
GROUPS = {  # group -> (calibration material, SOURCE: 1 Drums / 2 Samples / 3 Voc / 0 All)
    "Kick": ("kick", 1), "Snare / Clap": ("snare", 1), "Hats / Cymbals": ("hats", 1), "Percussion": ("perc", 1),
    "Drum Bus": ("drums", 1), "808 / Bass": ("bass", 2), "Vocals": ("vocal", 3), "Keys / Piano": ("keys", 2),
    "Samples / Melody": ("samples", 2), "Master": ("mix", 0),
}
MODEL = {"VCA": 1, "FET": 2, "OPTO": 3, "TUBE": 4}
DEPTH = {"g": "Gentle", "m": "Medium", "h": "Heavy"}
GUESS = {"g": -18.0, "m": -24.0, "h": -32.0}
BEATS = {"r16": 0.25, "r8": 0.5, "r4": 1.0}
NOTE_NAME = {"r16": "1/16", "r8": "1/8", "r4": "1/4"}

# (group, name, depth, model, ratio, knee dB, attack ms, release ms | r16 / r8 / r4 (the genre's tempo),
#  detector "P" peak / "R" RMS, mix, extras)
# Kick / snare: attack 10 .. 25 ms lets the transient through (punch), < 3 ms shaves it and lifts the body (fat);
# the release resets before the next hit (the 1/16 of the tempo) or breathes with it (1/8, 1/4); parallel
# (mix < 1) for crush under the clean signal. docs/GENRE_PRESETS.md section 2.
K, S_, H, P, B, BA, V, KE, SA, M = GROUPS.keys()
RECIPES = {
    "Boom Bap": [
        (K, "Classic Knock", "m", "FET", 4, 4, 12, "r16", "P", 1.0, {}),
        (K, "Dusty Thump", "m", "TUBE", 3, 10, 2, "r8", "R", 1.0, {}),
        (K, "Jazzy Soft Kick", "g", "OPTO", 2.5, 8, 15, 120, "R", 1.0, {}),
        (K, "Gold Punch", "m", "VCA", 6, 3, 20, 90, "P", 1.0, {}),
        (K, "Underground Grit", "h", "FET", 8, 0, 0.5, "r16", "P", 0.5, {}),
        (K, "Lofi Squash", "h", "FET", 20, 0, 0.1, "r8", "P", 0.35, {}),
        (S_, "Classic Crack", "m", "FET", 4, 4, 8, "r16", "P", 1.0, {}),
        (S_, "Dusty Fat Snare", "m", "TUBE", 5, 8, 0.5, 60, "P", 1.0, {}),
        (S_, "Jazzy Brush", "g", "OPTO", 2, 10, 10, 150, "R", 1.0, {}),
        (S_, "Gold Snap", "m", "VCA", 4, 2, 25, 80, "P", 1.0, {}),
        (S_, "Underground Layer Glue", "m", "FET", 6, 2, 3, 70, "P", 1.0, {}),
        (S_, "Lofi Snare Parallel", "h", "FET", 20, 0, 0.1, "r16", "P", 0.4, {}),
        (H, "Classic Hat Even", "g", "VCA", 3, 6, 1, 60, "P", 1.0, {}),
        (H, "Dusty Soft Top", "m", "OPTO", 3, 10, 0.3, 100, "R", 1.0, {}),
        (H, "Jazzy Ride Smooth", "g", "OPTO", 2, 12, 8, 250, "R", 1.0, {}),
        (H, "Gold Tick", "g", "VCA", 4, 3, 10, 40, "P", 1.0, {}),
        (H, "Lofi Hat Crush", "h", "FET", 10, 0, 0.1, 50, "P", 0.3, {}),
        (P, "Classic Shaker Level", "g", "VCA", 3, 6, 2, 80, "P", 1.0, {}),
        (P, "Dusty Bongo Punch", "m", "FET", 4, 4, 10, 90, "P", 1.0, {}),
        (P, "Jazzy Tamb Glue", "g", "OPTO", 2.5, 10, 5, 200, "R", 1.0, {}),
        (P, "Underground Perc Smack", "m", "FET", 6, 2, 15, 50, "P", 1.0, {}),
        (B, "Classic Bus Glue", "g", "VCA", 2, 6, 30, "r8", "P", 1.0, {}),
        (B, "Dusty Tape Bus", "m", "TUBE", 3, 12, 15, "r4", "R", 1.0, {}),
        (B, "Jazzy Room Bus", "g", "OPTO", 2, 10, 20, 300, "R", 1.0, {}),
        (B, "Gold Knock Bus", "m", "VCA", 4, 4, 10, "r16", "P", 1.0, {}),
        (B, "Underground NY Parallel", "h", "FET", 8, 0, 1, "r16", "P", 0.4, {}),
        (B, "Lofi SP Squash", "h", "FET", 12, 0, 0.3, "r8", "P", 0.6, {}),
        (B, "Hard Knock Bus", "m", "FET", 4, 2, 5, 100, "P", 1.0, {}),
        (BA, "Classic Bass Level", "m", "OPTO", 4, 6, 10, 150, "R", 1.0, {}),
        (BA, "Dusty Warm Bass", "m", "TUBE", 3, 10, 20, 250, "R", 1.0, {}),
        (BA, "Jazzy Upright", "g", "OPTO", 2.5, 10, 15, 200, "R", 1.0, {}),
        (BA, "Gold Grip", "m", "FET", 6, 4, 3, 120, "P", 1.0, {}),
        (BA, "Underground Sub Hold", "g", "VCA", 3, 6, 30, "r4", "R", 1.0, {}),
        (V, "Classic Rap Upfront", "m", "FET", 4, 4, 1, 60, "P", 1.0, {}),
        (V, "Dusty Tube Vocal", "m", "TUBE", 3, 8, 5, 120, "R", 1.0, {}),
        (V, "Jazzy Smooth Vocal", "g", "OPTO", 3, 10, 10, 80, "R", 1.0, {}),
        (V, "Gold Adlib Control", "h", "FET", 8, 2, 0.5, 50, "P", 1.0, {"material": "backing"}),
        (V, "Underground Raw Parallel", "h", "FET", 12, 0, 0.3, 70, "P", 0.5, {}),
        (KE, "Classic Rhodes Smooth", "g", "OPTO", 2.5, 10, 10, 150, "R", 1.0, {}),
        (KE, "Dusty Piano Warm", "m", "TUBE", 3, 10, 15, 300, "R", 1.0, {}),
        (KE, "Jazzy Keys Breathe", "g", "OPTO", 2, 12, 20, 400, "R", 1.0, {}),
        (KE, "Gold Keys Pump", "m", "VCA", 4, 4, 5, "r8", "P", 1.0, {}),
        (SA, "Classic Soul Chop Glue", "m", "VCA", 3, 6, 10, "r8", "P", 1.0, {}),
        (SA, "Dusty Vinyl Sample", "m", "TUBE", 4, 10, 5, "r4", "R", 1.0, {}),
        (SA, "Jazzy Loop Level", "g", "OPTO", 2, 10, 15, 300, "R", 1.0, {}),
        (SA, "Lofi Sample Pump", "h", "FET", 8, 2, 1, "r8", "P", 0.6, {}),
        (M, "Classic Mix Glue", "g", "VCA", 2, 6, 30, "r8", "P", 1.0, {}),
        (M, "Dusty Tape Master", "g", "TUBE", 2, 12, 30, 400, "R", 1.0, {}),
        (M, "Gold Knock Master", "m", "VCA", 4, 6, 30, "r16", "P", 1.0, {}),
        (M, "Lofi Master Squash", "h", "FET", 4, 6, 10, "r8", "P", 0.5, {}),
    ],
    "Trap": [
        (K, "ATL Click", "m", "VCA", 6, 3, 15, 50, "P", 1.0, {}),
        (K, "Dark Tight Kick", "m", "VCA", 4, 4, 3, "r16", "P", 1.0, {}),
        (K, "Cloud Soft Kick", "g", "OPTO", 2.5, 8, 10, 120, "R", 1.0, {}),
        (K, "Rage Slam", "h", "FET", 10, 0, 5, 60, "P", 1.0, {}),
        (K, "Memphis Knock", "m", "FET", 4, 4, 10, "r16", "P", 1.0, {}),
        (K, "Luxury Clean Punch", "m", "VCA", 3, 6, 20, 80, "P", 1.0, {}),
        (S_, "ATL Clap Snap", "m", "VCA", 4, 2, 10, 60, "P", 1.0, {}),
        (S_, "Dark Snare Body", "m", "FET", 5, 4, 1, 50, "P", 1.0, {}),
        (S_, "Cloud Clap Smooth", "g", "OPTO", 3, 10, 5, 120, "R", 1.0, {}),
        (S_, "Rage Crack", "h", "FET", 8, 0, 3, 40, "P", 0.6, {}),
        (S_, "Memphis Rimshot", "m", "FET", 4, 4, 8, "r16", "P", 1.0, {}),
        (S_, "Luxury Layer Glue", "g", "VCA", 2, 6, 15, "r16", "P", 1.0, {}),
        (H, "ATL Roll Even", "m", "VCA", 4, 4, 0.5, 30, "P", 1.0, {}),
        (H, "Dark Hat Tame", "g", "OPTO", 3, 10, 1, 60, "R", 1.0, {}),
        (H, "Rage Hat Bite", "m", "FET", 6, 2, 5, 30, "P", 1.0, {}),
        (H, "Memphis Hat Grit", "h", "FET", 10, 0, 0.3, 40, "P", 0.4, {}),
        (H, "Luxury Open Hat Tail", "g", "VCA", 3, 6, 2, 150, "P", 1.0, {}),
        (P, "ATL Perc Pop", "m", "VCA", 4, 3, 10, 50, "P", 1.0, {}),
        (P, "Dark Perc Level", "g", "OPTO", 3, 10, 3, 100, "R", 1.0, {}),
        (P, "Rage Perc Smash", "h", "FET", 10, 0, 1, 40, "P", 0.5, {}),
        (P, "Luxury Shaker Smooth", "g", "VCA", 2, 8, 5, 100, "P", 1.0, {}),
        (B, "ATL Bus Glue", "g", "VCA", 2, 6, 30, "r16", "P", 1.0, {}),
        (B, "Dark Bus Weight", "m", "TUBE", 3, 10, 20, "r8", "R", 1.0, {}),
        (B, "Cloud Bus Soft", "g", "OPTO", 2, 12, 20, 300, "R", 1.0, {}),
        (B, "Rage Bus Slam", "h", "FET", 6, 2, 3, "r16", "P", 1.0, {}),
        (B, "Memphis Parallel Crush", "h", "FET", 20, 0, 0.3, "r16", "P", 0.35, {}),
        (B, "Luxury Bus Punch", "m", "VCA", 4, 4, 15, "r16", "P", 1.0, {}),
        (B, "Dark Parallel Body", "h", "FET", 10, 0, 1, "r8", "P", 0.4, {}),
        (BA, "ATL 808 Even", "m", "OPTO", 4, 6, 15, 300, "R", 1.0, {}),
        (BA, "Dark 808 Hold", "m", "VCA", 3, 8, 30, "r4", "R", 1.0, {}),
        (BA, "Rage 808 Grip", "h", "FET", 8, 2, 3, 150, "P", 1.0, {}),
        (BA, "Memphis 808 Knock", "m", "FET", 4, 4, 20, 100, "P", 1.0, {}),
        (BA, "Luxury Sub Smooth", "g", "TUBE", 2.5, 12, 25, 400, "R", 1.0, {}),
        (V, "ATL Vocal Upfront", "h", "FET", 6, 2, 1, 60, "P", 1.0, {}),
        (V, "Dark Vocal Tube", "m", "TUBE", 4, 8, 8, 60, "R", 1.0, {}),
        (V, "Cloud Vocal Smooth", "m", "OPTO", 3, 10, 10, 120, "R", 1.0, {}),
        (V, "Rage Vocal Crush", "h", "FET", 20, 0, 0.3, 50, "P", 0.5, {}),
        (V, "Luxury Adlib Tame", "m", "VCA", 4, 4, 3, 80, "P", 1.0, {"material": "backing"}),
        (KE, "ATL Piano Level", "g", "OPTO", 2.5, 10, 8, 120, "R", 1.0, {}),
        (KE, "Dark Keys Pump", "m", "VCA", 4, 4, 5, "r8", "P", 1.0, {}),
        (KE, "Cloud Pad Glue", "g", "TUBE", 2, 12, 30, 400, "R", 1.0, {}),
        (KE, "Luxury Bell Control", "m", "VCA", 3, 6, 2, 100, "P", 1.0, {}),
        (SA, "ATL Loop Glue", "g", "VCA", 2, 6, 20, "r8", "P", 1.0, {}),
        (SA, "Dark Melody Weight", "m", "TUBE", 3, 10, 15, "r4", "R", 1.0, {}),
        (SA, "Cloud Guitar Smooth", "g", "OPTO", 2.5, 10, 10, 200, "R", 1.0, {}),
        (SA, "Rage Synth Bite", "m", "FET", 6, 2, 3, "r16", "P", 1.0, {}),
        (M, "ATL Mix Glue", "g", "VCA", 2, 6, 30, "r16", "P", 1.0, {}),
        (M, "Dark Master Weight", "g", "TUBE", 2, 12, 30, 300, "R", 1.0, {}),
        (M, "Rage Loud Bus", "m", "FET", 4, 4, 10, "r16", "P", 1.0, {}),
        (M, "Luxury Clean Master", "g", "VCA", 1.5, 10, 30, "r8", "P", 1.0, {}),
    ],
    "Techno": [
        (K, "Peak Kick Punch", "m", "VCA", 4, 4, 15, "r16", "P", 1.0, {}),
        (K, "Hypnotic Kick Pump", "h", "VCA", 6, 2, 2, "r4", "P", 1.0, {}),
        (K, "Minimal Kick Tight", "g", "VCA", 3, 6, 10, 60, "P", 1.0, {}),
        (K, "Detroit Kick Warm", "m", "TUBE", 3, 10, 10, "r16", "R", 1.0, {}),
        (K, "Hard Kick Distort", "h", "FET", 12, 0, 0.3, 50, "P", 1.0, {}),
        (K, "Dub Kick Round", "g", "OPTO", 2.5, 10, 20, "r8", "R", 1.0, {}),
        (S_, "Peak Clap Snap", "m", "VCA", 4, 3, 10, 60, "P", 1.0, {}),
        (S_, "Minimal Clap Tight", "g", "VCA", 3, 6, 15, 40, "P", 1.0, {}),
        (S_, "Detroit Clap Room", "m", "OPTO", 3, 10, 3, "r8", "R", 1.0, {}),
        (S_, "Acid Clap Bite", "m", "FET", 6, 2, 5, 50, "P", 1.0, {}),
        (S_, "Hard Clap Smash", "h", "FET", 10, 0, 1, "r16", "P", 0.6, {}),
        (S_, "Dub Snare Space", "g", "TUBE", 2.5, 12, 10, "r4", "R", 1.0, {}),
        (H, "Peak Hat Drive", "m", "FET", 4, 4, 3, "r16", "P", 1.0, {}),
        (H, "Hypnotic Hat Groove", "m", "VCA", 4, 4, 8, "r8", "P", 1.0, {}),
        (H, "Minimal Hat Tick", "g", "VCA", 3, 3, 15, 30, "P", 1.0, {}),
        (H, "Acid Hat Grit", "h", "FET", 8, 0, 0.5, 30, "P", 0.45, {}),
        (H, "Hard Ride Control", "g", "OPTO", 3, 10, 2, 150, "R", 1.0, {}),
        (P, "Peak Perc Pop", "m", "VCA", 4, 3, 12, "r16", "P", 1.0, {}),
        (P, "Hypnotic Perc Loop", "m", "OPTO", 3, 8, 5, "r8", "R", 1.0, {}),
        (P, "Minimal Click Level", "g", "VCA", 3, 6, 1, 40, "P", 1.0, {}),
        (P, "Detroit Conga Warm", "g", "TUBE", 2.5, 10, 10, 120, "R", 1.0, {}),
        (B, "Peak Bus Glue", "g", "VCA", 2, 6, 30, "r8", "P", 1.0, {}),
        (B, "Hypnotic Bus Pump", "m", "VCA", 4, 4, 10, "r4", "P", 1.0, {}),
        (B, "Minimal Bus Clean", "g", "VCA", 1.5, 8, 30, "r8", "P", 1.0, {}),
        (B, "Detroit Bus Warm", "m", "TUBE", 3, 10, 20, "r8", "R", 1.0, {}),
        (B, "Hard Bus Crush", "h", "FET", 8, 0, 1, "r16", "P", 1.0, {}),
        (B, "Hard Parallel Smash", "h", "FET", 20, 0, 0.1, "r16", "P", 0.35, {}),
        (B, "Dub Rumble Group", "m", "OPTO", 4, 8, 5, "r4", "R", 1.0, {}),
        (BA, "Peak Bass Lock", "m", "VCA", 4, 4, 5, "r16", "P", 1.0, {}),
        (BA, "Hypnotic Rumble Control", "m", "OPTO", 4, 8, 3, "r8", "R", 1.0, {}),
        (BA, "Acid 303 Squeeze", "h", "FET", 8, 2, 1, 80, "P", 1.0, {}),
        (BA, "Detroit Bass Warm", "m", "TUBE", 3, 10, 15, 200, "R", 1.0, {}),
        (BA, "Minimal Sub Even", "g", "OPTO", 2.5, 10, 20, 300, "R", 1.0, {}),
        (V, "Peak Vocal Chop", "m", "FET", 4, 4, 2, 60, "P", 1.0, {}),
        (V, "Hypnotic Whisper Level", "h", "OPTO", 4, 10, 5, 100, "R", 1.0, {}),
        (V, "Detroit Soul Vocal", "m", "TUBE", 3, 8, 8, 120, "R", 1.0, {}),
        (V, "Dub Vocal Space", "g", "OPTO", 2.5, 12, 15, "r4", "R", 1.0, {}),
        (V, "Hard Shout Crush", "h", "FET", 10, 0, 0.5, 50, "P", 0.6, {}),
        (KE, "Detroit Chord Stab", "m", "VCA", 4, 4, 5, "r16", "P", 1.0, {}),
        (KE, "Dub Chord Pump", "m", "VCA", 4, 4, 2, "r4", "P", 1.0, {}),
        (KE, "Peak Pad Glue", "g", "TUBE", 2, 12, 30, 500, "R", 1.0, {}),
        (KE, "Minimal Keys Level", "g", "OPTO", 2, 8, 15, 200, "R", 1.0, {}),
        (SA, "Acid Lead Squeeze", "m", "FET", 6, 2, 3, 60, "P", 1.0, {}),
        (SA, "Hypnotic Loop Pump", "m", "VCA", 4, 4, 5, "r4", "P", 1.0, {}),
        (SA, "Peak Synth Glue", "g", "VCA", 2, 6, 20, "r8", "P", 1.0, {}),
        (SA, "Hard Noise FX Crush", "h", "FET", 10, 0, 0.5, "r16", "P", 0.5, {}),
        (M, "Peak Club Glue", "g", "VCA", 2, 6, 30, "r8", "P", 1.0, {}),
        (M, "Hypnotic Master Breath", "g", "VCA", 2, 6, 10, "r4", "P", 1.0, {}),
        (M, "Detroit Warm Master", "g", "TUBE", 2, 12, 20, "r4", "R", 1.0, {}),
        (M, "Hard Loud Bus", "m", "FET", 4, 4, 10, "r16", "P", 1.0, {}),
    ],
    "DnB": [
        (K, "Modern Kick Punch", "m", "VCA", 4, 4, 10, 50, "P", 1.0, {}),
        (K, "Roller Kick Tight", "m", "FET", 4, 4, 5, "r16", "P", 1.0, {}),
        (K, "Liquid Kick Soft", "g", "OPTO", 2.5, 8, 10, 100, "R", 1.0, {}),
        (K, "Neuro Kick Smash", "h", "FET", 10, 0, 1, 40, "P", 1.0, {}),
        (K, "Jump-Up Kick Knock", "m", "FET", 6, 2, 15, 40, "P", 1.0, {}),
        (K, "Break Kick Level", "g", "VCA", 3, 6, 3, "r16", "P", 1.0, {}),
        (S_, "Modern Snare Crack", "m", "VCA", 4, 2, 10, 50, "P", 1.0, {}),
        (S_, "Roller Snare Body", "m", "FET", 5, 4, 0.5, 40, "P", 1.0, {}),
        (S_, "Liquid Snare Smooth", "g", "OPTO", 3, 10, 5, 100, "R", 1.0, {}),
        (S_, "Neuro Snare Slam", "h", "FET", 8, 0, 2, 30, "P", 0.6, {}),
        (S_, "Jump-Up Snare Smack", "m", "FET", 6, 2, 15, 40, "P", 1.0, {}),
        (S_, "Break Ghost Lift", "m", "VCA", 4, 6, 1, "r16", "R", 1.0, {}),
        (H, "Modern Shuffle Even", "g", "VCA", 3, 6, 1, 30, "P", 1.0, {}),
        (H, "Roller Hat Drive", "m", "FET", 4, 4, 3, "r16", "P", 1.0, {}),
        (H, "Liquid Ride Smooth", "g", "OPTO", 2.5, 12, 5, 150, "R", 1.0, {}),
        (H, "Neuro Hat Bite", "m", "FET", 6, 2, 5, 25, "P", 1.0, {}),
        (H, "Break Cymbal Tame", "g", "OPTO", 3, 10, 1, 80, "R", 1.0, {}),
        (P, "Modern Perc Pop", "m", "VCA", 4, 3, 10, 40, "P", 1.0, {}),
        (P, "Liquid Shaker Smooth", "g", "OPTO", 2.5, 10, 3, 100, "R", 1.0, {}),
        (P, "Neuro Perc Smash", "h", "FET", 10, 0, 0.5, 30, "P", 0.5, {}),
        (P, "Break Perc Glue", "g", "VCA", 2.5, 6, 10, "r8", "P", 1.0, {}),
        (B, "Modern Bus Glue", "g", "VCA", 2, 6, 30, "r8", "P", 1.0, {}),
        (B, "Roller Bus Punch", "m", "VCA", 4, 4, 10, "r16", "P", 1.0, {}),
        (B, "Liquid Bus Soft", "g", "OPTO", 2, 10, 20, 250, "R", 1.0, {}),
        (B, "Neuro Bus Slam", "h", "FET", 6, 2, 3, "r16", "P", 1.0, {}),
        (B, "Amen Break Fatten", "h", "FET", 8, 2, 1, "r16", "P", 0.5, {}),
        (B, "Break Tape Squash", "h", "TUBE", 6, 6, 0.5, 60, "P", 0.6, {}),
        (B, "Jump-Up Bus Loud", "m", "FET", 4, 4, 10, 50, "P", 1.0, {}),
        (BA, "Modern Reese Control", "m", "VCA", 4, 4, 5, "r8", "P", 1.0, {}),
        (BA, "Roller Sub Even", "m", "OPTO", 4, 8, 15, 200, "R", 1.0, {}),
        (BA, "Liquid Bass Warm", "g", "TUBE", 2.5, 10, 20, 250, "R", 1.0, {}),
        (BA, "Neuro Bass Squeeze", "h", "FET", 8, 2, 1, 60, "P", 1.0, {}),
        (BA, "Jump-Up Wobble Grip", "h", "FET", 6, 2, 3, 80, "P", 1.0, {}),
        (V, "Modern MC Upfront", "h", "FET", 6, 2, 1, 50, "P", 1.0, {}),
        (V, "Liquid Vocal Silk", "m", "OPTO", 3, 10, 10, 100, "R", 1.0, {}),
        (V, "Roller Vocal Chop", "m", "VCA", 4, 4, 3, 60, "P", 1.0, {}),
        (V, "Jump-Up MC Hype", "h", "FET", 10, 0, 0.5, 40, "P", 0.6, {}),
        (V, "Liquid Backing Air", "g", "OPTO", 2.5, 12, 15, 150, "R", 1.0, {"material": "backing"}),
        (KE, "Liquid Rhodes Smooth", "g", "OPTO", 2.5, 12, 12, 180, "R", 1.0, {}),
        (KE, "Liquid Piano Breath", "m", "TUBE", 3, 10, 20, 350, "R", 1.0, {}),
        (KE, "Modern Stab Tight", "m", "VCA", 4, 4, 3, "r16", "P", 1.0, {}),
        (KE, "Roller Pad Pump", "m", "VCA", 4, 4, 2, "r4", "P", 1.0, {}),
        (SA, "Liquid Strings Glue", "g", "TUBE", 2, 12, 30, 400, "R", 1.0, {}),
        (SA, "Neuro Synth Bite", "m", "FET", 6, 2, 2, 50, "P", 1.0, {}),
        (SA, "Break Loop Level", "m", "VCA", 3, 6, 10, "r8", "P", 1.0, {}),
        (SA, "Jump-Up Hoover Grip", "h", "FET", 8, 2, 1, 60, "P", 1.0, {}),
        (M, "Modern Mix Glue", "g", "VCA", 2, 6, 30, "r8", "P", 1.0, {}),
        (M, "Liquid Warm Master", "g", "TUBE", 2.5, 10, 30, 250, "R", 1.0, {}),
        (M, "Neuro Loud Bus", "m", "FET", 4, 4, 10, "r16", "P", 1.0, {}),
        (M, "Roller Master Punch", "m", "VCA", 3, 6, 20, "r16", "P", 1.0, {}),
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
           "// name, mix, values, material, depth, genre, group, note"]
    names = set()
    for genre, bpm in GENRES.items():
        recipes = RECIPES[genre]
        assert len(recipes) == 50, (genre, len(recipes))
        for group, name, depth, model, ratio, knee, attack, rel, det, mix, extra in recipes:
            key = f"{genre}/{group}/{name}"
            assert key not in names, key
            names.add(key)
            material, source = GROUPS[group]
            material = extra.get("material", material)
            if isinstance(rel, str):
                ms = round(60000.0 / bpm * BEATS[rel])
                release = f"{ms} ms ({NOTE_NAME[rel]} at {fmt(bpm)} BPM)"
            else:
                ms = rel
                release = f"{ms} ms"
            values = {"model": MODEL[model], "source": source, "thresholdDb": cal.get(key, GUESS[depth]), "ratio": float(ratio),
                      "kneeDb": float(knee), "attackMs": float(attack), "releaseMs": float(min(1000, max(10, ms))),
                      "detector": 0 if det == "P" else 1}
            note = f"{model} {fmt(ratio)}:1, attack {fmt(attack)} ms, release {release}" + (", RMS" if det == "R" else "")
            if mix < 1.0:
                note += f", parallel {round(mix * 100)} %"
            vals = ", ".join(f'{{ "{k}", {fmt(v) if isinstance(v, float) else v} }}' for k, v in values.items())
            out.append(f'{{ "{name}", {float(mix)}f, {{ {vals} }}, "{material}", Preset::{DEPTH[depth]}, "{genre}", "{group}", "{note}" }},')
    Path(__file__).resolve().parent.parent.joinpath("Source/Plugin/GenrePresets.inc").write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"{len(names)} genre presets, {len(cal)} calibrated")


if __name__ == "__main__":
    main()
