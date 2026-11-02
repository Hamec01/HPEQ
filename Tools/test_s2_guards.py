"""Negative tests for the actual mixed-build error and stale laboratory caches."""
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'out/s2'
source = json.loads((ROOT / 'docs/reports/s2/final_v4.json').read_text())
manifest = json.loads((WORK / 'final_v4/manifest.json').read_text())
results = []


def reject(name, change, message, stale_cache=False):
    with tempfile.TemporaryDirectory(prefix='guard-', dir=WORK) as temp:
        folder = Path(temp)
        assert folder.resolve().is_relative_to(WORK.resolve()), 'Temporary cleanup must stay inside the S2 workspace'
        altered = dict(manifest)
        change(altered)
        path = folder / 'manifest.json'
        path.write_text(json.dumps(altered), encoding='utf-8')
        if stale_cache:
            (folder / 'example.f32').write_bytes(b'wrong PCM')
            binding = dict(files=source['files'], manifest=hashlib.sha256(path.read_bytes()).hexdigest(), renders={'example.f32': '0' * 64})
            (folder / 'identity.json').write_text(json.dumps(binding))
        p = subprocess.run([sys.executable, '-I', '-X', 'utf8', 'Tools/analyze_s2.py', str(folder), str(folder / 'report.json')], cwd=ROOT, capture_output=True, text=True)
        passed = p.returncode != 0 and message in p.stderr and not (folder / 'report.json').exists()
        results.append(dict(name=name, passed=passed, expected_rejection=message, exit_code=p.returncode))
        assert passed, p.stderr


reject('dsp_identity_mismatch', lambda m: m.update(dsp_identity='old object'), 'Compiled DSP/method identity differs')
reject('unimplemented_candidate', lambda m: m.update(implemented=False), 'Wrong experiment role')
reject('corrupt_cached_pcm', lambda m: None, 'Stale run', stale_cache=True)
target = WORK / 'stale_probe_output'
assert not target.exists()
p = subprocess.run([str(ROOT / 'build/Release/HamloShirota_StaleBuildProbe.exe'), str(target)], cwd=ROOT, capture_output=True, text=True)
passed = p.returncode == 2 and 'Mixed incremental build' in p.stderr and not target.exists()
results.append(dict(name='mixed_cpp_translation_units', passed=passed, exit_code=p.returncode, output_created=target.exists()))
assert passed, p.stderr
(ROOT / 'docs/reports/s2/guard_tests.json').write_text(json.dumps(dict(source_id=source['source_id'], checks=results), indent=2) + '\n', encoding='utf-8')
print('4/4 provenance fault-injection tests passed; invalid runs are rejected before a report/output is created.')
