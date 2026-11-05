"""R13 paired analysis. Never writes over an existing analysis or private demo directory."""
import hashlib
import itertools
import json
import pathlib
import sys
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def tp(x):
    return 20 * np.log10(max(1e-20, np.max(np.abs(resample_poly(x, 8, 1, axis=0, window=('kaiser', 10))))))


def distance(rows):
    env = [np.array(r['rms128'], dtype=float) for r in rows]
    values = [1 - float(np.dot(a, b) / max(1e-30, np.linalg.norm(a) * np.linalg.norm(b))) for a, b in itertools.combinations(env, 2)]
    return {'mean_cosine_distance': float(np.mean(values)), 'minimum_cosine_distance': float(min(values))}


def main(before, after, output, demos):
    before, after, output, demos = map(pathlib.Path, (before, after, output, demos))
    if output.exists() or demos.exists():
        raise RuntimeError('Refusing to overwrite evidence')
    b = json.loads((before / 'results.json').read_text())
    a = json.loads((after / 'results.json').read_text())
    old = {r['key']: r for r in b['rows']}
    exact = [r['key'] for r in a['rows'] if r['key'] in old and r['pcm_sha256'] == old[r['key']]['pcm_sha256']]
    assert len(exact) == len(b['rows']) == 288
    assert all(r['finite'] for r in a['rows'])
    structures = [r for r in a['structures'] if r['recipe'] >= 8]
    assert len(structures) == 78
    for r in structures:
        assert r['plans'] >= (48 if r['recipe'] == 8 else 16), r
        assert r['rhythm_masks32'] >= (24 if r['recipe'] == 8 else 16), r
    new_rows = [r for r in a['rows'] if r['recipe'] == 8]
    assert len(new_rows) == 96
    assert all(r['prefix_error'] == 0 for r in new_rows if r['bars'] == 2)
    audio = []
    for p in sorted(after.glob('*_1_8_1_*.wav')):
        fs, x = wavfile.read(p)
        peak = tp(x.astype(float))
        assert peak <= -1, (p, peak)
        audio.append({'file': p.name, 'sha256': sha(p), 'true_peak_db': peak})
    assert len(audio) == 48
    diversity = []
    demos.mkdir(parents=True)
    demo_rows = []
    for name in sorted({r['source'] for r in new_rows}):
        old_rows = [r for r in b['rows'] if not r['compatibility'] and r['source'] == name and r['bars'] == 1]
        fresh_rows = [r for r in new_rows if r['source'] == name and r['bars'] == 1]
        d = {'source': name, 'old': distance(old_rows), 'new': distance(fresh_rows)}
        d['ratio'] = d['new']['mean_cosine_distance'] / d['old']['mean_cosine_distance']
        diversity.append(d)
        for seed in (1, 2, 3, 4):
            op = before / f'{name}_1_1_1_{seed}.wav'
            npth = after / f'{name}_1_8_1_{seed}.wav'
            fs, x = wavfile.read(op)
            fs2, y = wavfile.read(npth)
            assert fs == fs2 and x.shape == y.shape
            x, y = x.astype(float), y.astype(float)
            rmsx, rmsy = np.sqrt(np.mean(x*x)), np.sqrt(np.mean(y*y))
            x *= 10**(-18/20) / max(1e-20, rmsx)
            y *= 10**(-18/20) / max(1e-20, rmsy)
            peak = max(tp(x), tp(y))
            common = min(1., 10**((-1.2-peak)/20))
            z = np.concatenate([x*common, np.zeros((round(fs*.3), 2)), y*common]).astype(np.float32)
            file = demos / f'{name}_seed{seed}_R12_then_R13.wav'
            wavfile.write(file, fs, z)
            finalpeak = tp(z.astype(float))
            assert finalpeak <= -1
            demo_rows.append({'file': file.name, 'seed': seed, 'source': name, 'true_peak_db': finalpeak,
                              'sha256': sha(file), 'rms_difference_db': float(20*np.log10(rmsx/rmsy)),
                              'matched_rms_db': -18+20*np.log10(common), 'old_sha256': sha(op), 'new_sha256': sha(npth)})
    report = {'method': '128-cell ending RMS; pairwise 1-cosine; 8 seeds/source; gain invariant. Independent scipy 8x Kaiser10 true peak.',
              'before_sha256': sha(before/'results.json'), 'after_sha256': sha(after/'results.json'),
              'old_exact': len(exact), 'new_renders': len(new_rows), 'structures': structures,
              'diversity': diversity, 'audio': audio, 'demos': demo_rows,
              'render_ms_new': {'mean': float(np.mean([r['render_ms'] for r in new_rows])), 'max': max(r['render_ms'] for r in new_rows)}}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2)+'\n')
    (demos/'comparisons.json').write_text(json.dumps(demo_rows, indent=2)+'\n')
    print(json.dumps({'exact': len(exact), 'diversity': diversity, 'worst_true_peak_db': max(r['true_peak_db'] for r in audio), 'demos': len(demo_rows)}, indent=2))


if __name__ == '__main__':
    main(*sys.argv[1:])
