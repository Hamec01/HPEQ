"""Independent S3 path oracle, convolution and routing analysis; immutable runs."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from scipy.signal import fftconvolve, correlate

ROOT = Path(__file__).resolve().parents[1]
FILES = ['Assets/Hrtf/horizontal.bin', 'Assets/Hrtf/manifest.json', 'Source/DSP/MotionPrototype.cpp',
         'Source/DSP/MotionPrototype.h', 'Tests/MotionLabMain.cpp', 'Tools/analyze_s3.py', 'docs/S3_METHODS.md']


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def db(x):
    return 10 * np.log10(max(float(x), 1e-30))


def bank():
    data = (ROOT / 'Assets/Hrtf/horizontal.bin').read_bytes()
    magic, rates, profiles, angles = struct.unpack_from('<8sIII', data)
    assert magic == b'HmHrS3A\0' and (rates, profiles, angles) == (3, 3, 72)
    offset, result = 20, {}
    for _ in range(rates):
        rate, taps = struct.unpack_from('<II', data, offset)
        offset += 8
        count = profiles * angles * 2 * taps
        result[rate] = np.frombuffer(data, dtype='<f4', count=count, offset=offset).reshape(profiles, angles, 2, taps).astype(float)
        offset += count * 4
    assert offset == len(data)
    return result


def oracle(row):
    """Analytic cumulative phase, not a simulation of the C++ state machine."""
    n, fs, scenario = row['frames'], row['rate'], row['scenario']
    frame = np.arange(n, dtype=float)
    if scenario == 'reset':
        frame[10001:] -= 10001
    if scenario in ('beats', 'tempo'):
        t = frame / fs * row['bpm'] / 60
        if scenario == 'tempo':
            t[36001:] = 36001 / fs * 90 / 60 + (frame[36001:] - 36001) / fs * 174 / 60
        lengths = np.array([.25, .75, .5, .5, 0, 1])
    else:
        t = frame / fs * 1000
        lengths = np.array([125, 375, 250, 250, 0, 500], dtype=float)
    if scenario == 'zero':
        return np.column_stack([np.full(n, 90), np.zeros(n), np.zeros(n), np.zeros(n)])
    if scenario in ('cw', 'ccw'):
        phase = np.mod(t, 1000) / 1000
        return np.column_stack([np.mod(phase * (-360 if scenario == 'cw' else 360), 360), np.zeros(n), np.ones(n), phase])
    cycle = lengths.sum()
    if scenario == 'nonloop':
        t = np.minimum(t, lengths[:4].sum())
    else:
        t = np.mod(t, cycle)
    boundaries = np.cumsum(lengths)
    segment = np.searchsorted(boundaries, t + 1e-10, side='right')
    segment = np.minimum(segment, 5)
    start = np.concatenate([[0], boundaries[:-1]])
    q = np.maximum(0, (t - start[segment]) / np.maximum(lengths[segment], 1e-30))
    node = segment // 2
    travelling = segment % 2
    begins = np.array([90., 270., 0.])[node]
    changes = np.array([180., 90., 90.])[node]
    interpolation = np.where(node == 1, q, q * q * (3 - 2 * q))
    angle = np.mod(begins + travelling * changes * interpolation, 360)
    if scenario == 'nonloop':
        final = t >= lengths[:4].sum()
        angle[final], node[final], travelling[final], q[final] = 0, 2, 0, 0
    return np.column_stack([angle, node, travelling, q * travelling])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('run', type=Path)
    ap.add_argument('report', type=Path)
    ap.add_argument('--baseline', action='store_true')
    args = ap.parse_args()
    m = json.loads((args.run / 'manifest.json').read_text())
    sources = {name: sha(ROOT / name) for name in FILES}
    identity = hashlib.sha256(('ShirotaMotionS3v1' + ''.join(sources[n] for n in sorted(sources))).encode()).hexdigest()
    assert m['source_id'] == m['dsp_identity'] == identity, 'Stale compiled DSP/method/data identity'
    assert m['implemented'] != args.baseline, 'Wrong baseline/candidate role'
    binding = dict(files=sources, manifest=sha(args.run / 'manifest.json'),
                   renders={p.name: sha(p) for p in args.run.iterdir() if p.suffix in ('.f32', '.f64')},
                   stress=sha(args.run / 'stress.json'), benchmark=sha(args.run / 'benchmark.json'))
    pin = args.run / 'identity.json'
    if pin.exists():
        assert json.loads(pin.read_text()) == binding, 'Corrupt/stale cached run'
    else:
        pin.write_text(json.dumps(binding, indent=2) + '\n', encoding='utf-8', newline='\n')
    rows = {r['id']: r for r in m['cases']}
    checks, cached = [], {}

    def add(goal, name, passed, **values):
        checks.append(dict(goal=goal, name=name, passed=bool(passed), **values))

    def arrays(name):
        if name not in cached:
            x = np.fromfile(args.run / (name + '.input.f32'), dtype='<f4').reshape(-1, 2).astype(float)
            y = np.fromfile(args.run / (name + '.f32'), dtype='<f4').reshape(-1, 2).astype(float)
            cached[name] = x, y, np.pad(x, ((rows[name]['latency'], 0), (0, 0)))[:len(x)]
        return cached[name]

    for name, row in rows.items():
        x, y, dry = arrays(name)
        add('M03', name + '_allocations', row['allocations'] == 0)
        add('M08', name + '_finite', row['finite'] and np.isfinite(y).all(), peak=row['peak'])
        if name.startswith(('neutral_', 'disabled_')):
            add('M01', name, np.array_equal(y, dry))
    for block in [64, 127, 512, 4096]:
        add('M03', 'events_' + str(block), np.array_equal(arrays('events_1')[1], arrays('events_' + str(block))[1]))

    filters = bank()
    export = json.loads((ROOT / 'Assets/Hrtf/manifest.json').read_text())
    assert export['bank_sha256'] == sources['Assets/Hrtf/horizontal.bin']
    add('M06', 'native_FIR_fidelity', export['minimum_retained_energy'] >= .999 and export['worst_magnitude_error_db'] <= .5,
        minimum_retained_energy=export['minimum_retained_energy'], worst_magnitude_error_db=export['worst_magnitude_error_db'])

    if m['implemented']:
        for name, row in rows.items():
            if not row['trace']:
                continue
            actual = np.fromfile(args.run / (name + '.trace.f64'), dtype='<f8').reshape(-1, 4)
            expected = oracle(row)
            error = np.abs(np.mod(actual[:, 0] - expected[:, 0] + 180, 360) - 180)
            # Boundary labels may differ at most one sample due to quantization.
            boundary = np.flatnonzero(np.any(expected[1:, 1:3] != expected[:-1, 1:3], axis=1)) + 1
            mask = np.ones(len(actual), dtype=bool)
            for i in boundary:
                mask[max(0, i - 1):i + 2] = False
            label_match = np.array_equal(actual[mask, 1:3], expected[mask, 1:3])
            add('M02', name, np.max(error) <= 1e-7 and label_match, max_angular_error_degrees=float(np.max(error)), label_match=label_match)
        for angle in [0, 90, 270]:
            _, y, x = arrays('pan_' + str(angle));y, x = y[10000:], x[10000:]
            ratio = db(np.mean(np.sum(y * y, axis=1)) / np.mean(np.sum(x * x, axis=1)))
            add('M04', 'pan_power_' + str(angle), abs(ratio) <= .01, gain_db=ratio)
            if angle:
                near, far = (0, 1) if angle == 90 else (1, 0)
                separation = db(np.mean(y[:, near] ** 2) / max(np.mean(y[:, far] ** 2), 1e-30))
                add('M04', 'pan_separation_' + str(angle), separation >= 100, separation_db=separation)
        for b in [-1, 1]:
            _, y, x = arrays('balance_' + str(b));near, far = (0, 1) if b == -1 else (1, 0)
            add('M04', 'balance_' + str(b), np.array_equal(y[:, near], x[:, near]) and np.all(y[:, far] == 0))
        add('M04', 'point_antiphase_cancelled', np.all(arrays('point_anti')[1] == 0))
        for name in ['spread_anti'] + ['binaural_spread_anti_' + str(i) for i in range(3)]:
            x, y, _ = arrays(name)
            ratio = float(np.mean(y ** 2) / np.mean(x ** 2))
            add('M04', name + '_retained', ratio > .01, output_input_energy=ratio)
        # Coverage added after the first190-check run; original60jobs unchanged.
        for profile in range(3):
            for channel in range(2):
                name = f'spread_hrir_{profile}_{channel}'
                x, y, _ = arrays(name)
                angle = 30 if channel == 0 else 330
                h = filters[48000][profile, angle // 5] / np.sqrt(2)
                expected = np.column_stack([fftconvolve(x[:, channel], ear)[:len(x)] for ear in h])
                relative = db(np.sum((y - expected) ** 2) / max(np.sum(expected ** 2), 1e-30))
                add('M04', name + '_independent_emitter', relative <= -90, relative_error_db=relative)
        for name, row in rows.items():
            if not name.startswith('hrir_'):
                continue
            if name.startswith('hrir_rate_'):
                profile, angle = 0, 90
            else:
                _, p, a = name.split('_');profile, angle = int(p), int(a) / 10
            f = filters[int(row['rate'])][profile]
            index, fraction = divmod(angle / 5, 1)
            h = f[int(index) % 72] * (1 - fraction) + f[(int(index) + 1) % 72] * fraction
            x, y, _ = arrays(name);mono = np.mean(x, axis=1)
            expected = np.column_stack([fftconvolve(mono, ear)[:len(mono)] for ear in h])
            relative = db(np.sum((y - expected) ** 2) / max(np.sum(expected ** 2), 1e-30))
            add('M05', name + '_convolution', relative <= -90, relative_error_db=relative)
        for profile in range(3):
            for angle in [90, 270]:
                h = filters[48000][profile, angle // 5]
                lag = int(np.argmax(correlate(h[1], h[0], mode='full')) - (h.shape[-1] - 1))
                spec = np.fft.rfft(h, 4096);hz = np.fft.rfftfreq(4096, 1 / 48000);band = (hz >= 3000) & (hz <= 12000)
                ild = db(np.sum(np.abs(spec[0, band]) ** 2) / np.sum(np.abs(spec[1, band]) ** 2))
                sign = 1 if angle == 90 else -1
                add('M05', f'ear_order_{profile}_{angle}', sign * lag > 0 and sign * ild > 0, right_minus_left_lag=lag, left_right_highband_db=ild)
        pre, post, change = [arrays('transition_' + name)[1] for name in ['pre', 'post', 'change']]
        residual = float(np.max(np.abs(change[48000:] - post[48000:])))
        bound = float(np.max(np.abs(pre[48000:]))) + float(np.max(np.abs(post[48000:]))) + .01
        add('M07', 'profile_position_transition', residual <= bound, residual_peak=residual, bound=bound)
        _, switched, dry = arrays('transition_enable')
        ramp = np.ones(len(switched))
        ramp[48000:48960] = 1 - (np.arange(960) + 1) / 960
        ramp[48960:96000] = 0
        ramp[96000:96960] = (np.arange(960) + 1) / 960
        expected = dry + ramp[:, None] * (pre - dry)
        relative = db(np.sum((switched - expected) ** 2) / np.sum(expected ** 2))
        add('M07', 'enable_ramp_exact', relative <= -90, relative_error_db=relative)
        worst = 0.
        for profile in filters[48000]:
            for i in range(72):
                before = profile[i] * .9998 + profile[(i - 1) % 72] * .0002
                after = profile[i] * .9998 + profile[(i + 1) % 72] * .0002
                worst = max(worst, abs(db(np.sum(after ** 2) / np.sum(before ** 2))))
        add('M07', 'angular_boundary_continuity', worst <= .5, worst_boundary_step_db=worst)
    else:
        for goal in ['M02', 'M04', 'M05', 'M07']:
            checks.append(dict(goal=goal, name='not_implemented', passed=None, reason='aligned-dry baseline'))
    stress = json.loads((args.run / 'stress.json').read_text())
    stress_bound = max(float(np.max(np.sum(np.abs(f), axis=-1))) for f in filters.values()) * .15 / np.sqrt(2) + .01
    add('M08', 'five_minute_stress', stress['frames'] == 48000 * 300 and stress['finite'] and stress['allocations'] == 0 and stress['peak'] <= stress_bound, **stress, l1_bound=stress_bound)
    for i, row in enumerate(json.loads((args.run / 'benchmark.json').read_text())):
        add('M09', 'cpu_' + str(i), row['mean_fraction'] <= .25, **row)
    result = dict(method='S3v1', source_id=identity, files=sources, implemented=m['implemented'], cases=len(rows), checks=checks,
                  passed=sum(c['passed'] is True for c in checks), failed=sum(c['passed'] is False for c in checks),
                  unavailable=sum(c['passed'] is None for c in checks), identity_sha256=sha(pin))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f"S3: {result['passed']} passed, {result['failed']} failed, {result['unavailable']} n/a")
    for c in checks:
        if c['passed'] is False:
            print(json.dumps(c))
    return 1 if result['failed'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
