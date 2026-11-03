"""Frozen S2 numerical gates; source/method/raw identity precedes acceptance."""
# First run used49cases:200candidate checks missed the full-window fault.
# Added independent checks, not looser targets; recaptured the final same-method
# control/candidate pair. Keep all earlier reports and archived methods.
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1]


def sha(p,normal=False):
    x=p.read_bytes();return hashlib.sha256(x.replace(b'\r\n',b'\n') if normal else x).hexdigest()
def identities():
    files={n:sha(ROOT/n,True) for n in ['Source/DSP/V2Gate.cpp','Source/DSP/V2Gate.h']}
    methods={n:sha(ROOT/n,True) for n in ['Tests/V2LabMain.cpp','Tools/analyze_zastava_s2.py','docs/S2_METHODS.md']}
    return dict(source_id=hashlib.sha256(('ZastavaS2SourceV1'+''.join(files.values())).encode()).hexdigest(),
                method_id=hashlib.sha256(('ZastavaS2MethodV1'+''.join(methods.values())).encode()).hexdigest(),files=files,methods=methods)
def write(p,j):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(j,indent=2)+'\n',encoding='utf-8',newline='\n')
def db(x):return float(20*np.log10(max(float(x),1e-30)))


def main():
    parser=argparse.ArgumentParser();parser.add_argument('run',type=Path);parser.add_argument('report',type=Path);parser.add_argument('--baseline',action='store_true');args=parser.parse_args()
    identity=identities();m=json.loads((args.run/'manifest.json').read_text());assert m['implemented']!=args.baseline,'Wrong experiment role'
    assert m['method_id']==identity['method_id'],'Stale compiled method'
    if args.baseline:
        snapshot=json.loads((ROOT/'out/s2/before_sources.json').read_text());assert m['source_id']==snapshot['source_id'],'Wrong baseline source'
    else:assert m['source_id']==identity['source_id'],'Stale compiled source'
    raw={p.name:sha(p) for p in sorted(args.run.glob('*.f32'))};pin=dict(manifest=sha(args.run/'manifest.json'),raw=raw,source_id=m['source_id'],method_id=m['method_id'])
    if (args.run/'identity.json').exists():assert json.loads((args.run/'identity.json').read_text())==pin,'Corrupt cached render'
    else:write(args.run/'identity.json',pin)
    rows={r['name']:r for r in m['cases']};checks=[]
    def add(goal,name,passed,**data):checks.append(dict(goal=goal,name=name,passed=bool(passed) if passed is not None else None,**data))
    def signal(name):
        row=rows[name];return np.fromfile(args.run/(name+'.f32'),dtype='<f4').reshape(-1,row['channels'])
    for name,row in rows.items():
        add('G09',name+'_allocation',row['allocations']==0)
        add('G10',name+'_finite',row['finite'])
        add('G01',name+'_PDC',row['latency']==int(.005*row['rate']+.5))
        if name.startswith('neutral_'):
            y=signal(name);n=np.arange(row['frames']);d=row['latency'];source=np.zeros_like(y)
            for ch in range(row['channels']):source[:,ch]=(.1*np.sin(n/row['rate']*6264.335)+.03*np.sin(n/row['rate']*(8293 if ch else 3217))).astype('<f4')
            expected=np.zeros_like(y);expected[d:]=source[:-d];add('G01',name+'_dry',np.array_equal(y,expected))
    if not m['implemented']:
        for goal in ['G02','G03','G04','G05','G06','G07','G08','G09_events','G11']:add(goal,'unavailable',None)
    else:
        for i,target in [(0,0),(1,-40),(2,-10)]:
            measured=db(np.mean(np.abs(signal('static_'+str(i))[24000:,0]))/.1);add('G02','static_'+str(i),abs(measured-target)<=.05,measured_db=measured,target_db=target)
        for name,target in [('knee_0',-10),('knee_1',-1.5),('knee_2',0),('rms',-9.030899869919436)]:
            measured=db(np.mean(np.abs(signal(name)[24000:,0]))/.1);add('G02',name,abs(measured-target)<=.05,measured_db=measured,target_db=target)
        floor=.01
        for profile in range(2):
            y=signal('timing_'+str(profile))[:,0].astype(float)/.1;openAt=4800+240;closeAt=9600+480-1+240
            rise=np.flatnonzero(y[openAt:]>=(floor+.9*(1-floor)))[0];fall=np.flatnonzero(y[closeAt:]<=(floor+.1*(1-floor)))[0]
            add('G03','attack_'+str(profile),abs(int(rise)-192)<=max(2,.03*192),samples=int(rise),target=192)
            add('G03','release_'+str(profile),abs(int(fall)-960)<=max(2,.03*960),samples=int(fall),target=960)
            hold=int(np.flatnonzero(y[9600+240:]<1-1e-6)[0]);add('G03','hold_'+str(profile),abs(hold-479)<=2,samples=hold,target=479)
        for mode in range(2):
            for profile in range(2):
                a=signal(f'window_{mode}_{profile}_0')[10560:24000];b=signal(f'window_{mode}_{profile}_1')[10560:24000]
                add('G04',f'window_release_{mode}_{profile}',np.array_equal(a,b),maximum_difference=float(np.max(np.abs(a-b))))
        for profile in range(2):
            a=rows[f'look_{profile}_0']['peak'];b=rows[f'look_{profile}_1']['peak'];add('G04','look_'+str(profile),db(b)>=-.1 and db(b/max(a,1e-30))>=15,peak_db=db(b),improvement_db=db(b/max(a,1e-30)))
            hits=rows[f'drum_{profile}_0']['hits'];add('G05','hits_'+str(profile),hits==8,hits=hits)
            y=signal(f'drum_{profile}_1')[:,0].astype(float)/.1;end=4800+240+48+720+1920
            add('G05','tail_end_'+str(profile),abs(y[end]-.01)<=1e-6 and abs(y[end-96]-.01)>1e-6,gain=float(y[end]))
        a=signal('drum_0_1')[4800+240+48+720+480,0];b=signal('drum_1_1')[4800+240+48+720+480,0];add('G05','soft_tail_retention',a>b,soft=float(a),hard=float(b))
        for i,target in [(0,-40),(1,0)]:
            value=db(np.mean(np.abs(signal('duck_'+str(i))[24000:,0]))/.1);add('G06','duck_'+str(i),abs(value-target)<=.05,measured_db=value)
        for name,row in rows.items():
            if name.startswith('neutral_') and name.endswith('_3'):add('G06',name+'_missing_status',row['missing_key'])
        good=db(np.mean(np.abs(signal('filter_0')[24000:,0]))/.1);bad=db(np.mean(np.abs(signal('filter_1')[24000:,0]))/.1);add('G08','key_filter',good-bad>=30,pass_db=good,stop_db=bad)
        bad=db(np.mean(np.abs(signal('filter_hp')[24000:,0]))/.1);add('G08','key_highpass',good-bad>=30,pass_db=good,stop_db=bad)
        independent=signal('link_0')[24000:].astype(float);linked=signal('link_1')[24000:];add('G08','independent',db(np.mean(np.abs(independent[:,0]))/np.mean(np.abs(independent[:,1])))>=39.95);add('G08','linked',np.array_equal(linked[:,0],linked[:,1]))
        for size in [16,32]:
            step=6000 if size==16 else 3000
            for sync in range(2):
                y=signal(f'rhythm_{size}_{sync}')[:,0].astype(float)/.1
                levels=[y[240+i*step+step//2] for i in range(size)]
                error=max(abs(db(value)-(-40 if i%2==0 else 0)) for i,value in enumerate(levels));add('G07',f'rhythm_{size}_{sync}',error<=.05,error_db=error)
                opening=240+step;cross=int(np.flatnonzero(y[opening:]>=floor+.9*(1-floor))[0]);add('G07',f'grid_{size}_{sync}',abs(cross-47)<=1,error_samples=cross-47)
        for t,target in [(1.46,.505),(1.55,1.)]:
            n=np.arange(round(t*48000),round((t+.005)*48000));source=(.1*np.sin(n/48000*6264.335)+.03*np.sin(n/48000*3217)).astype('<f4')
            actual=signal('events_512')[n+240,0];mask=np.abs(source)>.01;gain=float(np.mean(actual[mask]/source[mask]));error=abs(db(gain/target));add('G07','seek_tempo_'+str(t),error<=.05,gain=gain,target=target,error_db=error)
        for block in [64,127,512,4096]:add('G09','events_'+str(block),raw['events_1.f32']==raw[f'events_{block}.f32'])
        for kind in range(4):
            y=signal('recovery_'+str(kind))[:,0].astype(float)/.1
            tests=[(int(.15*48000),.01),(int(.3*48000),.01 if kind==3 else 1),(int(.5*48000),.01 if kind==2 else 1)]
            error=max(abs(db(y[n]/target)) for n,target in tests);add('G10','recovery_'+str(kind),error<=.05,error_db=error)
        y=signal('recovery_3');add('G10','reset_latency',np.all(y[28800:29040]==0) and abs(float(y[29040,0])-.1)<1e-6)
        add('G06','key_loss_status',rows['recovery_0']['missing_seen'] and not rows['recovery_0']['missing_key'])
        add('G10','stress_bound',rows['stress']['peak']<=.52,peak=rows['stress']['peak'])
        measured=max(r['mean_fraction'] for r in rows.values() if r['name'] not in ['stress','events_1']);add('G11','mean_CPU',measured<=.25,worst_mean_fraction=measured)
        performance={name:{key:row[key] for key in ['mean_fraction','maximum_wall_fraction','cycles_at_maximum_wall','median_cycles','maximum_cycles']} for name,row in rows.items()}
    result=dict(identity=identity,source_id=m['source_id'],implemented=m['implemented'],checks=checks,cases=len(rows),passed=sum(c['passed'] is True for c in checks),failed=sum(c['passed'] is False for c in checks),unavailable=sum(c['passed'] is None for c in checks),identity_sha256=sha(args.run/'identity.json'))
    if m['implemented']:result['performance']=performance
    write(args.report,result);print('S2:',result['passed'],'passed,',result['failed'],'failed,',result['unavailable'],'n/a')
    for c in checks:
        if c['passed'] is False:print(json.dumps(c))
    return 1 if result['failed'] else 0


if __name__=='__main__':raise SystemExit(main())
