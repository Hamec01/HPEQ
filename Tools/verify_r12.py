"""Replay R12 source,corpus,compatibility and release checks without rerendering."""
import hashlib
import json
import math
import pathlib
import re
ROOT=pathlib.Path(__file__).resolve().parents[1]


def digest(path,normal=False):
    data=path.read_bytes()
    if normal:data=data.replace(b'\r\n',b'\n')
    return hashlib.sha256(data).hexdigest()


def check():
    report=ROOT/'docs/reports/r12';receipt=json.loads((report/'acceptance.json').read_text(encoding='utf-8-sig'))
    for name,pin in receipt['files'].items():assert digest(ROOT/name,pin['normalized'])==pin['sha256'],name
    before={r['key']:r for r in json.loads((report/'before.json').read_text())}
    after=json.loads((report/'after_final.json').read_text());classic=[r for r in after if r['version']==0];modern=[r for r in after if r['version']==1]
    assert len(before)==len(classic)==len(modern)==96
    for row in classic:assert row['pcm_sha256']==before[row['key']]['pcm_sha256'],row['key']
    assert all(r['finite'] for r in modern)
    rises=[r for r in modern if r['kind']==0];assert len(rises)==48 and all(r['growth_db']>=6 for r in rises)
    audio=json.loads((report/'analysis_final.json').read_text())['audio'];assert len(audio)==24
    assert all(math.isfinite(r['true_peak_db']) and r['true_peak_db']<=-1 for r in audio)
    rise_audio=[r for r in audio if '_0_' in r['file']];assert sum(r['brightness_ratio']>=1.15 for r in rise_audio)>=math.ceil(.9*len(rise_audio))
    old=json.loads((report/'playback_before.json').read_text())['programs'];new=json.loads((report/'playback_final.json').read_text())['programs']
    assert len(old)==len(new)==225
    for b,a in zip(old,new):assert b['program']==a['program'] and b['load']==a['load'] and b['restore']==a['restore'],a['program']
    native=(report/'native_final.txt').read_text(errors='replace');summary=re.search(r'HamloRevolution_Tests: (\d+) expectations passed, (\d+) failed',native)
    assert summary and int(summary[1])==receipt['native_expectations'] and int(summary[2])==0
    plans=re.findall(r'kind=(\d) recipe=(\d) structures=(\d+)',native);assert len(plans)==14
    for kind,recipe,count in plans:assert int(count)>=(12 if recipe=='1' else 4)
    identity=json.loads((report/'validators_final/identity.json').read_text(encoding='utf-8-sig'))
    assert len(identity['results'])==7 and all('exit=0' in s for s in identity['results'])
    assert '537 tests passed, 0 tests failed' in (report/'validators_final/steinberg.txt').read_text(errors='replace')
    assert digest(ROOT/receipt['build_dll'])==digest(ROOT/receipt['delivery_dll'])==identity['dll_sha256']
    assert digest(ROOT/'Tests/TransitionLab.cpp')=='bc31e4416511131098e451be213fefd494455cb740c4bc7ca8a90e05c7dacbdb'
    demos=json.loads((ROOT/'dist/HamloRevolution-0.3.0-r12/Audio/comparisons.json').read_text());assert len(demos)==12
    assert all(r['true_peak_db']<=-1 for r in demos)
    print(f"R12 PASS:96 exact Classic,96 new corpus renders,225 exact programs/states,{summary[1]} native,pluginval5-10/Steinberg537,12 A/B,hashes")


if __name__=='__main__':check()
