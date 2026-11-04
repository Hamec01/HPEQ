"""Create level-matched private before/after auditions from the frozen lab WAVs."""
import json
import pathlib
import sys
import numpy as np
from scipy import signal
from scipy.io import wavfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from transition_materials import load,sha


def main():
    before=pathlib.Path(sys.argv[1]);after=pathlib.Path(sys.argv[2]);out=pathlib.Path(sys.argv[3]);assert not out.exists();out.mkdir(parents=True)
    rows=[]
    for material in ('drums120','toms90','break170'):
        for kind,bars in ((0,2),(1,1)):
            for seed in (1,8):
                key=f'{material}_{kind}_{bars}_{seed}'
                bpath=before/(key+'_0.wav');apath=after/(key+'_1.wav');fs,b=load(bpath);rate,a=load(apath);assert fs==rate
                br=np.sqrt(np.mean(b*b));ar=np.sqrt(np.mean(a*a));assert br>1e-10 and ar>1e-10
                bp=np.max(np.abs(signal.resample_poly(b/br,8,1,axis=0)));ap=np.max(np.abs(signal.resample_poly(a/ar,8,1,axis=0)))
                target=min(10**(-18/20),10**(-1.1/20)/max(bp,ap));bg=target/br;ag=target/ar
                gap=np.zeros((round(fs*.4),2));combined=np.concatenate((b*bg,gap,a*ag)).astype(np.float32)
                label=f'{material}_{"RISE" if kind==0 else "DRUM_FILL"}_seed{seed}'
                path=out/(label+'_BEFORE_then_AFTER.wav');wavfile.write(path,fs,combined)
                rows.append(dict(file=path.name,sha256=sha(path),before_sha256=sha(bpath),after_sha256=sha(apath),
                    before_gain_db=float(20*np.log10(bg)),after_gain_db=float(20*np.log10(ag)),matched_rms=float(target),
                    true_peak_db=float(20*np.log10(np.max(np.abs(signal.resample_poly(combined,8,1,axis=0)))))))
    (out/'comparisons.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
    (out/'README.txt').write_text('Private owner-material comparisons. BEFORE,0.4s silence,AFTER.\n'
        'Both halves use the same RMS and an independent8x peak ceiling.\n'
        'Only uniform gain was changed;these files are listening demonstrations,not plugin exports.\n'
        'Original unscaled render identities are in comparisons.json. No reference-library audio is embedded in the plugin.\n',encoding='utf-8')
    print(f'{len(rows)} matched A/B comparisons;worst peak {max(r["true_peak_db"] for r in rows):.3f}dBTP')


if __name__=='__main__':main()
