"""Owner-material prototype A/Bs. Frozen group gain; strict DSP/input/tool caches."""
import argparse
import hashlib
import json
import math
import subprocess
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'out/s2/demos'
REPORT = ROOT / 'docs/reports/s2/demos.json'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def peak8(x):
    return float(np.max(np.abs(resample_poly(x.astype(float), 8, 1, axis=0))))


def db(value):
    return 20 * math.log10(max(value, 1e-30))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--material-dir', type=Path, default=Path('C:/Users/Ham_h/AppData/Local/Temp/claude/C--DRUMENGINE/ec2c5161-f2c8-4110-8e0e-02c63cfee6b7/scratchpad/mat/wav'))
    args = parser.parse_args()
    final = json.loads((ROOT / 'docs/reports/s2/final_v4.json').read_text())
    assert final['implemented'] and final['failed'] == 0
    source = final['source_id']
    tool_id = hashlib.sha256((sha(Path(__file__)) + sha(ROOT / 'Tests/SpatialDemoMain.cpp') + sha(ROOT / 'build/Release/HamloShirota_SpatialDemo.exe')).encode()).hexdigest()
    groups = []
    for name in ['vocal_lead1', 'keys', 'drumbus1', 'synth']:
        original = args.material_dir / (name + '.wav')
        rate, samples = wavfile.read(original)
        if np.issubdtype(samples.dtype, np.integer):
            samples = samples.astype(float) / (np.iinfo(samples.dtype).max + 1)
        else:
            samples = samples.astype(float)
        if samples.ndim == 1:
            samples = np.column_stack([samples, samples])
        samples = samples[:, :2]
        if rate != 48000:
            g = math.gcd(rate, 48000)
            samples = resample_poly(samples, 48000 // g, rate // g, axis=0)
        length = min(len(samples), 5 * 48000)
        starts = range(0, max(1, len(samples) - length + 1), 12000)
        start = max(starts, key=lambda i: float(np.sum(samples[i:i + length] ** 2)))
        samples = samples[start:start + length]
        rms = float(np.sqrt(np.mean(samples * samples)))
        gain = min(10 ** (-18 / 20) / max(rms, 1e-20), 10 ** (-6 / 20) / max(float(np.max(np.abs(samples))), 1e-20))
        samples = np.pad(samples * gain, ((0, int(2.5 * 48000)), (0, 0))).astype('<f4')
        input_hash = hashlib.sha256(samples.tobytes()).hexdigest()
        key = dict(source_id=source, input_sha256=input_hash, original_sha256=sha(original), tool_id=tool_id)
        identity = hashlib.sha256(json.dumps(key, sort_keys=True).encode()).hexdigest()
        folder = WORK / name / identity[:20]
        folder.mkdir(parents=True, exist_ok=True)
        record = folder / 'identity.json'
        if record.exists():
            cached = json.loads(record.read_text())
            assert cached['identity'] == key
            for f, expected in cached['files'].items():
                assert sha(folder / f) == expected, 'Modified cached demonstration'
            groups.append(cached)
            print(name, 'resumed verified cache')
            continue
        raw_input = folder / 'input.f32'
        raw_input.write_bytes(samples.tobytes())
        subprocess.run([str(ROOT / 'build/Release/HamloShirota_SpatialDemo.exe'), str(raw_input), str(folder)], cwd=ROOT, check=True)
        rendered = json.loads((folder / 'demo_manifest.json').read_text())
        assert rendered['source_id'] == source, 'Demo executable is stale'
        audio = {scene: np.fromfile(folder / (scene + '.f32'), dtype='<f4').reshape(-1, 2) for scene in rendered['scenes']}
        assert all(np.isfinite(x).all() for x in audio.values())
        raw_peaks = {scene: peak8(x) for scene, x in audio.items()}
        common = min(1., 10 ** (-1.01 / 20) / max(raw_peaks.values()))
        rows = []
        for scene, x in audio.items():
            prepared = (x * common).astype(np.float32)
            peak = peak8(prepared)
            assert db(peak) <= -1, 'Prepared demo exceeds -1 dBTP estimate'
            file = folder / (scene + '.wav')
            wavfile.write(file, 48000, prepared)
            rows.append(dict(scene=scene, path=str(file), raw_peak_dbtp=db(raw_peaks[scene]), prepared_peak_dbtp=db(peak)))
        cached = dict(material=name, identity=key, source=str(original), selected_start_48k=start, source_frames=length,
                      preparation_gain_db=db(gain), common_demo_gain_db=db(common), tail_seconds=2.5,
                      measured_headroom='8x polyphase reconstruction estimate', recipes=rows,
                      files={p.name: sha(p) for p in folder.iterdir() if p.is_file()})
        record.write_text(json.dumps(cached, indent=2) + '\n', encoding='utf-8')
        groups.append(cached)
        print(name, len(rows), 'A/Bs verified; common gain', round(db(common), 3), 'dB')
    REPORT.write_text(json.dumps(dict(source_id=source, groups=groups), indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
