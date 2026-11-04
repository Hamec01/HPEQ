"""Independent 8x true-peak and segment energy checks for private level auditions."""
import hashlib
import json
import pathlib
import re
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

root=pathlib.Path(__file__).resolve().parents[1]
folder=root/'out/wetlevel_demo_final'
report=root/'docs/reports/wetlevel/auditions.json'
assert not report.exists(), 'Refusing to overwrite evidence'
log=(root/'docs/reports/wetlevel/native_final.txt').read_text(errors='replace')
records=[]
for path in sorted(folder.glob('*.wav')):
    rate,raw=wavfile.read(path)
    assert rate==48000 and raw.ndim==2 and raw.shape[1]==2
    data=raw.astype(np.float64)/(2**31 if raw.dtype==np.int32 else 2**15)
    assert np.isfinite(data).all()
    peak=max(float(np.max(np.abs(resample_poly(data[:,c],8,1)))) for c in range(2))
    db=20*np.log10(max(1e-30,peak))
    assert db<=-1, (path.name,db)
    n=(data.shape[0]-19200)//2
    before=data[:n];after=data[n+19200:]
    assert before.shape==after.shape and np.max(np.abs(data[n:n+19200]))==0
    gain=10*np.log10(np.sum(after*after)/np.sum(before*before))
    if path.name.startswith('Send_'):assert abs(gain-12)<.01,gain
    match=re.search(r'AUDITION '+re.escape(path.name)+r' common_gain=([\d.]+) source_sha=([a-f0-9]{64})',log)
    assert match,path.name
    records.append(dict(path=path.relative_to(root).as_posix(),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        true_peak_db=float(db),after_before_energy_db=float(gain),common_gain=float(match[1]),source_sha256=match[2],
        order='WET LEVEL0dB,0.4s silence,WET LEVEL+12dB; same common gain'))
assert len(records)==2
report.write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8')
print('PASS:2 private auditions,8x truepeak<=-1dBTP,Send gain+12dB,common rather than separate normalization')
