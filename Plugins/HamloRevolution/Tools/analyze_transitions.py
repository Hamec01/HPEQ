"""Independent audio diagnostics for a completed TransitionLab run."""
import itertools
import json
import pathlib
import sys
import numpy as np
from scipy import signal
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from transition_materials import load,describe,sha


def main():
    before=pathlib.Path(sys.argv[1]);after=pathlib.Path(sys.argv[2]);dest=pathlib.Path(sys.argv[3]);assert not dest.exists()
    b=json.loads((before/'results.json').read_text());a=json.loads((after/'results.json').read_text())
    old={r['key']:r for r in b};legacy=[r for r in a if r['version']==0];new=[r for r in a if r['version']==1]
    assert len(old)==len(legacy)==96
    exact=all(r['pcm_sha256']==old[r['key']]['pcm_sha256'] for r in legacy)
    rises=[r for r in new if r['kind']==0]
    report=dict(method_sha256=sha(pathlib.Path(__file__)),legacy_exact=exact,legacy_cases=len(legacy),modern_cases=len(new),
                rise_growth_pass=sum(r['growth_db']>=6 for r in rises),rise_cases=len(rises),rise_min_growth_db=min(r['growth_db'] for r in rises),
                worst_render_ms=max(r['render_ms'] for r in new),median_render_ms=float(np.median([r['render_ms'] for r in new])),audio=[],diversity=[])
    for path in sorted(after.glob('*_1.wav')):
        fs,x=load(path);d=describe(x,fs);tp=float(np.max(np.abs(signal.resample_poly(x,8,1,axis=0))))
        report['audio'].append(dict(file=path.name,true_peak_db=20*np.log10(max(tp,1e-30)),
            brightness_ratio=float(np.mean(d['centroid8'][-2:])/max(1e-30,np.mean(d['centroid8'][:2]))),**d))
    for key,group in itertools.groupby(sorted(new,key=lambda r:(r['source'],r['kind'],r['bars'])),key=lambda r:(r['source'],r['kind'],r['bars'])):
        rows=list(group);cos=[]
        for x,y in itertools.combinations(rows,2):
            u=np.array(x['rms64']);v=np.array(y['rms64']);cos.append(float(np.dot(u,v)/max(1e-30,np.linalg.norm(u)*np.linalg.norm(v))))
        # First analysis used the full clip,including FILL's deliberately unchanged
        # earlier bars. Retain it and add independent edited-region/legacy comparisons.
        local=[];oldcos=[]
        for x,y in itertools.combinations(rows,2):
            size=64//key[2] if key[1]==1 else 64
            u=np.array(x['rms64'][-size:]);v=np.array(y['rms64'][-size:]);local.append(float(np.dot(u,v)/max(1e-30,np.linalg.norm(u)*np.linalg.norm(v))))
            ox=old[x['key'][:-1]+'0'];oy=old[y['key'][:-1]+'0'];u=np.array(ox['rms64'][-size:]);v=np.array(oy['rms64'][-size:]);oldcos.append(float(np.dot(u,v)/max(1e-30,np.linalg.norm(u)*np.linalg.norm(v))))
        report['diversity'].append(dict(source=key[0],kind=key[1],bars=key[2],median_envelope_cosine=float(np.median(cos)),max_envelope_cosine=max(cos),
            edited_median=float(np.median(local)),legacy_edited_median=float(np.median(oldcos)),
            modern_unique_pcm=len({r['pcm_sha256'] for r in rows}),legacy_unique_pcm=len({old[r['key'][:-1]+'0']['pcm_sha256'] for r in rows})))
    dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('Classic exact:',exact,'cases',len(legacy),'new',len(new))
    print('Rise growth >=6dB:',report['rise_growth_pass'],'/',len(rises),'min',round(report['rise_min_growth_db'],2))
    print('Render median/max ms:',round(report['median_render_ms'],1),round(report['worst_render_ms'],1))
    print('Worst8x true peak dBTP:',round(max(r['true_peak_db'] for r in report['audio']),3))
    rise_audio=[r for r in report['audio'] if '_0_' in r['file']]
    print('Rise brightness>=1.15:',sum(r['brightness_ratio']>=1.15 for r in rise_audio),'/',len(rise_audio))
    for r in report['diversity']:print(r)


if __name__=='__main__':main()
