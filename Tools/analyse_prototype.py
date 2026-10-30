"""Independent pitch/sequence analysis. Does not import or translate the C++ DSP.

Raw input is planar float32. Preserve machine-readable rows and failures; a pitch
pass alone is never interpreted as transparent audio quality.
"""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import math
import numpy as np
from scipy import signal


def pitch_metrics(x, rate, expected):
    x = x.astype(np.float64)
    window = signal.windows.hann(len(x), sym=False)
    fft_size = 1 << (len(x) * 32 - 1).bit_length()
    spectrum = np.abs(np.fft.rfft(x * window, n=fft_size))
    lo, hi = max(1, int(20 / rate * fft_size)), int(.48 * fft_size)
    peak = lo + int(np.argmax(spectrum[lo:hi]))
    a, b, c = np.log(np.maximum(spectrum[peak - 1:peak + 2], 1e-30))
    offset = .5 * (a - c) / (a - 2 * b + c)
    measured = (peak + offset) * rate / fft_size
    cents = 1200 * math.log2(measured / expected)
    t = np.arange(len(x)) / rate
    basis = np.column_stack([np.cos(2 * np.pi * measured * t), np.sin(2 * np.pi * measured * t), np.ones(len(t))])
    # Weighted fit suppresses boundary leakage without removing modulation in the measured interval.
    fit = np.linalg.lstsq(basis * np.sqrt(window[:, None]), x * np.sqrt(window), rcond=None)[0]
    fundamental = basis[:, :2] @ fit[:2]
    residual = x - basis @ fit
    residual_db = 10 * math.log10(max(np.sum(window * residual**2), 1e-30) / max(np.sum(window * fundamental**2), 1e-30))
    frequency = np.fft.rfftfreq(fft_size, 1 / rate)
    usable = (frequency > 20) & (frequency < rate * .48) & (np.abs(frequency - measured) > 10)
    spur_db = 20 * math.log10(max(float(np.max(spectrum[usable])), 1e-30) / max(float(spectrum[peak]), 1e-30))
    return dict(expected_hz=expected, measured_hz=measured, cents=cents, residual_dbc=residual_db, strongest_spur_dbc=spur_db)


def reverse_reference(frames, channels, delay, length):
    indices = np.arange(frames, dtype=np.int64)
    x = np.stack([(((indices * 37 + ch * 19) % 251) - 125) / 512 for ch in range(channels)])
    y = np.zeros_like(x)
    w = signal.windows.hann(length, sym=False)
    for start in range(delay + length, frames, length // 2):
        count = min(length, frames - start)
        captured = x[:, start - delay - length:start - delay]
        y[:, start:start + count] += captured[:, ::-1][:, :count] * w[:count]
    return y.astype(np.float32)


def analyse(directory, output):
    rows = list(csv.DictReader((directory / 'manifest.tsv').open(), delimiter='\t'))
    results, pitch, exact, blocks, diagnostics = [], [], [], {}, []
    allocation_failures = []
    for row in rows:
        name, kind = row['case'], row['type']
        rate, ch, frames = int(float(row['rate'])), int(row['channels']), int(row['frames'])
        raw_path = directory / (name + '.f32')
        x = np.fromfile(raw_path, dtype='<f4').reshape(ch, frames)
        record = dict(case=name, type=kind, finite=bool(np.isfinite(x).all()), peak=float(np.max(np.abs(x))),
                      allocations=int(row['allocations']), sha256=hashlib.sha256(raw_path.read_bytes()).hexdigest())
        if record['allocations']:
            allocation_failures.append(name)
        if kind in ('pitch', 'alias'):
            start = int(row['delay']) + int(row['pitch_bound']) + round(.25 * rate)
            stop = start + round(.5 * rate)
            if stop > frames:
                raise ValueError(f'Analysis window exceeds render: {name}')
            shift = int(row['shift'])
            if kind == 'pitch':
                record['channels'] = []
                for channel in range(ch):
                    hz = float(row['hz'] if channel == 0 else row['right_hz'])
                    metrics = pitch_metrics(x[channel, start:stop], rate, hz * 2**(shift / 12))
                    metrics['pass'] = abs(metrics['cents']) <= 5
                    record['channels'].append(metrics)
                record['pass'] = all(item['pass'] for item in record['channels'])
                if ch == 2 and row['hz'] == row['right_hz']:
                    record['identical_stereo'] = bool(np.array_equal(x[0], x[1]))
                    record['pass'] &= record['identical_stereo']
                pitch.append(record)
            else:
                record['out_of_band_output_dbr'] = 20 * math.log10(max(float(np.sqrt(np.mean(x[:, start:stop].astype(float)**2))), 1e-30) / (.25 / math.sqrt(2)))
                diagnostics.append(record)
        elif kind == 'timing':
            expected = np.zeros_like(x)
            expected[:, int(row['delay'])] = .5
            record['pass'] = bool(np.array_equal(x, expected))
            nonzero = np.flatnonzero(x[0])
            record['first_nonzero'] = int(nonzero[0]) if len(nonzero) else None
            record['requested_sample'] = int(row['delay'])
            exact.append(record)
        elif kind == 'reverse':
            reference = reverse_reference(frames, ch, int(row['delay']), int(row['reverse_length']))
            record['max_absolute_error'] = float(np.max(np.abs(x - reference)))
            record['pass'] = record['max_absolute_error'] <= 1e-6
            nonzero = np.flatnonzero(x[0])
            record['first_nonzero'] = int(nonzero[0]) if len(nonzero) else None
            record['capture_samples'] = int(row['reverse_length'])
            exact.append(record)
        elif kind == 'determinism':
            group = name.rsplit('_', 1)[0]
            if group not in blocks:
                blocks[group] = x.copy()
            record['pass'] = bool(np.array_equal(x, blocks[group]))
            exact.append(record)
        elif kind == 'transient':
            indices = np.flatnonzero(np.abs(x[0]) > 1e-7)
            record['first_nonzero'] = int(indices[0]) if len(indices) else None
            record['last_above_minus140_dbfs'] = int(indices[-1]) if len(indices) else None
            record['energy_relative_input_db'] = 10 * math.log10(max(float(np.sum(x.astype(float)**2)), 1e-30) / .25)
            diagnostics.append(record)
        results.append(record)
    all_pitch_channels = [channel for item in pitch for channel in item['channels']]
    summary = dict(
        method='PROTOTYPE_METHODS.md v1; global Hann FFT peak, independent sine fit and offline reverse OLA',
        cases=len(results), pitch_cases=len(pitch), pitch_pass=sum(item['pass'] for item in pitch),
        pitch_channels=len(all_pitch_channels), worst_pitch_cents=max((abs(item['cents']) for item in all_pitch_channels), default=None),
        worst_residual_dbc=max((item['residual_dbc'] for item in all_pitch_channels), default=None),
        worst_spur_dbc=max((item['strongest_spur_dbc'] for item in all_pitch_channels), default=None),
        exact_cases=len(exact), exact_pass=sum(item['pass'] for item in exact),
        finite_all=all(item['finite'] for item in results), allocation_failures=allocation_failures,
        failures=[item['case'] for item in results if not item.get('pass', True) or not item['finite']],
        diagnostics=diagnostics,
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(dict(summary=summary, results=results), indent=2), encoding='utf-8')
    print(json.dumps(summary, indent=2))
    return 1 if summary['failures'] or allocation_failures else 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    raise SystemExit(analyse(args.directory, args.output))
