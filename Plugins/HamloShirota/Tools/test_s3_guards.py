"""Reject stale data, compiled objects, method identities and altered raw caches."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'out/s3'
final = json.loads((ROOT / 'docs/reports/s3/final.json').read_text())
original = json.loads((WORK / 'final/manifest.json').read_text())
results = []


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def native(name, executable, corrupt, message):
    with tempfile.TemporaryDirectory(dir=WORK, prefix='fault-') as temp:
        folder = Path(temp);assert folder.resolve().is_relative_to(WORK.resolve())
        bank = ROOT / 'Assets/Hrtf/horizontal.bin'
        if corrupt:
            data = bytearray(bank.read_bytes());data[-1] ^= 1
            bank = folder / 'bad.bin';bank.write_bytes(data)
        output = folder / 'output'
        p = subprocess.run([str(ROOT / 'build/Release' / executable), str(bank), str(output)], cwd=ROOT, capture_output=True, text=True)
        passed = p.returncode == 2 and message in p.stderr and not output.exists()
        results.append(dict(name=name, passed=passed, exit_code=p.returncode, output_created=output.exists(), rejection=message))
        assert passed, p.stderr


def analyzer(name, change, message, corrupt=False):
    with tempfile.TemporaryDirectory(dir=WORK, prefix='fault-') as temp:
        folder = Path(temp);assert folder.resolve().is_relative_to(WORK.resolve())
        manifest = dict(original);change(manifest)
        path = folder / 'manifest.json';path.write_text(json.dumps(manifest), encoding='utf-8')
        if corrupt:
            for file in ['stress.json', 'benchmark.json']:
                (folder / file).write_bytes((WORK / 'final' / file).read_bytes())
            (folder / 'fake.f32').write_bytes(b'wrong samples')
            binding = dict(files=final['files'], manifest=sha(path), renders={'fake.f32': '0' * 64}, stress=sha(folder / 'stress.json'), benchmark=sha(folder / 'benchmark.json'))
            (folder / 'identity.json').write_text(json.dumps(binding), encoding='utf-8')
        p = subprocess.run([sys.executable, '-I', '-X', 'utf8', 'Tools/analyze_s3.py', str(folder), str(folder / 'report.json')], cwd=ROOT, capture_output=True, text=True)
        passed = p.returncode != 0 and message in p.stderr and not (folder / 'report.json').exists()
        results.append(dict(name=name, passed=passed, exit_code=p.returncode, rejection=message))
        assert passed, p.stderr


native('corrupt_HRIR_bank', 'HamloShirota_MotionLab.exe', True, 'HRTF bank SHA256 mismatch')
native('mixed_translation_units', 'HamloShirota_StaleMotionProbe.exe', False, 'Mixed motion translation units')
analyzer('stale_data_method_identity', lambda m: m.update(dsp_identity='old data'), 'Stale compiled DSP/method/data identity')
analyzer('unimplemented_candidate', lambda m: m.update(implemented=False), 'Wrong baseline/candidate role')
analyzer('corrupt_PCM_cache', lambda m: None, 'Corrupt/stale cached run', True)
(ROOT / 'docs/reports/s3/guard_tests.json').write_text(json.dumps(dict(source_id=final['source_id'], checks=results), indent=2) + '\n', encoding='utf-8', newline='\n')
print('5/5 provenance rejection tests passed')
