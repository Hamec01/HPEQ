"""Replay R14 compatibility,composition measurements and delivered VST3 identity."""
import hashlib,json,pathlib,re,zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def digest(p,normal=False):
    data=p.read_bytes();return hashlib.sha256(data.replace(b'\r\n',b'\n') if normal else data).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def check():
    report=ROOT/'docs/reports/r14';receipt=read(report/'acceptance.json')
    for name,pin in receipt['files'].items():assert digest(ROOT/name,pin['normalized'])==pin['sha256'],name
    b=read(report/'before.json');a=read(report/'after_final.json');old={r['key']:r for r in b['rows']}
    assert b['composition']=='n/a' and a['composition']=='available'
    assert len(b['rows'])==444 and len(a['rows'])==540
    previous=[r for r in a['rows'] if r['key'] in old]
    assert len(previous)==444 and all(r['pcm_sha256']==old[r['key']]['pcm_sha256'] for r in previous)
    assert all(r['finite'] for r in a['rows']);new=[r for r in a['rows'] if r['recipe']==21]
    assert len(new)==96 and all(r['prefix_error']==0 for r in new if r['bars']==2)
    analysis=read(report/'analysis_final.json')
    assert analysis['before_sha256']==digest(report/'before.json') and analysis['after_sha256']==digest(report/'after_final.json')
    assert len(analysis['structures'])==54
    for r in analysis['structures']:
        assert r['plans']>=(64 if r['recipe']==21 else 24)
        assert r['rhythm_masks']>=(48 if r['recipe']==21 else 24)
    assert all(r['passed']>=58 for r in analysis['plan_similarity'] if r['ordinary'])
    assert analysis['median_reduction']>=.20 and analysis['reduction_pass']
    assert len(analysis['audio'])==48 and all(r['true_peak_db']<=-1 for r in analysis['audio'])
    assert len(analysis['demos'])==24 and all(r['true_peak_db']<=-1 for r in analysis['demos'])
    for r in analysis['demos']:assert digest(ROOT/receipt['package']/'Audio'/r['file'])==r['sha256']
    refs=read(report/'reference_analysis.json');assert refs['inventory']==790 and refs['analyzed']==597 and len(refs['references'])==597
    before=read(ROOT/'docs/reports/r13/playback_final.json')['programs'];after=read(report/'playback_final.json')['programs']
    assert len(before)==len(after)==225
    for b,a in zip(before,after):
        assert all(b[k]==a[k] for k in ('program','load','restore','latency')),a['program']
        assert a['missing_sources_reset']
    native=(report/'native_final.txt').read_text(errors='replace');match=re.search(r'HamloRevolution_Tests: (\d+) expectations passed, (\d+) failed',native)
    assert match and int(match[1])==receipt['native_expectations'] and int(match[2])==0
    identity=read(report/'validators_final/identity.json');assert len(identity['results'])==7 and all('exit=0' in v for v in identity['results'])
    assert '537 tests passed, 0 tests failed' in (report/'validators_final/steinberg.txt').read_text(errors='replace')
    assert digest(ROOT/receipt['build_dll'])==digest(ROOT/receipt['delivery_dll'])==identity['dll_sha256']
    assert digest(ROOT/'Tests/DrumComposeLab.cpp')==digest(ROOT/'out/r14/DrumComposeLab-before.cpp')
    assert digest(ROOT/'Tests/DrumFillLab.cpp')==digest(ROOT/'out/r13/DrumFillLab-before.cpp')
    assert digest(ROOT/'Tests/TransitionLab.cpp')=='bc31e4416511131098e451be213fefd494455cb740c4bc7ca8a90e05c7dacbdb'
    with zipfile.ZipFile(ROOT/receipt['archive']) as z:
        assert z.testzip() is None and not any('Audio/' in n for n in z.namelist())
        for name in z.namelist():assert z.read(name)==(ROOT/receipt['package']/name).read_bytes()
    print(f'R14 PASS:444 old exact,96 new renders,597 reference analyses,54 variation probes,225 programs/states,{match[1]} native,48 peaks,24 A/B,validators,ZIP/SHA')
if __name__=='__main__':check()
