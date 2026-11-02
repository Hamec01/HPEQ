"""S4 production laboratory: source-pinned paired renders, never relabel caches."""
import argparse
import hashlib
import json
import numpy as np
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]


def digest(p,normalized=False):
    data=p.read_bytes()
    return hashlib.sha256(data.replace(b'\r\n',b'\n') if normalized else data).hexdigest()


def identities():
    names=sorted({str(p.relative_to(ROOT)).replace('\\','/') for pattern in ['Source/DSP/V2*','Source/Standalone/*','Source/UI/ShirotaV2Panel.*'] for p in ROOT.glob(pattern) if p.is_file()}|{'Assets/Hrtf/production.bin','Assets/Hrtf/production.json'})
    files={n:digest(ROOT/n,not n.endswith('.bin')) for n in names}
    source=hashlib.sha256(('ShirotaS4ProductionV1'+''.join(files.values())).encode()).hexdigest()
    methodFiles=['Tests/IntegrationLabMain.cpp','Tools/analyze_s4.py','docs/S4_METHODS.md']
    methods={n:digest(ROOT/n,True) for n in methodFiles}
    method=hashlib.sha256(('ShirotaS4MethodV1'+''.join(methods.values())).encode()).hexdigest()
    return source,method,files,methods


def main():
    p=argparse.ArgumentParser();p.add_argument('run',type=Path);p.add_argument('report',type=Path);p.add_argument('--baseline',action='store_true');a=p.parse_args()
    m=json.loads((a.run/'manifest.json').read_text());source,method,files,methods=identities()
    assert m['implemented']!=a.baseline,'Wrong experiment role'
    assert m.get('method_id')==method,'Stale compiled laboratory method'
    assert m.get('source_id')==('d1ef423 frozen facade' if a.baseline else source),'Stale compiled production identity'
    pin=dict(methods=methods,source_files=None if a.baseline else files,manifest=digest(a.run/'manifest.json'),renders={f.name:digest(f) for f in a.run.glob('*.f32')})
    record=a.run/'identity.json'
    if record.exists():assert json.loads(record.read_text())==pin,'Corrupt or stale cached render'
    else:record.write_text(json.dumps(pin,indent=2)+'\n',encoding='utf-8',newline='\n')
    checks=[]
    def add(goal,name,result,**data):checks.append(dict(goal=goal,name=name,passed=bool(result),**data))
    rows={r['name']:r for r in m['cases']}
    for name,r in rows.items():
        add('I03',name+'_allocations',r['allocations']==0)
        add('I05',name+'_finite_PDC',r['finite'] and r['latency']==int(r['rate']*.0025+.5),peak=r['peak'],latency=r['latency'])
        add('I12',name+'_raw',digest(a.run/(name+'.f32'))==r['sha256'])
    if m['implemented']:
        for block in [64,127,512,4096]:add('I03','events_'+str(block),rows['events_'+str(block)]['sha256']==rows['events_1']['sha256'])
        for name,r in rows.items():
            if name.startswith(('neutral','bypass')):add('I02',name,r['dry_exact'])
        add('I01','parameter_count',m['parameter_count']==159,parameters=m['parameter_count'])
        signal=lambda i:np.fromfile(a.run/('timing_'+str(i)+'.f32'),dtype='<f4').reshape(-1,2).astype(float)
        for step,pre,post in [(1,2,3),(4,5,6)]:
            y,old,new=signal(step),signal(pre),signal(post)
            peak=float(np.max(np.abs(y)));bound=float(np.max(np.abs(old))+np.max(np.abs(new)))+.01
            add('I07','transition_'+str(step),peak<=bound,peak=peak,bound=bound)
        y=signal(1);start=12001+56-4800;indices=np.arange(start,start+9600);dry=.1*np.sin(997*2*np.pi*(indices-120)/48000)
        wet=y[indices,0]-.55*dry;spec=np.abs(np.fft.rfft(wet*np.hanning(len(wet))))**2;hz=np.fft.rfftfreq(len(wet),1/48000)
        outband=10*np.log10(max(float(np.sum(spec[np.abs(hz-997)>100])/np.sum(spec)),1e-30))
        add('I07','held_tone_no_sweep',outband<=-30,outside_carrier_100Hz_db=outband)
    else:checks.extend(dict(goal=g,name='not_implemented',passed=None) for g in ['I02','I03_events','I01_append'])
    bank=json.loads((ROOT/'Assets/Hrtf/production.json').read_text())
    add('I04','native_filter',bank['maximum_magnitude_error_db']<=.5 and bank['maximum_ITD_error_samples']<=1,**{k:bank[k] for k in ['maximum_magnitude_error_db','maximum_ITD_error_samples','maximum_tail_energy_loss']})
    result=dict(method_id=method,source_id=m['source_id'],implemented=m['implemented'],methods=methods,source_files=None if a.baseline else files,cases=len(rows),checks=checks,
                passed=sum(c['passed'] is True for c in checks),failed=sum(c['passed'] is False for c in checks),unavailable=sum(c['passed'] is None for c in checks),identity_sha256=digest(record))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f"S4: {result['passed']} passed, {result['failed']} failed, {result['unavailable']} n/a")
    for c in checks:
        if c['passed'] is False:print(json.dumps(c))
    return 1 if result['failed'] else 0


if __name__=='__main__':raise SystemExit(main())
