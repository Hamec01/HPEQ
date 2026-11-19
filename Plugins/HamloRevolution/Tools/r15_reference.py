"""R15 acoustic reference measurement (owner request: acoustic fills only, no electronic ones).

Selection is by explicit labels only:
  A_acoustic   MusicRadar IF_AcouFill / IF_AcouFillAlt (tempo from folder), Wave Alchemy 'acoustic fills'
  B_live_bb    Sonic Mechanics Boom Bap Breaks drum fills (live breaks by pack, not by file name): reported apart
  C_snare_roll MusicRadar IF_LudSnrRoll (acoustic Ludwig snare rolls): reported apart
Excluded: electronic packs (Vandalism, Cymatics trap 808, Chop Shop house, Wave Alchemy electronic/experimental),
MusicRadar AcouGlitchChop (processed chops), KikSnrFill / SnareRoll (source not labelled), Axel's (mixed origin),
vinyl fills (no tempo). For comparison the same tool measures the last bar of R15 and R14 4-beat lab renders.

Usage: python -I Tools/r15_reference.py <fills root> <lab render folder> <out folder>
"""
import collections, json, math, os, re, statistics, subprocess, sys, wave

root, labdir, outdir = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(outdir, exist_ok=True)
rows = []
mr = os.path.join(root, 'Musicradar Drum Fills')
for folder in sorted(os.listdir(mr)):
    m = re.match(r'(\d+)bpm', folder)
    if not m:
        continue
    for f in sorted(os.listdir(os.path.join(mr, folder))):
        if not f.lower().endswith('.wav'):
            continue
        if f.startswith('IF_AcouFill'):
            rows.append(('A_acoustic', m.group(1), os.path.join(mr, folder, f)))
        elif f.startswith('IF_LudSnrRoll'):
            rows.append(('C_snare_roll', m.group(1), os.path.join(mr, folder, f)))
wa = os.path.join(root, 'Wave.Alchemy.Drum.Fills.and.Builds.WAV.REX2.AiFF-MAGNETRiXX', 'wav', 'acoustic fills')
for f in sorted(os.listdir(wa)):
    m = re.match(r'wafb_fill_acoustic_(\d+)_', f)
    if m:
        rows.append(('A_acoustic', m.group(1), os.path.join(wa, f)))
bb = os.path.join(root, 'Sonic Mechanics - Boom Bap Breaks', 'BBB_WAV_LOOPS', 'BBB_DRUM_FILLS')
for f in sorted(os.listdir(bb)):
    m = re.match(r'BBB_(\d+)_BPM_DRUM_FILL', f)
    if m:
        rows.append(('B_live_bb', m.group(1), os.path.join(bb, f)))

# R15 / R14 lab renders: listen file = loop (2 bars) + render (2 bars) + 1 beat; keep the first 4 bars so that the
# tool's "last 4 beats" is the fill bar of the 4-beat edit
tempo = {'boombap': 90, 'trap': 140, 'dnb': 172, 'techno': 128}
cut = os.path.join(outdir, 'lab_bars')
os.makedirs(cut, exist_ok=True)
for f in sorted(os.listdir(labdir)):
    m = re.match(r'(boombap|trap|dnb|techno)_kit\d_(phrase|r14)_4b_s\d\.wav', f)
    if not m:
        continue
    bpm = tempo[m.group(1)]
    with wave.open(os.path.join(labdir, f)) as w:
        params = w.getparams()
        bar = round(4 * w.getframerate() * 60 / bpm)
        frames = w.readframes(4 * bar)
    target = os.path.join(cut, f)
    with wave.open(target, 'wb') as o:
        o.setparams(params)
        o.writeframes(frames)
    rows.append(('R15_phrase' if m.group(2) == 'phrase' else 'R14_compose', str(bpm), target))

manifest = os.path.join(outdir, 'manifest.tsv')
with open(manifest, 'w', encoding='utf-8') as f:
    for r in rows:
        f.write('\t'.join(r) + '\n')
hits_path = os.path.join(outdir, 'hits.tsv')
tool = os.path.join('build', 'Release', 'HamloRevolution_DrumFillReferenceLab.exe')
print(subprocess.run([tool, manifest, hits_path], capture_output=True, text=True).stdout.strip())

files = collections.defaultdict(list)
with open(hits_path, encoding='utf-8') as f:
    header = f.readline().rstrip('\n').split('\t')
    for line in f:
        v = dict(zip(header, line.rstrip('\n').split('\t')))
        if v['secondary'] == '1':
            continue
        files[(v['group'], v['file'])].append(v)
group_files = collections.Counter(r[0] for r in rows)


def nearest(t, step):
    return abs(t - step * round(t / step))


