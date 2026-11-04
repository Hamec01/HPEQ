"""Independent 8x peak and calibration/check identity verification."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

def main():
    parser=argparse.ArgumentParser();parser.add_argument('calibration',type=Path);parser.add_argument('check',type=Path);parser.add_argument('output',type=Path);args=parser.parse_args()
    assert not args.output.exists(),'Refusing to overwrite evidence'
    cal=json.loads((args.calibration/'result.json').read_text(encoding='utf-8-sig'))
    check=json.loads((args.check/'result.json').read_text(encoding='utf-8-sig'))
    assert cal['passed']==check['passed']==200
    for key in ('dsp_sha256','method_sha256','skeleton_sha256','materials'):
        assert cal['identity'][key]==check['identity'][key],key
    original={r['id']:r for r in cal['recipes']}
    rows=[]
    for item in check['recipes']:
        assert item['passed'] and item['state_exact'] and item['workflow_valid']
        old=original[item['id']]
        assert abs(old['work_db']-item['work_db'])<.01,(item['id'],'calibration/check work mismatch')
        path=args.check/'audio'/f"{item['id']}.wav"
        assert hashlib.sha256(path.read_bytes()).hexdigest()==item['wav_sha256']
        fs,raw=wavfile.read(path);assert fs==48000 and raw.shape[1]==2
        audio=raw.astype(np.float64)/(2**31 if raw.dtype==np.int32 else 2**15)
        assert np.isfinite(audio).all()
        peak=max(float(np.max(np.abs(resample_poly(audio[:,c],8,1)))) for c in range(2))
        tp=float(20*np.log10(max(1e-30,peak)))
        assert tp<=-1,(item['id'],tp)
        rows.append(dict(id=item['id'],true_peak_8x_dbtp=tp,work_db=item['work_db'],
            calibration_delta_db=item['work_db']-old['work_db'],calibration_pcm_exact=item['pcm_sha256']==old['pcm_sha256'],wav_sha256=item['wav_sha256']))
        print(item['id'],f'{tp:.4f}dBTP',flush=True)
    assert len(rows)==200
    result=dict(passed=200,total=200,worst_peak_dbtp=max(r['true_peak_8x_dbtp'] for r in rows),
        exact_calibration_pcm=sum(r['calibration_pcm_exact'] for r in rows),
        max_work_delta_db=max(abs(r['calibration_delta_db']) for r in rows),recipes=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('PASS:200 independent8x peaks,identical DSP/method/material/skeleton,compiled-bank calibration agreement')

if __name__=='__main__':main()
