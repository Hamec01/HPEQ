"""Create unclipped, equal-loudness A/B pairs from private bank render WAVs.

Usage: python prepare_auditions.py bank_output_directory
Keep the calibrated dry/processed files unchanged. For listening compare
ID-matched-dry.wav with ID-matched.wav. Both receive the same final attenuation
when needed to retain at least 1.5 dB true-peak headroom, using measured 8x peaks.
The gain match uses each record's BS.1770 active-window loudness difference.
"""
import hashlib
import json
from pathlib import Path
import sys
import numpy as np
import soundfile as sf

def prepare(folder):
    for file in sorted((folder/'records').glob('*.json')):
        row=json.loads(file.read_text(encoding='utf-8-sig'));key=row['id']
        existing=row.get('audition',{})
        power=key.startswith('PW')
        source_output_hash=(row['checks'][1]['output_pcm_sha256'] if power else row['output_pcm_sha256'])
        if existing.get('source_output_pcm_sha256')==source_output_hash and existing.get('headroom_target_dbtp')==-1.51:
            reference=folder/'audio'/existing['reference'];matched=folder/'audio'/existing['processed']
            if hashlib.sha256(reference.read_bytes()).hexdigest()!=existing['reference_sha256'] or hashlib.sha256(matched.read_bytes()).hexdigest()!=existing['processed_sha256']:
                raise ValueError('Existing audition files changed')
            continue
        check=next(c for c in row['checks'] if c.get('variant')==0 and c.get('level_db' if power else 'level_offset_db')==0)
        reference_metrics=row['input'] if power else check['input']
        delta=reference_metrics['lufs']-check['output']['lufs']
        maximum=max(reference_metrics['true_peak_8x_dbtp'],check['output']['true_peak_8x_dbtp']+delta)
        common=min(0.,-1.51-maximum) # PCM24 rounding margin; verification still requires <= -1.5 dBTP.
        dry,rate=sf.read(folder/'audio'/f'{key}-dry.wav',always_2d=True)
        processed,other_rate=sf.read(folder/'audio'/f'{key}-processed.wav',always_2d=True)
        if rate!=48000 or other_rate!=rate or dry.shape!=processed.shape:
            raise ValueError('Audition source format mismatch')
        a=dry*10**(common/20);b=processed*10**((common+delta)/20)
        if not np.isfinite(a).all() or not np.isfinite(b).all() or max(np.max(np.abs(a)),np.max(np.abs(b)))>=1:
            raise ValueError('Audition clipping; preserve originals and inspect')
        reference=folder/'audio'/f'{key}-matched-dry.wav';matched=folder/'audio'/f'{key}-matched.wav'
        sf.write(reference,a,rate,subtype='PCM_24');sf.write(matched,b,rate,subtype='PCM_24')
        row['audition']=dict(reference=reference.name,processed=matched.name,common_attenuation_db=common,
            source_output_pcm_sha256=source_output_hash,headroom_target_dbtp=-1.51,
            processed_match_gain_db=delta,method='BS.1770 match + common gain for >=1.5 dB measured 8x headroom; PCM24',
            reference_sha256=hashlib.sha256(reference.read_bytes()).hexdigest(),processed_sha256=hashlib.sha256(matched.read_bytes()).hexdigest())
        file.write_text(json.dumps(row,indent=2)+'\n',encoding='utf-8')

if __name__=='__main__':prepare(Path(sys.argv[1]))
