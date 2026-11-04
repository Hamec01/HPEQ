"""Replay the R10 receipt against source, laboratory, test and release identities."""
import hashlib
import json
import math
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]


def digest(path, normalized=False):
    data = path.read_bytes()
    if normalized:
        data = data.replace(b'\r\n', b'\n')
    return hashlib.sha256(data).hexdigest()


def check(root=ROOT):
    report = root / 'docs/reports/r10'
    receipt = json.loads((report / 'acceptance.json').read_text(encoding='utf-8-sig'))
    for name, expected in receipt['files'].items():
        assert digest(root / name, expected['normalized']) == expected['sha256'], name
    before = json.loads((report / 'before.json').read_text())
    after = json.loads((report / 'after_release.json').read_text())
    assert len(before['programs']) == len(after['programs']) == 225
    for b, a in zip(before['programs'], after['programs']):
        assert b['program'] == a['program']
        # The first receipt verifier incorrectly cross-compared different procedures.
        # R9 already has17 load-vs-after-playback-restore hash differences. T1 is
        # compatibility: compare the same procedure across versions, independently.
        assert b['load'] == a['load'] and b['restore'] == a['restore'], a['program']
        assert a['missing_sources_reset'] and b['latency'] == a['latency']
    assert len(after['probes']) == 16
    for a in after['probes']:
        assert a['finite'] and math.isfinite(a['return_dry_db'])
        assert a['return_dry_db'] >= -24 and a['peak'] < 1 and a['tail_energy'] > 0, a
        assert a['source'] == 1 and a['condition'] == 0
    native = (report / 'native_delivery.txt').read_text(encoding='utf-8', errors='replace')
    match = re.search(r'HamloRevolution_Tests: (\d+) expectations passed, (\d+) failed', native)
    assert match and int(match[1]) == receipt['native_expectations'] and int(match[2]) == 0
    identity = json.loads((report / 'validators_delivery/identity.json').read_text(encoding='utf-8-sig'))
    assert len(identity['results']) == 7 and all('exit=0' in s for s in identity['results'])
    assert '537 tests passed, 0 tests failed' in (report / 'validators_delivery/steinberg.txt').read_text(errors='replace')
    assert digest(root / receipt['delivery_dll']) == identity['dll_sha256']
    assert digest(root / receipt['build_dll']) == identity['dll_sha256']
    print(f"R10 PASS:225 exact programs+old-state reloads,16/16 spatial probes,"
          f"{receipt['native_expectations']} native,pluginval5-10/Steinberg537,source+release hashes")


if __name__ == '__main__':
    check()
