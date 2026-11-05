"""Replay R13 evidence and delivery identity without rerendering private audio."""
import hashlib
import json
import pathlib
import re
import zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]


def digest(path, normal=False):
    data=path.read_bytes()
    return hashlib.sha256(data.replace(b'\r\n', b'\n') if normal else data).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def check():
    report=ROOT/'docs/reports/r13'
    receipt=read(report/'acceptance.json')
    for name,pin in receipt['files'].items():
        assert digest(ROOT/name,pin['normalized'])==pin['sha256'],name
    before=read(report/'before.json');after=read(report/'after_final.json')
    assert before['new_features']=='n/a' and after['new_features']=='available'
    assert len(before['rows'])==288 and len(after['rows'])==384
    old={r['key']:r for r in before['rows']}
    exact=[r for r in after['rows'] if r['key'] in old]
    assert len(exact)==288 and all(r['pcm_sha256']==old[r['key']]['pcm_sha256'] for r in exact)
    assert all(r['finite'] for r in after['rows'])
    new=[r for r in after['rows'] if r['recipe']==8]
    assert len(new)==96 and all(r['prefix_error']==0 for r in new if r['bars']==2)
    analysis=read(report/'analysis_final.json')
    assert analysis['before_sha256']==digest(report/'before.json')
    assert analysis['after_sha256']==digest(report/'after_final.json')
    assert len(analysis['structures'])==78
    for r in analysis['structures']:
        assert r['plans']>=(48 if r['recipe']==8 else 16)
        assert r['rhythm_masks32']>=(24 if r['recipe']==8 else 16)
    assert len(analysis['audio'])==48 and all(r['true_peak_db']<=-1 for r in analysis['audio'])
    assert len(analysis['demos'])==24 and all(r['true_peak_db']<=-1 for r in analysis['demos'])
    for r in analysis['demos']:
        assert digest(ROOT/receipt['package']/'Audio'/r['file'])==r['sha256']
    oldprograms=read(ROOT/'docs/reports/r12/playback_final.json')['programs']
    newprograms=read(report/'playback_final.json')['programs']
    assert len(oldprograms)==len(newprograms)==225
    for b,a in zip(oldprograms,newprograms):
        assert all(b[k]==a[k] for k in ('program','load','restore','latency')),a['program']
        assert a['missing_sources_reset']
    summary=re.search(r'HamloRevolution_Tests: (\d+) expectations passed, (\d+) failed',(report/'native_final.txt').read_text(errors='replace'))
    assert summary and int(summary[1])==receipt['native_expectations'] and int(summary[2])==0
    identity=read(report/'validators_final/identity.json')
    assert len(identity['results'])==7 and all('exit=0' in s for s in identity['results'])
    assert '537 tests passed, 0 tests failed' in (report/'validators_final/steinberg.txt').read_text(errors='replace')
    assert digest(ROOT/receipt['build_dll'])==digest(ROOT/receipt['delivery_dll'])==identity['dll_sha256']
    assert digest(ROOT/'Tests/DrumFillLab.cpp')==digest(ROOT/'out/r13/DrumFillLab-before.cpp')
    assert digest(ROOT/'Tests/TransitionLab.cpp')=='bc31e4416511131098e451be213fefd494455cb740c4bc7ca8a90e05c7dacbdb'
    with zipfile.ZipFile(ROOT/receipt['archive']) as z:
        assert z.testzip() is None and not any('Audio/' in n for n in z.namelist())
        for n in z.namelist():assert z.read(n)==(ROOT/receipt['package']/n).read_bytes(),n
    print(f'R13 PASS:288 old exact,96 new renders,78 variation probes,225 programs/states,{summary[1]} native,48 peaks,24 A/B,validators,ZIP/SHA')


if __name__=='__main__':check()
