"""Production magnitude/ITD-preserving low-delay adaptation; S3 bank untouched."""
import hashlib
import json
import struct
from pathlib import Path
import numpy as np
from scipy.signal import correlate

ROOT=Path(__file__).resolve().parents[1]


def lag(h):
    return int(np.argmax(correlate(h[1],h[0],mode='full'))-(h.shape[-1]-1))


def main():
    raw=(ROOT/'Assets/Hrtf/horizontal.bin').read_bytes();offset=20
    for _ in range(3):
        fs,n=struct.unpack_from('<II',raw,offset);offset+=8
        count=3*72*2*n
        data=np.frombuffer(raw,dtype='<f4',count=count,offset=offset).reshape(3,72,2,n).astype(float)
        offset+=count*4
        if fs==48000:source=data.copy()
    size=65536;target=np.zeros((3,72,2,512));errors=[];itd=[];loss=[]
    hz=np.fft.rfftfreq(size,1/48000)
    for p in range(3):
        for a in range(72):
            h=source[p,a]
            magnitude=np.abs(np.fft.rfft(h,size,axis=-1))
            cepstrum=np.fft.irfft(np.log(np.maximum(magnitude,1e-12)),size,axis=-1)
            causal=np.zeros_like(cepstrum);causal[:,0]=cepstrum[:,0];causal[:,1:size//2]=2*cepstrum[:,1:size//2];causal[:,size//2]=cepstrum[:,size//2]
            minimum=np.fft.irfft(np.exp(np.fft.rfft(causal,axis=-1)),size,axis=-1)
            correction=lag(h)-lag(minimum[:,:2048])
            delays=[56-int(np.floor(correction/2)),56+int(np.ceil(correction/2))]
            assert min(delays)>=0
            for ear,d in enumerate(delays):
                target[p,a,ear,d:]=minimum[ear,:512-d]
                loss.append(float(1-np.sum(minimum[ear,:512-d]**2)/np.sum(minimum[ear]**2)))
            prepared=target[p,a].astype('<f4').astype(float)
            changed=np.abs(np.fft.rfft(prepared,size,axis=-1))
            mask=(hz>=100)&(hz<=16000)&(magnitude>np.max(magnitude,axis=-1,keepdims=True)*.01)
            errors.append(float(np.max(np.abs(20*np.log10(np.maximum(changed,1e-30)/np.maximum(magnitude,1e-30)))[mask])))
            itd.append(abs(lag(prepared)-lag(h)))
    blob=target.astype('<f4').tobytes();path=ROOT/'Assets/Hrtf/production.bin'
    if path.exists():assert path.read_bytes()==blob,'Refusing changed bank overwrite'
    else:path.write_bytes(blob)
    report=dict(source_bank_sha256=hashlib.sha256(raw).hexdigest(),production_bank_sha256=hashlib.sha256(blob).hexdigest(),
                tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),rate=48000,taps=512,reference=56,
                maximum_magnitude_error_db=max(errors),maximum_ITD_error_samples=max(itd),maximum_tail_energy_loss=max(loss),
                method='Real-cepstrum minimum phase; measured pair correlation lag restored around56samples; float32,512taps. Same profile gains as S3.')
    (ROOT/'Assets/Hrtf/production.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(report,indent=2))
    assert max(errors)<=.5 and max(itd)<=1


if __name__=='__main__':main()
