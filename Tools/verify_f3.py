"""Local F3 acceptance replay. Normal builds do not require the audit archives."""
import argparse
import copy
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RECEIPT = ROOT / 'docs/reports/f3/acceptance.json'

def sha(path, normalize=False):
    data = path.read_bytes()
    return hashlib.sha256(data.replace(b'\r\n', b'\n') if normalize else data).hexdigest()

def identity(prefix, paths):
    return hashlib.sha256((prefix + ''.join(sha(p, True) for p in paths)).encode()).hexdigest()

def source(root):
    return identity('FaderF3SourceV1', sorted(p for p in (root/'Source').rglob('*') if p.is_file()))

def method(host=False):
    files = ['Tests/F3HostMain.cpp' if host else 'Tests/F3LegacyLab.cpp',
             'Tests/AllocationAudit.cpp', 'Tests/AllocationAudit.h', 'docs/F3_METHODS.md']
    return identity('FaderF3HostMethodV1' if host else 'FaderF3MethodV1', [ROOT/p for p in files])

def validate(receipt):
    assert receipt['stage'] == 'F3'
    assert receipt['source_id'] == source(ROOT), 'Stale F3 source'
    assert receipt['host_method_id'] == method(True), 'Stale host method'
    assert receipt['legacy_method_id'] == method(), 'Stale Legacy method'
    for group in ['reports', 'raw', 'tools', 'binaries']:
        for name, expected in receipt[group].items():
            path = ROOT/name
            assert path.is_file() and sha(path, group == "tools") == expected, f'Corrupt or missing {group}: {name}'
    before = json.loads((ROOT/'docs/reports/f3/legacy_before.json').read_text())
    after = json.loads((ROOT/'docs/reports/f3/legacy_final.json').read_text())
    assert before['source_id'] == source(ROOT/'out/f3/baseline'), 'Control source mismatch'
    assert before['method_id'] == after['method_id'] == method()
    assert before['contract'] == after['contract'] and len(after['cases']) == 226
    assert before['failures'] == after['failures'] == 0
    assert after['source_id'] == receipt['source_id']
    for a, b in zip(before['cases'], after['cases']):
        assert a == b and b['passed'], f'Legacy difference: {b["case"]}'
        for observation in b['observations']:
            assert observation['allocations'] == 0 and observation['finite']
    host = json.loads((ROOT/'docs/reports/f3/host_final_v2.json').read_text())
    assert host['source_id'] == receipt['source_id'] and host['method_id'] == method(True)
    assert host['passed'] == 224 and host['failed'] == 0 and all(r['passed'] for r in host['results'])
    assert all(t['mean_block_percent'] <= 25 for t in host['timings'])
    regression = json.loads((ROOT/'docs/reports/f3/regression_final_v2.json').read_text())
    assert regression['method_id'] == method(True) and regression['failed'] >= 1
    kernel = json.loads((ROOT/'docs/reports/f3/kernel_after_v1.json').read_text())
    kernel_source = identity('FaderF2KernelV1', [ROOT/p for p in [
        'Source/DSP/FaderKernel.h','Source/DSP/FaderMeter.h',
        'Source/DSP/FaderKernel.cpp','Source/DSP/FaderMeter.cpp']])
    assert kernel['source_id'] == kernel_source == receipt['kernel_source_id']
    assert kernel['method_id'] == receipt['kernel_method_id']
    assert sum(c['passed'] is True for c in kernel['checks']) == 281
    assert all(c['passed'] is True for c in kernel['checks'])
    assert re.search(r'59 passed, 0 failed', (ROOT/'docs/reports/f3/kernel_unit_v1.txt').read_text())
    assert re.search(r'49 expectations passed, 0 failed', (ROOT/'docs/reports/f3/native_final.txt').read_text())
    vendor = ROOT/'docs/reports/f3/validators_final'
    report = json.loads((vendor/'identity.json').read_text(encoding='utf-8-sig'))
    assert len(report['results']) == 7 and all('exit=0' in r for r in report['results'])
    assert '537 tests passed, 0 tests failed' in (vendor/'steinberg.txt').read_text(encoding='utf-8-sig')
    dll = 'build/HamloFader_artefacts/Release/VST3/HamloFader.vst3/Contents/x86_64-win/HamloFader.vst3'
    assert report['dll_sha256'] == sha(ROOT/dll) == receipt['dll_sha256']
    moduleinfo = 'Contents/Resources/moduleinfo.json'
    cids = lambda p: re.findall(r'"CID"\s*:\s*"([^"\n]+)"', p.read_text())
    old = cids(ROOT/'out/f3/baseline/HamloFader.vst3'/moduleinfo)
    new = cids(ROOT/'build/HamloFader_artefacts/Release/VST3/HamloFader.vst3'/moduleinfo)
    assert len(old) == 2 and old == new, 'VST class identity changed'
    # These sources/methods are frozen. F3 intentionally supersedes F1/F2 whole-shipping guards.
    for part in ['Source/DSP/FaderGain.cpp','Source/DSP/LegacyFaderCore.cpp',
                 'Source/UI/BlackSteel.cpp','Source/UI/BlackSteel.h',
                 'Source/UI/HamloFaderPanel.cpp','Source/UI/HamloFaderPanel.h',
                 'Source/UI/PresetScreen.cpp','Source/UI/PresetScreen.h',
                 'Tests/F1LabMain.cpp','Tests/F2LabMain.cpp','Tools/analyze_f2.py',
                 'docs/HAMLOFADER_V2.md']:
        assert sha(ROOT/part,True) == sha(ROOT/'out/f3/baseline'/part,True), f'Frozen file changed: {part}'
    return {'passed': True, 'host': 224, 'legacy_renders': 678, 'kernel_lab': 281,
            'kernel_unit': 59, 'native': 49, 'vendor_levels': [5,6,7,8,9,10],
            'steinberg': 537, 'negative_probe_failures': regression['failed']}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--guards', type=Path, help='write NEW rejection-guard report')
    args = parser.parse_args()
    receipt = json.loads(RECEIPT.read_text())
    result = validate(receipt)
    if args.guards:
        assert not args.guards.exists(), 'Refusing to overwrite guard evidence'
        checks = []
        for field in ['source_id', 'host_method_id', 'legacy_method_id', 'raw', 'binaries']:
            bad = copy.deepcopy(receipt)
            if isinstance(bad[field], dict):
                bad[field][next(iter(bad[field]))] = '0'*64
            else:
                bad[field] = '0'*64
            try:
                validate(bad)
            except AssertionError:
                checks.append({'guard': field, 'rejected': True})
            else:
                raise AssertionError('Guard accepted corrupt receipt: '+field)
        args.guards.parent.mkdir(parents=True, exist_ok=True)
        args.guards.write_text(json.dumps(checks, indent=2)+'\n')
        result['rejection_guards'] = len(checks)
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    main()
