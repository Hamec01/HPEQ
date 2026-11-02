"""Source/material/tool-pinned binaural audition groups, with resumable caches."""
import argparse
import hashlib
import json
import math
import shutil
import subprocess
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'out/s3/demos'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def peak8(x):
    return float(np.max(np.abs(resample_poly(x.astype(float), 8, 1, axis=0))))


def db(x):
    return 20 * math.log10(max(x, 1e-30))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--material-dir', type=Path, default=Path('C:/Users/Ham_h/AppData/Local/Temp/claude/C--DRUMENGINE/ec2c5161-f2c8-4110-8e0e-02c63cfee6b7/scratchpad/mat/wav'))
    args = parser.parse_args()
    final = json.loads((ROOT / 'docs/reports/s3/final.json').read_text())
    assert final['implemented'] and final['failed'] == 0
    for name, value in final['files'].items():
        assert sha(ROOT / name) == value, 'Changed DSP/method/data since acceptance'
    exe = ROOT / 'build/Release/HamloShirota_MotionLab.exe'
    tool = hashlib.sha256((sha(Path(__file__)) + sha(exe)).encode()).hexdigest()
    groups = []
    for name in ['vocal_lead1', 'keys', 'drumbus1', 'synth']:
        original = args.material_dir / (name + '.wav')
        rate, x = wavfile.read(original)
        x = x.astype(float) / (np.iinfo(x.dtype).max + 1) if np.issubdtype(x.dtype, np.integer) else x.astype(float)
        if x.ndim == 1:
            x = np.column_stack([x, x])
        x = x[:, :2]
        if rate != 48000:
            g = math.gcd(rate, 48000);x = resample_poly(x, 48000 // g, rate // g, axis=0)
        length = min(len(x), 48000 * 5)
        start = max(range(0, max(1, len(x) - length + 1), 12000), key=lambda i: float(np.sum(x[i:i + length] ** 2)))
        x = x[start:start + length]
        gain = min(10 ** (-18 / 20) / max(float(np.sqrt(np.mean(x ** 2))), 1e-20), 10 ** (-6 / 20) / max(float(np.max(np.abs(x))), 1e-20))
        x = np.pad(x * gain, ((0, 12000), (0, 0))).astype('<f4')
        key = dict(source_id=final['source_id'], original_sha256=sha(original), input_sha256=hashlib.sha256(x.tobytes()).hexdigest(), tool_id=tool)
        identifier = hashlib.sha256(json.dumps(key, sort_keys=True).encode()).hexdigest()
        folder = WORK / name / identifier[:20];folder.mkdir(parents=True, exist_ok=True)
        record = folder / 'identity.json'
        if record.exists():
            group = json.loads(record.read_text())
            assert group['identity'] == key
            for file, value in group['files'].items():
                assert sha(folder / file) == value, 'Corrupt demonstration cache'
            groups.append(group);print(name, 'resumed verified cache');continue
        source = folder / 'input.f32';source.write_bytes(x.tobytes())
        # Preserve any interrupted native attempt; reuse only completed pinned groups.
        attempt = 0
        while (folder / ('render_' + str(attempt))).exists():
            attempt += 1
        render = folder / ('render_' + str(attempt))
        subprocess.run([str(exe), str(ROOT / 'Assets/Hrtf/horizontal.bin'), str(render), '--demo', str(source)], cwd=ROOT, check=True)
        manifest = json.loads((render / 'demo_manifest.json').read_text())
        assert manifest['source_id'] == final['source_id']
        audio = {s: np.fromfile(render / (s + '.f32'), dtype='<f4').reshape(-1, 2) for s in manifest['scenes']}
        assert len(audio) == 17 and all(np.isfinite(v).all() for v in audio.values())
        raw_peaks = {s: peak8(v) for s, v in audio.items()}
        common = min(1., 10 ** (-1.01 / 20) / max(raw_peaks.values()))
        rows = []
        for scene, data in audio.items():
            prepared = (data * common).astype('<f4');peak = db(peak8(prepared));assert peak <= -1
            path = render / (scene + '.wav');wavfile.write(path, 48000, prepared)
            rows.append(dict(scene=scene, path=str(path), raw_peak_dbtp=db(raw_peaks[scene]), prepared_peak_dbtp=peak))
        group = dict(material=name, source=str(original), identity=key, selected_start_48k=start, source_frames=length,
                     preparation_gain_db=db(gain), common_demo_gain_db=db(common), tail_seconds=.25, recipes=rows,
                     files={str(p.relative_to(folder)).replace('\\', '/'): sha(p) for p in list(render.iterdir()) + [source] if p.is_file()})
        record.write_text(json.dumps(group, indent=2) + '\n', encoding='utf-8', newline='\n');groups.append(group)
        print(name, '17auditions verified; common gain', round(db(common), 3), 'dB')
    listen = ROOT / 'out/s3/listen';listen.mkdir(exist_ok=True)
    aliases = []
    for group in groups:
        for row in group['recipes']:
            path = listen / (group['material'] + '_' + row['scene'] + '.wav');shutil.copy2(row['path'], path)
            aliases.append(dict(path=str(path), sha256=sha(path), source_path=row['path']))
    (listen / 'manifest.json').write_text(json.dumps(dict(source_id=final['source_id'], files=aliases), indent=2) + '\n', encoding='utf-8', newline='\n')
    (ROOT / 'docs/reports/s3/demos.json').write_text(json.dumps(dict(source_id=final['source_id'], groups=groups), indent=2) + '\n', encoding='utf-8', newline='\n')


if __name__ == '__main__':
    main()
