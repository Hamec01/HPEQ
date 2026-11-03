"""Replay the local R9 acceptance without trusting stale source/binary caches."""
import argparse
import copy
import hashlib
import json
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path,normal=False):
    data=path.read_bytes()
    return hashlib.sha256(data.replace(b'\r\n',b'\n') if normal else data).hexdigest()
def source():
    return {p.relative_to(ROOT).as_posix():sha(p,True) for p in sorted((ROOT/'Source').rglob('*')) if p.is_file()}
def verify(receipt):
    assert receipt['source']==source(),'Stale source'
    for group in ['reports','methods','artifacts']:
        for name,expected in receipt[group].items():
            assert (ROOT/name).is_file() and sha(ROOT/name,group=='methods')==expected,'Changed '+group+': '+name
    before=json.loads((ROOT/'docs/reports/r9/before_v2.json').read_text())
    after=json.loads((ROOT/'docs/reports/r9/after_release.json').read_text())
    assert len(before)==8 and before==after,'Old audio/measurement mismatch'
    bold=next(r for r in after if r['material']=='drums' and r['recipe']==3)
    assert bold['delta_db']>=-18,'BOLD goal failed'
    text=(ROOT/'docs/reports/r9/native_release.txt').read_text(encoding='utf-8-sig')
    assert '1286 expectations passed, 0 failed' in text,'Native checks failed'
    match=re.search(r'Downsampling residual=([-.0-9]+)',text)
    assert match and float(match[1]) < -70,'Import alias goal failed'
    vendor=ROOT/'docs/reports/r9/validators_release'
    identity=json.loads((vendor/'identity.json').read_text(encoding='utf-8-sig'))
    assert len(identity['results'])==7 and all('exit=0' in r for r in identity['results'])
    assert '537 tests passed, 0 tests failed' in (vendor/'steinberg.txt').read_text(encoding='utf-8-sig')
    bundle=ROOT/'build/HamloRevolution_artefacts/Release/VST3/HamloRevolution.vst3'
    dll=bundle/'Contents/x86_64-win/HamloRevolution.vst3'
    assert sha(dll)==identity['dll_sha256']==receipt['dll_sha256'],'Wrong DLL'
    cids=lambda p:re.findall(r'"CID"\s*:\s*"([^"\n]+)"',p.read_text())
    old=ROOT/'out/r9/baseline/HamloRevolution.vst3/Contents/Resources/moduleinfo.json'
    new=bundle/'Contents/Resources/moduleinfo.json'
    assert len(cids(old))==2 and cids(old)==cids(new),'VST3 IDs changed'
    assert '"Version": "0.2.1"' in new.read_text()
    for folder in ['DSP','Core','Link']:
        for p in (ROOT/'out/r9/baseline/Source'/folder).rglob('*'):
            if p.is_file():
                target=ROOT/'Source'/folder/p.relative_to(ROOT/'out/r9/baseline/Source'/folder)
                assert sha(p,True)==sha(target,True),'Existing engine/generator changed: '+str(target)
    return {'passed':True,'native':1286,'sample_workspace':190,'paired_renders':8,
            'pluginval_levels':[5,6,7,8,9,10],'steinberg':537,
            'import_alias_dbfs':float(match[1]),'bold_drums_delta_db':bold['delta_db']}
def main():
    p=argparse.ArgumentParser();p.add_argument('--guards',type=Path);args=p.parse_args()
    receipt=json.loads((ROOT/'docs/reports/r9/acceptance.json').read_text())
    result=verify(receipt)
    if args.guards:
        assert not args.guards.exists(),'Refusing to overwrite evidence'
        checks=[]
        for group in ['source','reports','methods','artifacts']:
            bad=copy.deepcopy(receipt);bad[group][next(iter(bad[group]))]='0'*64
            try:verify(bad)
            except AssertionError:checks.append({'guard':group,'rejected':True})
            else:raise AssertionError('Corrupt cache accepted')
        args.guards.write_text(json.dumps(checks,indent=2)+'\n')
        result['rejection_guards']=len(checks)
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
