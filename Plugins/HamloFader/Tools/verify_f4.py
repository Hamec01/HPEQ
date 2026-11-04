"""Verify the accepted F4 interface and paired unchanged audio laboratories."""
import hashlib,json,pathlib,re
ROOT=pathlib.Path(__file__).resolve().parents[1]
def sha(path,normal=False):
    data=path.read_bytes()
    return hashlib.sha256(data.replace(b'\r\n',b'\n') if normal else data).hexdigest()
def check():
    report=ROOT/'docs/reports/f4';receipt=json.loads((report/'acceptance.json').read_text())
    for name,pin in receipt['files'].items():assert sha(ROOT/name,pin['normalized'])==pin['sha256'],name
    text=(report/'native_v1.txt').read_text(errors='replace')
    assert '163 expectations passed, 0 failed' in text
    host=json.loads((report/'host_v1.json').read_text());legacy=json.loads((report/'legacy_v1.json').read_text())
    assert host['passed']==224 and host['failed']==0
    assert legacy['failures']==0 and len(legacy['cases'])==226
    identity=json.loads((report/'validators_v1/identity.json').read_text(encoding='utf-8-sig'))
    assert len(identity['results'])==7 and all('exit=0' in v for v in identity['results'])
    assert '537 tests passed, 0 tests failed' in (report/'validators_v1/steinberg.txt').read_text(errors='replace')
    assert sha(ROOT/receipt['dll'])==identity['dll_sha256']
    for path in (ROOT/'Source/DSP').glob('*'):
        if path.is_file():assert sha(path,True)==sha(ROOT/'out/f3/accepted_v2/Source/DSP'/path.name,True),path.name
    print('F4 PASS:163native,224host,678Legacy renders,unchanged DSP,validators,source/binary pins')
if __name__=='__main__':check()
