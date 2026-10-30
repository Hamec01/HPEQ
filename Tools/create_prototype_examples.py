"""Prepare reproducible dry/wet/insert listening files; never claim an audition.

Owner material is used as relative-transposition texture, not as ground-truth
absolute-note material. Only the generated C5 test has a declared known root.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import subprocess
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly


def raw(path, channels=2):
    return np.fromfile(path, dtype='<f4').reshape(channels, -1).T


def save_set(out, stem, dry, wet):
    insert = dry.astype(float) + .7 * wet
    true_peak = max(float(np.max(np.abs(resample_poly(x.astype(float), 8, 1)))) for x in (dry, wet, insert))
    scale = min(1., 10**(-1 / 20) / max(true_peak, 1e-30))
    for label, audio in [('Dry', dry), ('Wet', wet), ('Insert', insert)]:
        wavfile.write(out / f'{stem}_{label}.wav', 48000, (audio * scale).astype(np.float32))
    return dict(scale=scale, maximum_true_peak_dbtp=20 * math.log10(max(true_peak * scale, 1e-30)),
                wet_dry_energy_db=10 * math.log10(max(float(np.sum(wet.astype(float)**2)), 1e-30) / max(float(np.sum(dry.astype(float)**2)), 1e-30)))


def main(executable, directory, material_directory):
    directory.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(executable.resolve()), 'notes', str(directory.resolve())], check=True)
    source = raw(directory / 'C5_source.f32')
    wet = raw(directory / 'C4_G4_C5_C3_wet.f32')
    metadata = dict(note_example=dict(source_note='C5', source_midi=72,
                    targets=['C4', 'G4', 'C5', 'C3'], midi=[60, 67, 72, 48],
                    requested_times_ms=[500, 1500, 2500, 3500],
                    **save_set(directory, 'Notes_C5_to_C4_G4_C5_C3', source, wet)), owner_material=[])
    if material_directory:
        for name in ['snare', 'keys', 'synth', 'vocal_lead1']:
            candidates = sorted(material_directory.glob(name + '.wav'))
            if not candidates:
                metadata['owner_material'].append(dict(name=name, status='missing source'))
                continue
            path = candidates[0]
            rate, x = wavfile.read(path)
            if np.issubdtype(x.dtype, np.integer):
                x = x.astype(float) / (max(abs(np.iinfo(x.dtype).min), np.iinfo(x.dtype).max))
            else:
                x = x.astype(float)
            if x.ndim == 1:
                x = np.column_stack([x, x])
            x = x[:, :2]
            if rate != 48000:
                factor = math.gcd(rate, 48000)
                x = resample_poly(x, 48000 // factor, rate // factor)
            length = min(len(x), 4 * 48000)
            # A fixed 1-second scan chooses the most energetic four-second excerpt.
            starts = range(0, max(1, len(x) - length + 1), 48000)
            start = max(starts, key=lambda i: float(np.sum(x[i:i + length]**2)))
            x = x[start:start + length].copy()
            rms = float(np.sqrt(np.mean(x**2)))
            x *= min(10**(-18 / 20) / max(rms, 1e-30), 10**(-6 / 20) / max(float(np.max(np.abs(x))), 1e-30))
            x = np.pad(x, ((0, 2 * 48000), (0, 0))).astype(np.float32)
            source_file = directory / f'{name}_source.f32'
            x.T.copy().tofile(source_file)
            for reverse in (0, 1):
                stem = name + ('_Reverse' if reverse else '_EightNotes')
                wet_file = directory / (stem + '.f32')
                subprocess.run([str(executable.resolve()), 'material', str(source_file.resolve()), str(wet_file.resolve()), '2', str(reverse)], check=True)
                output = raw(wet_file)
                metadata['owner_material'].append(dict(name=stem, source=str(path), source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                    excerpt_start_sample=start, source_note='unspecified: relative interval texture only', **save_set(directory, stem, x, output)))
    (directory / 'examples.json').write_text(json.dumps(metadata, indent=2), encoding='utf-8')
    print(json.dumps(metadata, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('executable', type=Path)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--material', type=Path)
    args = parser.parse_args()
    main(args.executable, args.directory, args.material)
