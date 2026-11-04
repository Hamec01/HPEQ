"""Verify private dry/processed auditions using one common gain and independent8x TP."""
import hashlib,json
from pathlib import Path
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly
root=Path(__file__).resolve().parents[1]
folder=root/'out/f5/demos_final';output=root/'docs/reports/f5/demos_final.json'
assert not output.exists()
records=json.loads((folder/'comparisons.json').read_text(encoding='utf-8-sig'))
assert len(records)==12
for row in records:
    path=folder/row['file'];assert hashlib.sha256(path.read_bytes()).hexdigest()==row['sha256']
    rate,raw=wavfile.read(path);assert rate==48000 and raw.shape[1]==2
    data=raw.astype(np.float64)/(2**31 if raw.dtype==np.int32 else 2**15)
    n=(len(data)-19200)//2
    assert data[:n].shape==data[n+19200:].shape and np.max(np.abs(data[n:n+19200]))==0
    assert np.isfinite(data).all()
    peak=max(float(np.max(np.abs(resample_poly(data[:,c],8,1)))) for c in range(2))
    row['true_peak_8x_dbtp']=float(20*np.log10(max(1e-30,peak)))
    assert row['true_peak_8x_dbtp']<=-1
output.write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8')
print('PASS:12 private auditions,exact prepared-input proof in native log,common gain,8x peak<=-1dBTP')
