"""Verify the Keep Dry release against its frozen measurements and source pins."""
import hashlib
import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]


def digest(path, normalized=False):
    value = path.read_bytes()
    if normalized:
        value = value.replace(b'\r\n', b'\n')
    return hashlib.sha256(value).hexdigest()


def check():
    report = ROOT / 'docs/reports/keepdry'
    receipt = json.loads((report / 'acceptance.json').read_text(encoding='utf-8-sig'))
    for name, pin in receipt['files'].items():
        assert digest(ROOT / name, pin['normalized']) == pin['sha256'], name
    before = (report / 'before.txt').read_text(errors='replace')
    after = (report / 'native_final.txt').read_text(errors='replace')
    old = re.findall(r'^PROGRAM (\d+) ([a-f0-9]{64})$', before, re.M)
    new = re.findall(r'^PROGRAM (\d+) ([a-f0-9]{64})$', after, re.M)
    assert len(old) == len(new) == 208 and old == new
    summary = re.search(r'HamloVerb_Tests: (\d+) expectations passed, (\d+) failed', after)
    assert summary and int(summary[1]) == receipt['native_expectations'] and int(summary[2]) == 0
    assert len(re.findall(r'LEVEL character=', after)) == 18
    identity = json.loads((report / 'validators/identity.json').read_text(encoding='utf-8-sig'))
    assert len(identity['results']) == 7 and all('exit=0' in v for v in identity['results'])
    assert '537 tests passed, 0 tests failed' in (report / 'validators/steinberg.txt').read_text(errors='replace')
    assert digest(ROOT / receipt['build_dll']) == digest(ROOT / receipt['delivery_dll']) == identity['dll_sha256']
    reference = (report / 'm0_reference.md').read_text(errors='replace')
    assert 'FAIL' not in reference
    assert reference.count('| PASS |') == 390
    print(f"KEEP DRY PASS:208 unchanged program renders,{summary[1]} native,"
          "390 M0,pluginval5-10/Steinberg537,source/release hashes")


if __name__ == '__main__':
    check()
