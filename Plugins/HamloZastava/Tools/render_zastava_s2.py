"""Source-pinned owner-material audition renders, separate from acceptance fixtures."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
import numpy as np
from scipy import signal
from scipy.io import wavfile

ROOT=Path(__file__).resolve().parents[1]


def sha(path,normal=False):
    data=path.read_bytes()
    return hashlib.sha256(data.replace(b'\r\n',b'\n') if normal else data).hexdigest()


def db(value):return float(20*np.log10(max(float(value),1e-30)))
def true_peak(audio):return float(np.max(np.abs(signal.resample_poly(audio,4,1,axis=0))))


def load(path):
    rate,audio=wavfile.read(path)
    if audio.ndim==1:audio=np.repeat(audio[:,None],2,axis=1)
    if audio.shape[1]!=2:raise ValueError('Expected mono or stereo material')
    if audio.dtype.kind=='i':audio=audio.astype(np.float64)/(2**(8*audio.dtype.itemsize-1))
    elif audio.dtype.kind=='u':audio=(audio.astype(np.float64)-128)/128
    else:audio=audio.astype(np.float64)
    assert np.isfinite(audio).all(),'Nonfinite source material'
    audio=audio[:int(12*rate)]
    divisor=np.gcd(rate,48000)
    if rate!=48000:audio=signal.resample_poly(audio,48000//divisor,rate//divisor,axis=0)
    scale=10**(-6/20)/max(true_peak(audio),1e-20)
    return np.pad((audio*scale).astype('<f4'),((0,12000),(0,0))),float(scale),rate


def main():
    p=argparse.ArgumentParser();p.add_argument('material',type=Path);p.add_argument('output',type=Path);p.add_argument('report',type=Path);args=p.parse_args()
    if args.output.exists():raise ValueError('Refusing to overwrite auditions')
    args.output.mkdir(parents=True)
    binary=ROOT/'build/Release/HamloZastava_V2Render.exe'
    accepted=json.loads((ROOT/'docs/reports/s2/candidate_02.json').read_text())
    result=dict(source_id=accepted['source_id'],renderer_sha256=sha(binary),driver_sha256=sha(ROOT/'Tests/V2RenderMain.cpp',True),script_sha256=sha(Path(__file__),True),rate=48000,rows=[])
    for recipe,filename in [('Clean','vocal_lead1.wav'),('Drum','drumbus1.wav'),('Duck','keys.wav'),('Rhythm','synth.wav')]:
        source=args.material/filename;audio,scale,original_rate=load(source)
        input_path=args.output/(recipe+'_input.f32');audio.tofile(input_path)
        dry=np.zeros_like(audio);dry[240:]=audio[:-240]
        wavfile.write(args.output/(recipe+'_dry.wav'),48000,dry)
        keypath='-';key_info=None
        if recipe=='Duck':
            key_source=args.material/'kick.wav';kick,key_scale,key_rate=load(key_source)
            key=np.zeros_like(audio);length=min(19200,len(kick))
            for at in range(0,len(audio)-12000,24000):
                count=min(length,len(audio)-at);key[at:at+count]+=kick[:count]
            keypath=args.output/'Duck_key.f32';key.tofile(keypath);wavfile.write(args.output/'Duck_key.wav',48000,key)
            key_info=dict(path=str(key_source),sha256=sha(key_source),scale=key_scale,original_rate=key_rate,placement='Every quarter note at 120 BPM; first 400 ms of the owner kick')
        for profile in ['Soft','Hard']:
            name=recipe+'_'+profile;folder=args.output/name
            subprocess.run([str(binary),str(input_path),str(keypath),str(folder),recipe,profile],check=True,capture_output=True)
            m=json.loads((folder/'manifest.json').read_text())
            assert m['source_id']==result['source_id'] and m['render_id']==result['driver_sha256'],'Stale audition binary'
            assert m['allocations']==0 and m['finite'] and m['latency']==240
            out=np.fromfile(folder/'output.f32',dtype='<f4').reshape(-1,2)
            assert out.shape==dry.shape and np.isfinite(out).all()
            peak=true_peak(out);assert db(peak)<=-1,'Audition exceeds -1 dBTP'
            wav=args.output/(name+'.wav');wavfile.write(wav,48000,out)
            # Level groups describe the source; they are not annotated wanted/noise labels.
            count=len(dry)//480;frame_energy=np.mean(dry[:count*480].reshape(count,480,2).astype(float)**2,axis=(1,2))
            output_energy=np.mean(out[:count*480].reshape(count,480,2).astype(float)**2,axis=(1,2))
            active=frame_energy>1e-12;low_limit,high_limit=np.quantile(frame_energy[active],[.25,.75])
            def energy_gain(mask):return db(np.sqrt(np.sum(output_energy[mask])/max(np.sum(frame_energy[mask]),1e-30)))
            result['rows'].append(dict(name=name,recipe=recipe,profile=profile,input=dict(path=str(source),sha256=sha(source),original_rate=original_rate,scale=scale,prepared_sha256=sha(input_path)),key=key_info,manifest=m,raw_sha256=sha(folder/'output.f32'),wav=str(wav),wav_sha256=sha(wav),true_peak_dbtp=db(peak),low_level_gain_db=energy_gain(active&(frame_energy<=low_limit)),high_level_gain_db=energy_gain(active&(frame_energy>=high_limit)),largest_adjacent_step=float(np.max(np.abs(np.diff(out,axis=0)))),dry_largest_adjacent_step=float(np.max(np.abs(np.diff(dry,axis=0))))))
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Owner auditions:',len(result['rows']),'finite, allocation-free, <= -1 dBTP')


if __name__=='__main__':main()
