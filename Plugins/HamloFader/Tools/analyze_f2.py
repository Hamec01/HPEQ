"""Paired F2 oracle: source/method/raw identity precedes numerical acceptance."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from scipy import signal
ROOT=Path(__file__).resolve().parents[1]
METHOD=['Tests/F2LabMain.cpp','Tools/analyze_f2.py','Source/DSP/FaderKernel.h','Source/DSP/FaderMeter.h','docs/HAMLOFADER_V2.md']
def sha(p,normal=False):
 b=p.read_bytes();return hashlib.sha256(b.replace(b'\r\n',b'\n') if normal else b).hexdigest()
def identity(prefix,files):return hashlib.sha256((prefix+''.join(sha(p,True) for p in files)).encode()).hexdigest()
def write(p,x):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(x,indent=2)+'\n',encoding='utf-8',newline='\n')
def db(x):return float(20*np.log10(max(float(x),1e-30)))
def rms(x):return db(np.sqrt(np.mean(np.asarray(x,dtype=float)**2)))
def coeffs(fs):
 if fs==48000:return [(np.array([1.53512485958697,-2.69169618940638,1.19839281085285]),np.array([1,-1.69065929318241,.73248077421585])),(np.array([1,-2,1]),np.array([1,-1.99004745483398,.99007225036621]))]
 # Bilinear remap of the published 48k digital filters via their analog poles/zeros.
 result=[]
 for b,a in coeffs(48000):
  z,p,k=signal.tf2zpk(b,a);za=96000*(z-1)/(z+1);pa=96000*(p-1)/(p+1);ka=k*np.real(np.prod(96000-pa)/np.prod(96000-za));zd,pd,kd=signal.bilinear_zpk(za,pa,ka,fs);bb,aa=signal.zpk2tf(zd,pd,kd);result.append((bb.real,aa.real))
 return result
TP=np.array([[.001708984375,-.0291748046875,-.0189208984375,-.00830078125],[.010986328125,.029296875,.0330810546875,.014892578125],[-.0196533203125,-.0517578125,-.0582275390625,-.026611328125],[.033203125,.089111328125,.1015625,.047607421875],[-.0594482421875,-.16650390625,-.2003173828125,-.102294921875],[.1373291015625,.465087890625,.77978515625,.97216796875],[.97216796875,.77978515625,.465087890625,.1373291015625],[-.102294921875,-.2003173828125,-.16650390625,-.0594482421875],[.047607421875,.1015625,.089111328125,.033203125],[-.026611328125,-.0582275390625,-.0517578125,-.0196533203125],[.014892578125,.0330810546875,.029296875,.010986328125],[-.00830078125,-.0189208984375,-.0291748046875,.001708984375]])
def oracle(x,fs):
 x=x.astype(float);k=x.copy()
 for b,a in coeffs(fs):k=signal.lfilter(b,a,k,axis=0)
 e=np.sum(k*k,axis=1);n=len(x);m=round(.4*fs);s=round(3*fs);hop=round(.1*fs)
 loud=lambda q:float(-.691+10*np.log10(max(float(np.mean(q)),1e-30)))
 blocks=np.array([np.mean(e[i:i+m]) for i in range(0,n-m+1,hop)]);absolute=blocks[blocks>10**((-70+.691)/10)]
 integrated=-120
 if len(absolute):
  gated=absolute[absolute>np.mean(absolute)*.1]
  if len(gated):integrated=loud(gated)
 tp=max(np.max(np.abs(x)),np.max(np.abs(signal.upfirdn(TP.ravel(),x,up=4,axis=0))))
 return dict(rmsM=rms(x[-m:]),rmsS=rms(x[-s:]),rmsI=rms(x),lufsM=loud(e[-m:]),lufsS=loud(e[-s:]),lufsI=integrated,samplePeak=db(np.max(np.abs(x))),truePeak=db(tp))
def main():
 p=argparse.ArgumentParser();p.add_argument('run',type=Path);p.add_argument('report',type=Path);args=p.parse_args();run=args.run;m=json.loads((run/'manifest.json').read_text());method=identity('FaderF2MethodV1',[ROOT/n for n in METHOD]);assert m['method_id']==method,'Stale method'
 files=['Source/DSP/FaderKernel.h','Source/DSP/FaderMeter.h','Source/DSP/FaderKernel.cpp','Source/DSP/FaderMeter.cpp']
 if m['implemented']:assert m['source_id']==identity('FaderF2KernelV1',[ROOT/n for n in files]),'Stale kernel'
 else:assert m['source_id']==identity('FaderF2ReferenceV1',[p for p in sorted((ROOT/'out/f2/baseline/Source').rglob('*')) if p.is_file()]),'Stale control'
 pins=dict(source_id=m['source_id'],method_id=method,manifest=sha(run/'manifest.json'),raw={p.name:sha(p) for p in sorted(run.glob('*.f32'))})
 if (run/'identity.json').exists():assert json.loads((run/'identity.json').read_text())==pins,'Corrupt cached output'
 else:write(run/'identity.json',pins)
 checks=[];rows={r['name']:r for r in m['cases']}
 def add(goal,name,passed,**v):checks.append(dict(goal=goal,name=name,passed=None if passed is None else bool(passed),**v))
 def audio(name,source=False):
  r=rows[name];return np.fromfile(run/(name+('_input' if source else '')+'.f32'),dtype='<f4').reshape(-1,r['channels'])
 for name,r in rows.items():
  if not r['available']:continue
  add('K12',name+'_finite',r['finite']);add('K12',name+'_alloc',r['allocations']==0)
  if name.startswith('neutral_'):add('K01',name,np.array_equal(audio(name),audio(name,True)))
 if not m['implemented']:
  for i in range(2,15):add(f'K{i:02}','new_feature',None)
 else:
  for name in ['disabled','bypass','mix_zero']:add('K01',name,np.array_equal(audio(name),audio(name,True)))
  for gain in [-60,-12,6,24]:
   name='gain_'+str(gain);x=audio(name,True)[48000:];y=audio(name)[48000:];error=abs(rms(y)-rms(x)-gain);add('K02',name,error<=.001,error_db=error)
  for mode in range(4):
   name=f'routing_{mode}';x=audio(name,True);expected=x.copy()
   if mode==1:expected[:]=np.mean(x.astype(float),axis=1)[:,None]
   if mode==2:expected[:]=x[:,0,None]
   if mode==3:expected[:]=x[:,1,None]
   for c in range(2):
    if mode&(1<<c):expected[:,c]*=-1
   error=float(np.max(np.abs(audio(name)-expected)));add('K02',name,error<=1e-6,error=error)
  add('K02','swap',np.array_equal(audio('swap'),audio('swap',True)[:,::-1]))
  x=audio('delay',True);y=audio('delay');expected=np.zeros_like(x);expected[96:,0]=x[:-96,0];expected[6:,1]=x[:-6,1];add('K02','integer_alignment',np.array_equal(y,expected))
  x=audio('fractional',True);expected=np.zeros_like(x);expected[1:,0]+=.5*x[:-1,0];expected[2:,0]+=.5*x[:-2,0];expected[2:,1]+=.5*x[:-2,1];expected[3:,1]+=.5*x[:-3,1];add('K02','fractional_alignment',np.max(np.abs(audio('fractional')-expected))<=1e-6)
  x=audio('channel_gain',True);y=audio('channel_gain');error=max(abs(rms(y[:,c])-rms(x[:,c])-g) for c,g in enumerate([-6,6]));add('K02','channel_trim',error<=.001,error_db=error)
  x=audio('gain_ramp',True)[:,0];y=audio('gain_ramp')[:,0];at=12000;n=960;target=10**(6/20);first=(y[at]/x[at]-1)/(target-1);add('K03','first_sample',first<=1/n+1e-6 and first>0,step=float(first));add('K03','endpoint',abs(float(y[at+n-1]/x[at+n-1])-target)<1e-6)
  tp_delta=[]
  for name,r in rows.items():
   if not name.startswith(('meter_','neutral_')) or r['frames']<r['rate']*.4:continue
   got=r['readings'][-1]['input'];want=oracle(audio(name,True),r['rate'])
   for field in ['rmsM','rmsS','rmsI','lufsM','lufsS','lufsI','samplePeak','truePeak']:
    error=abs(got[field]-want[field]);limit=.1 if field.startswith('lufs') else .001;add('K05' if field=='truePeak' else 'K04',name+'_'+field,error<=limit,error=error)
   if r['rate']==48000:tp_delta.append(dict(name=name,sinc16_dbtp=db(np.max(np.abs(signal.resample_poly(audio(name,True),16,1,axis=0)))),annex_dbtp=got['truePeak']))
  got=rows['meter_5']['readings'][-1]['input'];add('K05','intersample_detected',got['truePeak']-got['samplePeak']>2,excess_db=got['truePeak']-got['samplePeak'])
  mono=rows['meter_mono']['readings'][-1]['input']['lufsI'];stereo=rows['meter_0']['readings'][-1]['input']['lufsI'];add('K04','mono_calibration',abs(mono+23)<=.1,lufs=mono);add('K04','stereo_sum',abs(stereo-mono-10*np.log10(2))<=.001,difference=stereo-mono)
  for name,r in rows.items():
   if not name.startswith('match_'):continue
   q=r['proposal'][0];add('K06',name+'_valid',q['valid'])
   x=audio(name,True);y=audio(name);g=q['gainDb'];fs=round(r['rate']);source=x[:4*fs];o=oracle(source,fs);measure='lufsI' if name.endswith('_1') else 'rmsI';target=0 if name=='match_guard' else -18;error=abs(o[measure]+g-target)
   if name!='match_guard':add('K06',name+'_target',error<=.1,error=error)
   else:add('K07','peak_guard',q['limited'] and q['predictedPeak']<=-1+.02,predicted=q['predictedPeak'])
   ratio=rms(y[4*fs+4800:])-rms(x[4*fs+4800:]);add('K06',name+'_held_gain',abs(ratio-g)<=.001,error=abs(ratio-g))
  for name in ['learn_silence','learn_short']:add('K06',name,not rows[name]['proposal'][0]['valid'])
  for profile in [0,1]:
   name=f'ride_{profile}';x=audio(name,True);y=audio(name);fs=48000;levels=[rms(y[round((i+1.25)*fs):round((i+1.9)*fs)]) for i in range(0,16,2)];original=[rms(x[round((i+1.25)*fs):round((i+1.9)*fs)]) for i in range(0,16,2)];reduction=1-np.ptp(levels)/np.ptp(original);add('K08',name+'_variation',reduction>=.5,reduction=float(reduction));add('K08',name+'_target',max(abs(v+26) for v in levels)<=.2,worst=float(max(abs(v+26) for v in levels)))
   readings=rows[name]['readings'];add('K08',name+'_range',all(-12-1e-9<=r['correctionDb']<=6+1e-9 for r in readings))
   name=f'pause_{profile}';y=audio(name);x=audio(name,True);start=round(2.58*fs);loss=rms(y[start:start+4800])-rms(x[start:start+4800]);add('K09',name,loss<=.1 and not rows[name]['readings'][-1]['active'],remaining_db=loss)
  a=rows['pause_0']['readings'][0]['correctionDb'];b=rows['pause_1']['readings'][0]['correctionDb'];add('K08','profiles_distinct',abs(a-b)>.05,difference=abs(a-b))
  for name in ['motion_custom','motion_sine','motion_triangle']:
   y=audio(name)[:,0];gain=20*np.log10(np.maximum(np.abs(y[48000:]/.1),1e-30));add('K10',name+'_bounds',gain.min()>=-24-.001 and gain.max()<=.001);add('K10',name+'_depth',abs(gain.min()+24)<=.05 and abs(gain.max())<=.05,min_db=float(gain.min()),max_db=float(gain.max()));add('K10',name+'_period',np.array_equal(y[48000:72000],y[72000:96000]))
  r=rows['motion_transport'];last=r['readings'];phase_errors=[]
  for item in last:
   t=item['sample']/48000
   if t in [3,3.25,3.5]:continue
   ppq=t*2 if t<=2 else 4+(t-2)*2.9
   if t>2.5:ppq+=3.25
   error=abs((item['phase']-ppq+.5)%1-.5);phase_errors.append(error)
  add('K10','transport_phase',max(phase_errors)<=174/60/48000,error_cycles=max(phase_errors));y=audio('motion_transport');add('K10','stopped_unity',np.max(np.abs(y[round(3.1*48000):round(3.49*48000)]-.1))<1e-6)
  reference=audio('events_1');r0=rows['events_1']['readings']
  for block in [64,127,512,4096]:add('K11','events_'+str(block),np.array_equal(audio('events_'+str(block)),reference) and rows['events_'+str(block)]['readings']==r0)
  add('K12','invalid_input_counter',rows['invalid']['readings'][-1]['badInput']>0)
  perf={name:r['mean_fraction'] for name,r in rows.items() if r['block']==256 and name!='stress'};add('K13','mean_CPU',max(perf.values())<=.25,worst=max(perf.values()))
 result=dict(implemented=m['implemented'],source_id=m['source_id'],method_id=method,checks=checks,passed=sum(c['passed'] is True for c in checks),failed=sum(c['passed'] is False for c in checks),unavailable=sum(c['passed'] is None for c in checks),raw_identity_sha256=sha(run/'identity.json'))
 if m['implemented']:result.update(tp_vs_sinc=tp_delta,performance=perf)
 write(args.report,result);print('F2',result['passed'],'passed,',result['failed'],'failed,',result['unavailable'],'n/a')
 for c in checks:
  if c['passed'] is False:print(json.dumps(c))
 return 1 if result['failed'] else 0
if __name__=='__main__':raise SystemExit(main())
