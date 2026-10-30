"""Independent analysis of production ScoreEngine raw stereo captures."""
import argparse, hashlib, json
from pathlib import Path
import numpy as np

def identity(root):
    h=hashlib.sha256()
    for p in sorted((root/'Source'/'DSP').glob('*')):
        if p.is_file(): h.update(p.name.encode());h.update(p.read_bytes())
    return h.hexdigest()

def main():
    ap=argparse.ArgumentParser();ap.add_argument('directory',type=Path);ap.add_argument('report',type=Path);args=ap.parse_args()
    read=lambda name:np.fromfile(args.directory/(name+'.f32'),dtype='<f4').reshape(-1,2).astype(float)
    pin=args.directory/'dsp_source.sha256'
    result={'dsp_sha256':pin.read_text().strip() if pin.exists() else 'not captured; preliminary run', 'analysis_current_dsp_sha256':identity(Path(__file__).resolve().parents[1]),'source_directory':str(args.directory),'checks':[],'pitch':[],'space':[]}
    def check(name,value,limit,passed):result['checks'].append(dict(name=name,value=float(value),limit=limit,passed=bool(passed)))
    unity=read('unity');reference=np.zeros_like(unity);reference[9600]=[.25,.175]
    err=np.max(abs(unity-reference));check('unity impulse exact delay',err,'<=1e-6',err<=1e-6)
    err=np.max(abs(read('bands_neutral')-unity));check('neutral complementary band null',err,'<=1e-6',err<=1e-6)
    for b in range(3):
        x=read(f'band_{b}');nz=np.flatnonzero(abs(x[:,0])>1e-12);first=int(nz[0]) if len(nz) else -1
        expected=4800+2400*(b+1);check(f'band {b} independent onset error',abs(first-expected),'<=1 sample',abs(first-expected)<=1)
    for initial in [0,-12]:
        for step in [-12,12]:
            x=read(f'pitch_{initial}_{step}')[:,0]
            for turn in range(4):
                start=int((.2+1.6*turn+.25)*48000);y=x[start:start+24000];expected=440*2**((initial+turn*step)/12)
                spectrum=abs(np.fft.rfft(y*np.hanning(len(y)),n=262144));k=int(np.argmax(spectrum[1:])+1)
                z=np.log(spectrum[k-1:k+2]+1e-30);fraction=.5*(z[0]-z[2])/(z[0]-2*z[1]+z[2]);frequency=(k+fraction)*48000/262144
                cents=1200*np.log2(frequency/expected);outside=spectrum.copy();lo=int((expected-8)*262144/48000);hi=int((expected+8)*262144/48000);outside[max(0,lo):hi+1]=0
                side_db=20*np.log10((np.max(outside)+1e-30)/(spectrum[k]+1e-30))
                result['pitch'].append(dict(initial=initial,step=step,turn=turn,expected=expected,measured=frequency,cents=cents,largest_outside_8Hz_db=side_db))
                if 55<=expected<=7040:check(f'pitch initial {initial}, step {step}, turn {turn}',abs(cents),'<=5 cents',abs(cents)<=5)
    for type in range(3):
        for route in range(3):
            x=read(f'space_{type}_{route}');peak=np.max(abs(x));tail=np.sum(x[int(2*48000):]**2);late=np.sum(x[int(12*48000):]**2)
            corr=np.corrcoef(x.T)[0,1];result['space'].append(dict(type=type,route=route,peak=peak,tail_energy=tail,late_energy=late,correlation=corr))
            check(f'space {type}/{route} finite',peak,'finite and <8',np.isfinite(x).all() and peak<8)
            check(f'space {type}/{route} decays',late/(np.sum(x*x)+1e-30),'<1e-8 after 12 s',late<np.sum(x*x)*1e-8)
    x=read('action_4');check('per-reflection Skip',np.max(abs(x)),'0',np.max(abs(x))==0)
    x=read('glitch_5');check('full Omit',np.max(abs(x)),'0',np.max(abs(x))==0)
    # Independent source-address reference for the ramp, including the documented overlaps.
    n=np.arange(96000);source=(.1*(2*np.fmod(n/48000.,.05)/.05-1)).astype('<f4').astype(float)
    def at(indices):
        indices=np.asarray(indices);valid=(indices>=0)&(indices<len(source));return np.where(valid,source[np.clip(indices,0,len(source)-1)],0)
    delay=length=4800;elapsed=n-delay
    for action in [1,2,3]:
        if action==1:
            frame=(elapsed-length)//(length//2);expected=np.zeros(len(n))
            for ago in [0,1]:
                f=frame-ago;start=delay+length+f*(length//2);age=n-start;valid=(elapsed>=length)&(f>=0)&(age>=0)&(age<length)
                expected+=np.where(valid,(.5-.5*np.cos(2*np.pi*age/length))*at(start-delay-1-age),0)
        else:
            age=elapsed%length;frame=n-age-delay;position=frame-length+(age%(length//4) if action==2 else length*((age/length)-.5*(age/length)**2))
            base=np.floor(position).astype(int);f=position-base;edge=np.clip(np.minimum(age,length-1-age)/32,0,1)
            expected=np.where(elapsed>=length,edge*(at(base)+f*(at(base+1)-at(base))),0)
        error=np.max(abs(read(f'action_{action}')[:,0]-expected));check(f'fragment action {action} analytic sample reference',error,'<=1e-6',error<=1e-6)
    for note in [36,48,55,67,72,84]:
        path=args.directory/f'combined_forward_{note}.f32'
        if not path.exists():continue
        forward=read(f'combined_forward_{note}')[:,0];reverse=read(f'combined_reverse_{note}')[:,0];frame=(elapsed-length)//(length//2);expected=np.zeros(len(n))
        for ago in [0,1]:
            f=frame-ago;start=delay+length+f*(length//2);age=n-start;indices=start-1-age;valid=(elapsed>=length)&(f>=0)&(age>=0)&(age<length)&(indices>=0)&(indices<len(n))
            expected+=np.where(valid,(.5-.5*np.cos(2*np.pi*age/length))*forward[np.clip(indices,0,len(n)-1)],0)
        error=np.max(abs(reverse-expected));check(f'combined pitch/Reverse note {note}',error,'<=1e-6; fragment fixed at 100 ms',error<=1e-6)
    echo=np.r_[np.zeros(delay),source[:-delay]];age=n%length;frame=n-age;edge=np.clip(np.minimum(age,length-1-age)/32,0,1)
    for mode in [1,2,3,4]:
        if mode==1:position=np.where(n//length<1,-1,((n//length-1)//2)*2*length+age)
        elif mode==2:position=frame-1-age
        elif mode==3:position=frame-length+age%(length//4)
        else:position=frame-length+length*((age/length)-.5*(age/length)**2)
        base=np.floor(position).astype(int);fraction=position-base
        def read_echo(indices):return np.where((indices>=0)&(indices<len(n)),echo[np.clip(indices,0,len(n)-1)],0)
        expected=edge*(read_echo(base)+fraction*(read_echo(base+1)-read_echo(base)))
        error=np.max(abs(read(f'glitch_{mode}')[:,0]-expected));check(f'texture {mode} analytic fragment addressing',error,'<=1e-6',error<=1e-6)
    timing=args.directory/'timing.csv'
    if timing.exists():
        rows=np.genfromtxt(timing,delimiter=',',names=True)
        check('timing rates and tempo divisions',np.max(abs(rows['requested_samples']-rows['measured_samples'])),'<=1 sample',np.max(abs(rows['requested_samples']-rows['measured_samples']))<=1 and np.max(rows['max_error'])<=1e-6)
    freeze=args.directory/'freeze.csv'
    if freeze.exists() and sum(1 for _ in freeze.open())>=311:
        rows=np.genfromtxt(freeze,delimiter=',',names=True);energies=rows['energy'][4:64].reshape(10,6).sum(axis=1);drift=10*np.log10(max(energies)/min(energies))
        check('Freeze settled 60 s drift',drift,'<=1 dB',drift<=1)
        check('Freeze 5 min finite',max(rows['peak']),'<8 and finite',np.isfinite(rows['energy']).all() and min(rows['finite'])==1 and max(rows['peak'])<8)
        release=10*np.log10(rows['energy'][-1]/rows['energy'][303]);check('Freeze release decay after 6 s',release,'<0 dB',release<0)
    for stress in ['raw_feedback','bands_space']:
        path=args.directory/stress/'bandstress.csv'
        if path.exists():
            rows=np.genfromtxt(path,delimiter=',',names=True);ratio=np.sum(rows['energy'][-5:])/(np.sum(rows['energy'][:5])+1e-30)
            check(f'{stress} maximum-feedback impulse decays',ratio,'<1e-4 final/first 5-second energy',ratio<1e-4 and np.isfinite(rows['energy']).all())
    result['passed']=sum(c['passed'] for c in result['checks']);result['failed']=sum(not c['passed'] for c in result['checks'])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(f"{result['passed']} passed, {result['failed']} failed")
    for c in result['checks']:
        if not c['passed']:print('FAIL',c)
    return int(result['failed']>0)

if __name__=='__main__':raise SystemExit(main())
