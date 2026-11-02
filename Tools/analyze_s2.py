"""S2 numerical method v1. Source-pinned offline prototype, not a preset calibration."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
METHOD_FILES = ['Tests/SpatialLabMain.cpp', 'Tools/analyze_s2.py', 'docs/S2_METHODS.md']
DSP_FILES = ['Source/DSP/SpatialPrototype.h', 'Source/DSP/SpatialPrototype.cpp']


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def db(energy):
    return 10 * math.log10(max(float(energy), 1e-30))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('run', type=Path)
    ap.add_argument('report', type=Path)
    ap.add_argument('--baseline', action='store_true', help='Explicitly analyze the unimplemented aligned-dry reference')
    args = ap.parse_args()
    manifest = json.loads((args.run / 'manifest.json').read_text())
    rows = {r['id']: r for r in manifest['cases']}
    implemented = manifest['implemented']
    if implemented == args.baseline:
        raise RuntimeError('Wrong experiment role: use --baseline only for the unimplemented reference')
    checks = []
    data = {}
    source = {name: sha(ROOT / name) for name in METHOD_FILES + DSP_FILES}
    compiled_identity = hashlib.sha256(('ShirotaSpatialS2v1' + ''.join(source[name] for name in sorted(source))).encode()).hexdigest()
    if manifest.get('source_id') != compiled_identity or manifest.get('dsp_identity') != compiled_identity:
        raise RuntimeError('Compiled DSP/method identity differs from current sources')
    identity = args.run / 'identity.json'
    # Bind once and reject results from changed DSP/method/input on later reads.
    binding = dict(files=source, manifest=sha(args.run / 'manifest.json'),
                   renders={p.name: sha(p) for p in sorted(args.run.glob('*.f32'))})
    if identity.exists():
        if json.loads(identity.read_text()) != binding:
            raise RuntimeError('Stale run: source, method, manifest or raw render changed')
    else:
        identity.write_text(json.dumps(binding, indent=2) + '\n')

    def add(goal, name, passed, **values):
        checks.append(dict(goal=goal, name=name, passed=bool(passed), **values))

    def arrays(name):
        if name not in data:
            r = rows[name]
            y = np.fromfile(args.run / (name + '.f32'), dtype='<f4').reshape(-1, 2)
            x = np.fromfile(args.run / (name + '.input.f32'), dtype='<f4').reshape(-1, 2)
            d = r['latency']
            aligned = np.pad(x, ((d, 0), (0, 0)))[:len(x)]
            data[name] = (x, y, aligned)
        return data[name]

    def ms(name, begin=.2):
        r = rows[name]
        _, y, x = arrays(name)
        y, x = y[int(begin * r['rate']):].astype(float), x[int(begin * r['rate']):].astype(float)
        return (y[:, 0] + y[:, 1]) * .5, (y[:, 0] - y[:, 1]) * .5, (x[:, 0] + x[:, 1]) * .5, (x[:, 0] - x[:, 1]) * .5

    for name in rows:
        _, y, aligned = arrays(name)
        if name.startswith(('neutral_', 'disabled_')):
            add('P01', name, y.tobytes() == aligned.tobytes())
        add('P11', name + '_allocation', rows[name]['allocations'] == 0)
        add('P10', name + '_finite', rows[name]['finite'] and np.isfinite(y).all(), peak=rows[name]['peak'])
    reference = arrays('events_1')[1].tobytes()
    for block in [64, 127, 512, 4096]:
        add('P11', 'event_blocks_' + str(block), arrays('events_' + str(block))[1].tobytes() == reference)

    if implemented:
        for width, expected in [(2, 6.020599913), (4, 12.041199827)]:
            m, s, rm, rs = ms('width_' + str(width))
            gain = db(np.mean(s * s) / np.mean(m * m)) - db(np.mean(rs * rs) / np.mean(rm * rm))
            add('P02', 'width_' + str(width), abs(gain - expected) <= .02, gain_db=gain, expected_db=expected)
        _, mono, _ = arrays('width_0')
        add('P03', 'width_zero_channels', np.array_equal(mono[:, 0], mono[:, 1]))
        sides = []
        for name in ['width_0', 'width_2', 'width_4'] + ['generate_' + str(i) for i in range(3)]:
            m, s, rm, _ = ms(name)
            error = db(np.mean((m - rm) ** 2))
            add('P03', name + '_fold', error <= -130, error_dbfs=error)
            if name.startswith('generate'):
                ratio = db(np.mean(s * s) / np.mean(m * m))
                add('P04', name + '_strength', -14 <= ratio <= 2, side_mid_db=ratio)
                sides.append(s)
        for i in range(3):
            for j in range(i):
                corr = float(np.corrcoef(sides[i], sides[j])[0, 1])
                add('P04', f'generator_{j}_{i}', abs(corr) < .95, side_correlation=corr)
        # First 139-check run missed combined generator/width/bass/band routing.
        # Extend coverage before the fix, keeping every original numeric limit.
        for i in range(3):
            _, y, _ = arrays('generate_zero_' + str(i))
            add('P03', f'generator_{i}_width_zero', np.array_equal(y[:, 0], y[:, 1]))
            _, doubled, _, _ = ms('generate_double_' + str(i))
            gain = db(np.mean(doubled ** 2) / np.mean(sides[i] ** 2))
            add('P02', f'generator_{i}_width_double', abs(gain - 6.020599913) <= .02, gain_db=gain)
            _, ref, _, _ = ms('generate_ref60_' + str(i))
            for suffix, goal, limit in [('bass60', 'P05', -30), ('band60', 'P06', -12)]:
                _, s, _, _ = ms(f'generate_{suffix}_{i}')
                gain = db(np.mean(s ** 2) / np.mean(ref ** 2))
                add(goal, f'generator_{i}_{suffix}', gain <= limit, gain_db=gain)
        for tone, limit in [('anti60', -30), ('anti1000', -.5)]:
            _, s, _, _ = ms('bass_' + tone)
            _, ref, _, _ = ms('ref_' + tone)
            gain = db(np.mean(s * s) / np.mean(ref * ref))
            add('P05', tone, gain <= limit if tone == 'anti60' else abs(gain) < .5, gain_db=gain)
        for tone in ['anti60', 'anti8000']:
            _, s, _, _ = ms('band_' + tone)
            _, ref, _, _ = ms('ref_' + tone)
            gain = db(np.mean(s * s) / np.mean(ref * ref))
            add('P06', tone, gain <= -12 if tone == 'anti60' else abs(gain) < .5, gain_db=gain)
        add('P07', 'off_intervention', abs(rows['width_4']['guard_db']) < .00001, guard_db=rows['width_4']['guard_db'])
        for floor in [0, .4]:
            name = 'guard_' + str(round(floor * 10))
            m, s, rm, rs = ms(name, .5)
            ratio = db(np.mean(s * s) / np.mean(m * m))
            target = db((1 - floor) / (1 + floor))
            off_m, off_s, _, _ = ms('width_4', .5)
            reduction = db(np.mean(s * s) / np.mean(off_s * off_s))
            add('P07', name + '_budget', abs(ratio - target) <= .5, side_mid_db=ratio, target_db=target)
            add('P07', name + '_meter', abs(rows[name]['guard_db'] - reduction) <= .5, actual_db=reduction, meter_db=rows[name]['guard_db'])
        for rate in [44100, 48000, 96000]:
            for pre in [0, 23]:
                name = f'arrival_{rate}_{pre}'
                _, y, _ = arrays(name)
                direct = int(rate * .25) + rows[name]['latency']
                indices = np.flatnonzero(np.max(np.abs(y[direct + 1:]), axis=1) > 1e-8)
                first = direct + 1 + int(indices[0]) if len(indices) else -1
                expected = direct + math.floor(rate * (6.7 + pre) / 1000)
                add('P08', name, abs(first - expected) <= 1, first=first, expected=expected)
        tails, centroids = [], []
        for c in range(3):
            name = 'space_impulse_' + str(c)
            _, y, _ = arrays(name)
            direct = int(.25 * rows[name]['rate']) + rows[name]['latency']
            tail = y[direct + 1:].astype(float)
            energy = np.sum(tail * tail, axis=1)
            centroid = float(np.sum(energy * (np.arange(len(energy)) + 1) / 48) / np.sum(energy))
            tails.append(tail.ravel())
            centroids.append(centroid)
            m, _, rm, _ = ms('space_noise_' + str(c))
            gain = db(np.mean(m * m) / np.mean(rm * rm))
            add('P10', 'space_mono_' + str(c), abs(gain) <= 6, gain_db=gain)
            _, decay, _ = arrays('tail_' + str(c))
            late = db(np.sum(decay[3 * 48000:].astype(float) ** 2) / max(np.sum(decay.astype(float) ** 2), 1e-30))
            add('P10', 'tail_decay_' + str(c), late <= -100, late_relative_db=late)
        add('P09', 'centroid_order', centroids[1] >= centroids[0] + 10 and centroids[2] >= centroids[1] + 10, centroid_ms=centroids)
        for i in range(3):
            for j in range(i):
                corr = float(np.dot(tails[i], tails[j]) / np.sqrt(np.dot(tails[i], tails[i]) * np.dot(tails[j], tails[j])))
                add('P09', f'space_{j}_{i}', abs(corr) < .9, tail_correlation=corr)
    else:
        for goal in ['P02', 'P03', 'P04', 'P05', 'P06', 'P07', 'P08', 'P09', 'P10_features']:
            checks.append(dict(goal=goal, name='not_implemented', passed=None, reason='n/a: aligned-dry baseline'))
    bench = json.loads((args.run / 'benchmark.json').read_text())
    for i, row in enumerate(bench):
        add('P12', 'cpu_' + str(i), row['mean_fraction'] <= .25, **row)
    report = dict(method='Shirota S2 numerical method v1', implemented=implemented, source_id=compiled_identity,
                  identity_sha256=sha(identity), files=source, cases=len(rows), checks=checks,
                  passed=sum(c['passed'] is True for c in checks), failed=sum(c['passed'] is False for c in checks),
                  unavailable=sum(c['passed'] is None for c in checks))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f"S2: {report['passed']} passed, {report['failed']} failed, {report['unavailable']} n/a")
    for c in checks:
        if c['passed'] is False:
            print(json.dumps(c))
    return 1 if report['failed'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