def per_file(hits, bpm):
    ticks = sorted((float(h['tick']), h) for h in hits if -2 <= float(h['tick']) < 96)
    if not ticks:
        return None
    peaks = [float(h['peak']) for _, h in ticks]
    top = max(peaks) or 1
    per_beat = [0, 0, 0, 0]
    early = late = weight = sync = 0.0
    classes = collections.Counter()
    micro = []
    roles = collections.Counter()
    tomlike = 0
    for t, h in ticks:
        p = float(h['peak']) / top
        b = min(3, max(0, int(t // 24)))
        per_beat[b] += 1
        if t >= 48:
            late += p * p
        else:
            early += p * p
        d16, d3, dt = nearest(t, 6), nearest(t, 3), nearest(t, 4)
        if nearest(t, 12) <= 0.75:
            cls = '8th'
        elif d16 <= 0.75:
            cls = '16th'
        elif dt <= 0.75 and dt < d3:
            cls = 'triplet'
        elif d3 <= 0.75:
            cls = '32nd'
        else:
            cls = 'off-grid'
        classes[cls] += 1
        micro.append(min(d16, d3, dt) / 24 * 60000 / bpm)
        pos = (t % 24) / 6
        s16 = round(pos) % 4
        sync += p * ((1.0 if s16 in (1, 3) else 0.5 if s16 == 2 else 0.0) if abs(pos - round(pos)) < 0.2 else 0.4)
        weight += p
        role = int(h['role'])
        roles[role] += 1
        if role in (0, 1) and float(h['lowmid']) >= 0.45 and float(h['high']) < 0.12 and float(h['decay']) >= 80:
            tomlike += 1
    iois = [b[0] - a[0] for a, b in zip(ticks, ticks[1:]) if b[0] - a[0] > 0.5]
    run = best = 1
    for a, b in zip(ticks, ticks[1:]):
        run = run + 1 if abs((b[0] - a[0]) - 6) <= 1.0 or abs((b[0] - a[0]) - 3) <= 0.8 or abs((b[0] - a[0]) - 4) <= 0.8 else 1
        best = max(best, run)
    beat_peak = [0.0] * 4
    for t, h in ticks:
        beat_peak[min(3, max(0, int(t // 24)))] += float(h['peak']) / top
    n = len(ticks)
    first_dense = next((i for i in range(4) if per_beat[i] >= 3), 4)
    return {
        'hits': n,
        'per_beat': per_beat,
        'roles': [roles[0] / n, roles[1] / n, roles[2] / n],
        'tomlike': tomlike / n,
        'classes': {k: classes[k] / n for k in ('8th', '16th', '32nd', 'triplet', 'off-grid')},
        'sync': sync / weight if weight else 0,
        'tension': (late - early) / (late + early) if late + early else 0,
        # position classes split half-way between grid points. First run used t >= 89 / 90 / 84: a last-16th hit
        # 1..2 ms early (slice pre-roll, rushing, human timing) counted as a rest (R15c, disclosed in the report)
        'landing_accent': any(t >= 87 and float(h['peak']) / top >= 0.8 for t, h in ticks),
        'rest_last_16th': not any(t >= 87 for t, _ in ticks),
        'rest_last_8th': not any(t >= 81 for t, _ in ticks),
        'micro_ms': statistics.median(micro),
        'fastest_ioi_ticks': min(iois) if iois else 24,
        'longest_run': best,
        'dyn_last_first': beat_peak[3] / beat_peak[0] if beat_peak[0] > 0 else None,
        'first_dense_beat': first_dense,
    }


groups = collections.defaultdict(list)
bpm_of = {(r[0], os.path.basename(r[2])): float(r[1]) for r in rows}
for (g, name), hits in files.items():
    m = per_file(hits, bpm_of.get((g, name), 120))
    if m:
        groups[g].append(m)


def q(values, p):
    values = sorted(v for v in values if v is not None)
    if not values:
        return float('nan')
    return values[min(len(values) - 1, int(p * (len(values) - 1) + 0.5))]


summary = {}
order = ['A_acoustic', 'B_live_bb', 'C_snare_roll', 'R15_phrase', 'R14_compose']
for g in order:
    ms = groups.get(g, [])
    if not ms:
        continue
    s = {
        'files_listed': group_files[g], 'files_measured': len(ms),
        'hits_per_bar_median': q([m['hits'] for m in ms], 0.5),
        'hits_per_beat_mean': [round(statistics.mean(m['per_beat'][i] for m in ms), 2) for i in range(4)],
        'role_share_kick_snare_hat': [round(statistics.mean(m['roles'][i] for m in ms), 3) for i in range(3)],
        'tomlike_share': round(statistics.mean(m['tomlike'] for m in ms), 3),
        'grid_class_share': {k: round(statistics.mean(m['classes'][k] for m in ms), 3) for k in ('8th', '16th', '32nd', 'triplet', 'off-grid')},
        'syncopation_median_p25_p75': [round(q([m['sync'] for m in ms], p), 3) for p in (0.5, 0.25, 0.75)],
        'tension_median_p25_p75': [round(q([m['tension'] for m in ms], p), 3) for p in (0.5, 0.25, 0.75)],
        'landing_accent_share': round(statistics.mean(m['landing_accent'] for m in ms), 3),
        'rest_last_16th_share': round(statistics.mean(m['rest_last_16th'] for m in ms), 3),
        'rest_last_8th_share': round(statistics.mean(m['rest_last_8th'] for m in ms), 3),
        'microtiming_ms_median': round(q([m['micro_ms'] for m in ms], 0.5), 2),
        'fastest_ioi_ticks_median': q([m['fastest_ioi_ticks'] for m in ms], 0.5),
        'longest_even_run_median': q([m['longest_run'] for m in ms], 0.5),
        'dynamics_beat4_over_beat1_median': round(q([m['dyn_last_first'] for m in ms], 0.5), 2),
        'first_dense_beat_hist': dict(collections.Counter(m['first_dense_beat'] + 1 for m in ms)),
    }
    summary[g] = s
with open(os.path.join(outdir, 'summary.json'), 'w', encoding='utf-8') as f:
    json.dump(summary, f, indent=1)
for g, s in summary.items():
    print('\n==', g)
    for k, v in s.items():
        print('  %-36s %s' % (k, v))
