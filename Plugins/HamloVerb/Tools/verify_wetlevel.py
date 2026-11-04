"""Replay the Wet Level acceptance receipt without rebuilding or rerendering."""
import hashlib
import json
import pathlib
import re
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]

def digest(path, normalized=False):
    data = path.read_bytes()
    if normalized:
        data = data.replace(b'\r\n', b'\n')
    return hashlib.sha256(data).hexdigest()

def check():
    report = ROOT/'docs/reports/wetlevel'
    receipt = json.loads((report/'acceptance.json').read_text(encoding='utf-8'))
    for name, pin in receipt['files'].items():
        assert digest(ROOT/name, pin['normalized']) == pin['sha256'], name
    old = (ROOT/'docs/reports/keepdry/native_final.txt').read_text(errors='replace')
    after = (report/'native_final.txt').read_text(errors='replace')
    pattern = r'^PROGRAM (\d+) ([a-f0-9]{64})$'
    assert len(re.findall(pattern, old, re.M)) == 208
    assert re.findall(pattern, old, re.M) == re.findall(pattern, after, re.M)
    summary = re.search(r'HamloVerb_Tests: (\d+) expectations passed, (\d+) failed', after)
    assert summary and int(summary[1]) == receipt['native_expectations'] and int(summary[2]) == 0
    assert (ROOT/'Tests/WetLevelTests.cpp').read_bytes() == (ROOT/'build/wetlevel_baseline/WetLevelTests.cpp').read_bytes()
    before = (report/'before.txt').read_text(errors='replace')
    for prefix in ('NEUTRAL ', 'SCREENSHOT '):
        assert [l for l in before.splitlines() if l.startswith(prefix)] == [l for l in after.splitlines() if l.startswith(prefix)]
    identity = json.loads((report/'validators/identity.json').read_text(encoding='utf-8-sig'))
    assert len(identity['results']) == 7 and all('exit=0' in v for v in identity['results'])
    assert '537 tests passed, 0 tests failed' in (report/'validators/steinberg.txt').read_text(errors='replace')
    assert digest(ROOT/receipt['build_dll']) == digest(ROOT/receipt['delivery_dll']) == identity['dll_sha256']
    reference = (report/'m0_reference.md').read_text(errors='replace')
    assert 'FAIL' not in reference and reference.count('| PASS |') == 390
    with zipfile.ZipFile(ROOT/receipt['archive']) as archive:
        assert archive.testzip() is None
        for name in archive.namelist():
            assert archive.read(name) == (ROOT/receipt['package']/name).read_bytes(), name
    print(f'WET LEVEL PASS: {summary[1]} native,208 exact programs,390 M0,unchanged neutral lab,validators,source/package hashes')

if __name__ == '__main__':
    check()
