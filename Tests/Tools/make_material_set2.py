"""Material set 2 for HPEQ_Lab presets (presets pass 3; set 1, the owner's first folder, is lost).
The sources are the owner's files on drive E: (vocal takes and mixes, sample folders); none of it is in git.

Each clip: 48 kHz stereo float WAV, up to 60 s around its loudest part, gained so the loudest 24 s
(the window HPEQ_Lab uses) has the old material's integrated loudness (from CALIBRATION.md), unless the
sample peak would pass -0.3 dBFS (then the peak sets the gain; reported).
Usage: python -I make_material_set2.py <out dir>   (--max <wav...>: the loudest each clip can get under the peak)
"""
import sys, json, numpy as np, scipy.io.wavfile as wf, scipy.signal as ss, warnings
warnings.simplefilter("ignore")
sys.stdout.reconfigure(encoding="utf-8")
FS = 48000
E = "E:/"
ST = E + "Sample pack/track stems/"
BB = E + "Downloads/Boom Bap Box/"
NV = BB + "{}/Navie D - Boom Bap {}.wav"

def load(path):
    sr, x = wf.read(path)
    x = np.asarray(x)
    if x.dtype.kind == 'i': x = x.astype(np.float64) / (np.iinfo(x.dtype).max + 1.0)
    elif x.dtype.kind == 'u': x = (x.astype(np.float64) - 128) / 128
    else: x = x.astype(np.float64)
    if x.ndim == 1: x = x[:, None]
    if x.shape[1] == 1: x = np.repeat(x, 2, axis=1)
    x = x[:, :2]
    if sr != FS:
        g = np.gcd(sr, FS)
        x = ss.resample_poly(x, FS // g, sr // g, axis=0)
    return x

# BS.1770-4 K-weighting at 48 kHz and gated integrated loudness
KB1, KA1 = [1.53512485958697, -2.69169618940638, 1.19839281085285], [1.0, -1.69065929318241, 0.73248077421585]
KB2, KA2 = [1.0, -2.0, 1.0], [1.0, -1.99004745483398, 0.99007225036621]
def lufs(x):
    y = ss.lfilter(KB2, KA2, ss.lfilter(KB1, KA1, x, axis=0), axis=0)
    blk, hop = int(0.4 * FS), int(0.1 * FS)
    p = np.array([np.sum(np.mean(y[s:s + blk] ** 2, axis=0)) for s in range(0, len(y) - blk + 1, hop)])
    p = p[-0.691 + 10 * np.log10(p + 1e-20) > -70]
    if len(p) == 0: return -100.0
    rel = -0.691 + 10 * np.log10(np.mean(p)) - 10
    p = p[-0.691 + 10 * np.log10(p) > rel]
    return -0.691 + 10 * np.log10(np.mean(p))

def loudest(x, seconds):
    n = min(len(x), int(seconds * FS))
    best, be = 0, -1
    for s in range(0, len(x) - n + 1, FS):
        e = np.sum(x[s:s + n:16] ** 2)
        if e > be: be, best = e, s
    return best, n

def window(x, seconds=60):
    # keep up to 60 s centred on the loudest 24 s
    s, n = loudest(x, 24)
    a = max(0, s - int((seconds - 24) / 2 * FS))
    return x[a:a + int(seconds * FS)]

def seq(hits, bpm, bars, pattern):
    """pattern: list of (step16 in bar, hit index, gain); hits cycle per 2 bars"""
    step = 60.0 / bpm / 4
    length = int(bars * 16 * step * FS) + FS
    out = np.zeros((length, 2))
    for bar in range(bars):
        for st, k, g in pattern:
            h = hits[(k + bar // 2) % len(hits)]
            t = int(((bar * 16 + st) * step) * FS)
            m = min(len(h), length - t)
            out[t:t + m] += g * h[:m]
    return out

def oneshots(folder, kind, names):
    return [load(NV.format(folder, kind + " - " + n)) for n in names]

def build():
    src = {}
    lead = [E + "Adobe/Голос твой/Голос твой _Recorded/Track 2_008.wav",
            E + "Adobe/Untitled Session 6/Untitled Session 6_Recorded/Track 2_016.wav",
            E + "Adobe/Untitled Session 10/Untitled Session 10_Recorded/Track 2_002.wav"]
    back = [E + "Adobe/Untitled Session 6/Untitled Session 6_Recorded/Track 3_003.wav",
            E + "Adobe/Untitled Session 10/Untitled Session 10_Recorded/Track 3_017.wav",
            E + "Adobe/Untitled Session 10/Untitled Session 10_Recorded/Track 4_021.wav"]
    for i, f in enumerate(lead): src[f"vocal_lead{i+1}"] = (f, -16.2)
    for i, f in enumerate(back): src[f"vocal_back{i+1}"] = (f, -19.9)
    src["mixdown_Ardnold"] = (E + "Downloads/HaM - это всё перманентно_mixdown.wav", -14.1)
    src["mixdown_boom_bap_3"] = (E + "Downloads/630--boom-bap--underground_TK9336094.wav.wav", -14.1)
    src["mixdown_sad_trap"] = (E + "Downloads/family-tales--sad--trap_TK14200059.wav.wav", -14.1)
    src["drumbus1"] = (ST + "1_Boombap_Drum_Kit_80_BPM_NA.wav", -20.9)
    src["bass1"] = (ST + "3_Sub_Bass_80_BPM_NA_6.wav", -9.4)
    src["bass2"] = (ST + "1_Deep_Bass_90_BPM_F_minor.wav", -9.4)
    src["guitar"] = (ST + "4_Guitar_Chords_80_BPM_NA.wav", -28.5)
    src["keys"] = (ST + "1_Piano_90_BPM_A_minor.wav", -21.7)
    src["keys2"] = (ST + "2_Felt_Piano_80_BPM_A_minor.wav", -21.7)
    src["synth"] = (ST + "2_Atmospheric_Synths_110_BPM_E_minor.wav", -24.0)
    clips = {k: (window(load(f)), t, f) for k, (f, t) in src.items()}

    # drum parts: 90 BPM, 24 bars from Boom Bap Box one-shots (fixed hits, fixed patterns)
    kicks = oneshots("Kicks", "Kick", ["Arrow", "Charactor", "Dotter", "Charactor"])
    snares = oneshots("Snares", "Snare", ["Alaska", "Brutal", "Charcoal", "Chives"])
    hats = oneshots("Hi Hat", "Hi Hat", ["Abstract", "Brat", "Cloth", "Drunk"])
    perc = [load(BB + "Percussion/Navie D - Boom Bap " + n + ".wav") for n in
            ["Shaker - Baglady", "Tambourine - Dial", "Stick - Clab", "Ride - Bun", "Shaker - Forex", "Chime - Madox"]]
    clips["kick"] = (seq(kicks, 90, 24, [(0, 0, 1.0), (3, 0, 0.9), (6, 0, 0.9), (8, 0, 1.0), (10, 0, 1.0), (13, 0, 0.9)]), -7.7, "Boom Bap Box kicks")
    clips["snare"] = (seq(snares, 90, 24, [(4, 0, 1.0), (12, 0, 1.0), (15, 0, 0.25), (9, 0, 0.2)]), -15.6, "Boom Bap Box snares")
    clips["hihat"] = (seq(hats, 90, 24, [(s, 0, g) for s, g in zip(range(0, 16, 2), [1, .6, .9, .55, 1, .6, .9, .7])] + [(15, 1, 0.4)]), -16.4, "Boom Bap Box hats")
    clips["perc"] = (seq(perc, 90, 24, [(2, 0, .7), (6, 1, .6), (10, 0, .7), (13, 2, .5), (14, 1, .6), (0, 3, .4), (8, 5, .35)]), -20.3, "Boom Bap Box percussion")

    report = {}
    for name, (x, target, origin) in clips.items():
        s, n = loudest(x, 24)
        now = lufs(x[s:s + n])
        gain = target - now
        peak = 20 * np.log10(np.max(np.abs(x)) + 1e-12)
        capped = peak + gain > -0.3
        if capped: gain = -0.3 - peak
        y = (x * 10 ** (gain / 20)).astype(np.float32)
        wf.write(f"{sys.argv[1]}/{name}.wav", FS, y)
        got = lufs(y[s:s + n].astype(np.float64))
        report[name] = {"source": origin, "seconds": round(len(y) / FS, 1), "targetLufs": target, "lufs": round(got, 1), "gainDb": round(gain, 1), "peakLimited": bool(capped)}
        print(f"{name:20s} {got:6.1f} LUFS (target {target}) gain {gain:+5.1f}{' PEAK' if capped else ''}  {origin}")
    json.dump(report, open(f"{sys.argv[1]}/material.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)

if sys.argv[1] == "--max":
    for f in sys.argv[2:]:
        x = window(load(f)); s, n = loudest(x, 24); pk = 20*np.log10(np.max(np.abs(x)))
        print(f"{lufs(x[s:s+n]) - 0.3 - pk:6.1f} max LUFS  {f}")
else:
    build()
