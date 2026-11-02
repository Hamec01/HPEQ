"""Independent finite-clip Kaiser resampling reference, not a translation of DSP."""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import math
from functools import lru_cache
import numpy as np


def source(kind, rate, gap=0):
    if kind == 'attack_pair':
        x = np.zeros(gap + 1, dtype=np.float32)
        x[0], x[-1] = .5, -.35
        return x
    if kind == 'attack_impulse':
        return np.array([.5], dtype=np.float32)
    length = round(rate * (.002 if kind == 'attack_pulse' else .006))
    age = np.arange(length)
    if kind == 'attack_pulse':
        return (.4 * np.sin(2 * np.pi * 800 * age / rate) * (.5 - .5 * np.cos(2 * np.pi * age / length))).astype(np.float32)
    value = (age.astype(np.uint64) * 747796405 + 2891336453) & 0xffffffff
    value = (((value >> ((value >> 28) + 4)) ^ value) * 277803737) & 0xffffffff
    value = (value >> 22) ^ value
    return (((value >> 8) / 8388608. - 1.) * .4 * np.exp(-age / (.0015 * rate))).astype(np.float32)


@lru_cache(maxsize=None)
def reference(kind, rate, shift, length, gap=0, origin=0):
    if kind == 'attack_pair':
        first = reference('attack_impulse', rate, shift, length, 0, origin)
        second = np.zeros(length)
        second[gap:] = first[:-gap]
        return first - .7 * second
    x = source(kind, rate)
    if shift == 0:
        out = np.zeros(length)
        out[origin:origin + len(x)] = x
        return out
    ratio = 2**(shift / 12)
    # Align the input's observable attack peak, not an unobservable file boundary
    # that may consist of a leading exact zero. Resample around that physical anchor.
    anchor = int(np.argmax(np.abs(x)))
    all_positions = anchor + (np.arange(length) - origin - anchor) * ratio
    valid = (all_positions >= -48) & (all_positions <= len(x) - 1 + 48)
    positions = all_positions[valid]
    whole = np.floor(positions).astype(np.int64)
    offsets = np.arange(-47, 49)
    indices = whole[:, None] + offsets
    distance = indices - positions[:, None]
    cutoff = .45 / max(1, ratio)
    support = np.abs(distance) / 48
    window = np.i0(9 * np.sqrt(np.maximum(0, 1 - support**2))) / np.i0(9)
    window[support > 1] = 0
    coefficients = 2 * cutoff * np.sinc(2 * cutoff * distance) * window
    coefficients /= np.maximum(np.sum(coefficients, axis=1, keepdims=True), 1e-30)
    samples = x[np.clip(indices, 0, len(x) - 1)].astype(float)
    samples[(indices < 0) | (indices >= len(x))] = 0
    out = np.zeros(length)
    out[valid] = np.sum(samples * coefficients, axis=1)
    return out


def analyse(directory, output):
    results = []
    for row in csv.DictReader((directory / 'attacks.tsv').open(), delimiter='\t'):
        rate, shift, frames = int(float(row['rate'])), int(row['shift']), int(row['frames'])
        path = directory / (row['case'] + '.f32')
        raw = np.fromfile(path, dtype='<f4').reshape(int(row['channels']), frames)
        stereo = len(raw) == 1 or np.array_equal(raw[0], raw[1])
        actual = raw[0].astype(float)
        gap = int(row.get('gap', 0))
        origin = int(row['onset']) + int(row['delay']) - int(row['start'])
        expected = reference(row['type'], rate, shift, frames, gap, origin)
        energy = float(np.sum(expected**2))
        gain_db = 10 * math.log10(max(float(np.sum(actual**2)), 1e-30) / energy)
        peak_error = int(np.argmax(np.abs(actual))) - int(np.argmax(np.abs(expected)))
        errors = []
        for lag in (-1, 0, 1):
            shifted = np.roll(actual, lag)
            if lag > 0:
                shifted[:lag] = 0
            elif lag < 0:
                shifted[lag:] = 0
            errors.append(float(np.sqrt(np.sum((shifted - expected)**2) / energy)))
        ratio = 2**(shift / 12)
        clip = source(row['type'], rate, gap)
        anchor = int(np.argmax(np.abs(clip)))
        end = origin + (gap if row['type'] == 'attack_pair' else anchor) + math.ceil(((1 if gap else len(clip) - anchor) + 48) / ratio) + round(.005 * rate)
        ghost = 10 * math.log10(max(float(np.sum(actual[end:]**2)), 1e-30) / energy)
        finite = bool(np.isfinite(raw).all())
        passed = finite and stereo and abs(peak_error) <= 1 and abs(gain_db) <= 1 and min(errors) <= .15 and ghost <= -60 and int(row['allocations']) == 0
        results.append(dict(case=row['case'], type=row['type'], shift=shift, peak_error_samples=peak_error,
                            energy_error_db=gain_db, normalised_l2=min(errors), late_energy_dbr=ghost,
                            finite=finite, identical_stereo=bool(stereo), allocations=int(row['allocations']), passed=passed,
                            sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    summary = dict(method='ATTACK_METHODS v3: peak-anchored Kaiser96, manifest origin, musical pair times', cases=len(results), passed=sum(r['passed'] for r in results),
                   worst_peak_error_samples=max(abs(r['peak_error_samples']) for r in results),
                   worst_energy_error_db=max(abs(r['energy_error_db']) for r in results),
                   worst_normalised_l2=max(r['normalised_l2'] for r in results),
                   worst_late_energy_dbr=max(r['late_energy_dbr'] for r in results),
                   finite_all=all(r['finite'] for r in results), allocations=sum(r['allocations'] for r in results))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(dict(summary=summary, results=results), indent=2), encoding='utf-8')
    print(json.dumps(summary, indent=2))
    return 0 if summary['passed'] == summary['cases'] else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    raise SystemExit(analyse(args.directory, args.output))
