"""Immutable R14 independent audio analysis and equal-RMS source/R13/R14 demos."""
import hashlib,json,pathlib,sys
import numpy as np
from scipy import signal,ndimage
from scipy.io import wavfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from r14_reference import band_events
ROOT=pathlib.Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def tp(x):return float(20*np.log10(max(1e-20,np.max(np.abs(signal.resample_poly(x,8,1,axis=0,window=('kaiser',10)))))))
def feature(x,fs,bpm):
    events=band_events(x,fs,bpm);n=len(x);f=np.zeros((3,128))
    for r,times in enumerate(events):
        for t in times:f[r,min(127,max(0,int(t*fs/n*128)))]=1
    # 20ms tolerance prevents filter delay/sub-sample jitter counting as a new beat.
    return ndimage.gaussian_filter1d(f,.020*fs/n*128,axis=1,mode='constant').ravel()
def cosine(a,b):return float(np.dot(a,b)/max(1e-30,np.linalg.norm(a)*np.linalg.norm(b)))
def main(before,after,report,demos):
    before,after,report,demos=map(pathlib.Path,(before,after,report,demos));assert not report.exists() and not demos.exists();demos.mkdir(parents=True)
    b=json.loads((before/'results.json').read_text());a=json.loads((after/'results.json').read_text());old={r['key']:r for r in b['rows']}
    exact=[r for r in a['rows'] if r['key'] in old and r['pcm_sha256']==old[r['key']]['pcm_sha256']];assert len(exact)==len(b['rows'])==444
    assert all(r['finite'] for r in a['rows']);new=[r for r in a['rows'] if r['recipe']==21];assert len(new)==96
    assert all(r['prefix_error']==0 for r in new if r['bars']==2)
    materials=json.loads((ROOT/'out/r13/materials_v2/materials.json').read_text());bpm={r['name']:r['bpm'] for r in materials['sources']}
    structures=[r for r in a['structures'] if r['recipe']>=21]
    assert len(structures)==54
    for r in structures:
        assert r['plans']>=(64 if r['recipe']==21 else 24),r
        assert r['rhythm_masks']>=(48 if r['recipe']==21 else 24),r
    ordinary={'drums90','drums120','drums140','drums170'}
    plan_similarity=[]
    for r in structures:
        if r['recipe']==21:
            passed=sum(v<=.65 for v in r['input_similarity']);plan_similarity.append({'source':r['source'],'passed':passed,'total':64,'ordinary':r['source'] in ordinary,'median':float(np.median(r['input_similarity']))})
            if r['source'] in ordinary:assert passed>=58,(r['source'],passed)
    audio=[];demo_rows=[]
    for name in sorted(bpm):
        fs,dry=wavfile.read(before/(name+'_source.wav'));dry=dry.astype(float);reference=feature(dry,fs,bpm[name])
        for seed in range(1,9):
            op=before/f'{name}_1_8_1_{seed}.wav';npth=after/f'{name}_1_21_1_{seed}.wav'
            fs2,x=wavfile.read(op);fs3,y=wavfile.read(npth);assert fs==fs2==fs3 and x.shape==y.shape==dry.shape;x=x.astype(float);y=y.astype(float)
            peak=tp(y);assert peak<=-1,(npth,peak)
            audio.append({'source':name,'seed':seed,'file':npth.name,'sha256':sha(npth),'true_peak_db':peak,'old_similarity':cosine(reference,feature(x,fs,bpm[name])),'new_similarity':cosine(reference,feature(y,fs,bpm[name]))})
            if seed<=4:
                matched=[]
                for z in (dry,x,y):matched.append(z*(10**(-18/20)/max(1e-20,float(np.sqrt(np.mean(z*z))))))
                common=min(1.,10**((-1.2-max(tp(z) for z in matched))/20));gap=np.zeros((round(fs*.3),2));z=np.concatenate([matched[0]*common,gap,matched[1]*common,gap,matched[2]*common]).astype(np.float32)
                path=demos/f'{name}_seed{seed}_source_R13_R14.wav';wavfile.write(path,fs,z);peak=tp(z.astype(float));assert peak<=-1
                demo_rows.append({'file':path.name,'source':name,'seed':seed,'sha256':sha(path),'true_peak_db':peak,'matched_rms_db':-18+20*np.log10(common)})
    oldmedian=float(np.median([r['old_similarity'] for r in audio]));newmedian=float(np.median([r['new_similarity'] for r in audio]));reduction=1-newmedian/oldmedian
    data={'method':'Independent scipy band-onset cosine,128cells,20msGaussian tolerance;8seeds/source. Equal-RMS source,300msgap,R13,300msgap,R14 demos.','before_sha256':sha(before/'results.json'),'after_sha256':sha(after/'results.json'),'old_exact':len(exact),'structures':structures,'plan_similarity':plan_similarity,'audio':audio,'old_median':oldmedian,'new_median':newmedian,'median_reduction':reduction,'goal_reduction':.20,'reduction_pass':reduction>=.20,'demos':demo_rows,'render_ms_new':{'mean':float(np.mean([r['render_ms'] for r in new])),'max':max(r['render_ms'] for r in new)}}
    report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(data,indent=2)+'\n');(demos/'comparisons.json').write_text(json.dumps(demo_rows,indent=2)+'\n')
    print(json.dumps({k:data[k] for k in ('old_exact','plan_similarity','old_median','new_median','median_reduction','reduction_pass')},indent=2))
    # Preserve the complete failing measurement before returning nonzero.
    if reduction<.20:raise RuntimeError('Rendered original-similarity goal not reached')
if __name__=='__main__':main(*sys.argv[1:])
