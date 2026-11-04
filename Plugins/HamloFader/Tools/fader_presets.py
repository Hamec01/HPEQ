"""Build the owned Fader musical recipes; optionally apply identity-bound calibration."""
import argparse
import json
import hashlib
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
GENRES=[('bb','Boom Bap','BB',90,['Classic','Dusty','Jazzy','Gold','Underground','Lofi']),
        ('trap','Trap','TRAP',140,['ATL','Dark','Cloud','Rage','Memphis','Luxury']),
        ('techno','Techno','TECHNO',132,['Peak','Hypnotic','Minimal','Detroit','Dub','Acid']),
        ('dnb','DnB','DNB',174,['Modern','Roller','Liquid','Neuro','Jump-Up','Break'])]
# Every row is a musical application, not a set of Mix/Output variants.
GROUPS=[('Kick','kick',[2,3,1,2,3,1],['Body','Pocket','Reference','Soft Catch','Skip','Anchor']),
        ('Snare / Clap','snare',[2,2,3,1,2,3],['Ghosts','Backbeat','Broken','Reference','Tail','Turnaround']),
        ('Hats / Cymbals','hihat',[3,2,3,2,1],['Shuffle','Silk','Stutter','Tame','Reference']),
        ('Percussion','perc',[3,3,2,3],['Weave','Cross Step','Control','Scatter']),
        ('Drum Bus','drumbus1',[2,3,2,3,1,2,3],['Glue Level','Pump','Natural','Push Pull','Reference','Fast Catch','Break']),
        ('808 / Bass','bass1',[2,3,1,3,2],['Sustain','Kick Space','Reference','Pulse','Low Control']),
        ('Vocals','vocal_lead1',[2,2,1,2,3],['Natural','Forward','Reference','Fast Words','Phrase Pulse']),
        ('Keys / Piano','keys2',[3,2,3,1],['Sway','Level','Arc','Reference']),
        ('Samples / Melody','keys',[2,3,1,3],['Level','Chop','Reference','Orbit']),
        ('Master','mixdown_boom_bap_3',[1,2,1,3],['RMS Reference','Soft Ride','LUFS Reference','Rhythmic Cut'])]
PATTERNS=[
    [[.08,.2,.62,.96,1,.84,.65,.88,.18,.32,.8,1,.9,.66,.45,.95],
     [1,.65,.3,.12,.85,1,.7,.25,1,.5,.18,.86,.7,.3,1,.6],
     [.1,.15,.2,.9,.2,.7,1,.25,.1,.2,.95,.4,.8,.3,.6,1],
     [.15,.3,.55,.9,1,.9,.7,.5,.15,.35,.7,1,.8,.55,.3,.75]],
    [[.06,.12,.18,.3,.65,1,.4,.12,.06,.2,.75,1,.3,.1,.65,.25],
     [1,.12,.12,.75,.2,.2,1,.08,.85,.08,.65,.08,.95,.15,.4,.1],
     [.08,.08,.75,.75,.12,.9,.1,1,.08,.65,.08,.65,.1,1,.1,.25],
     [.08,.12,.2,.35,.6,.9,1,.5,.08,.15,.35,.8,1,.55,.25,.1]],
    [[.03,.13,.4,.82,1,.9,.6,.3,.03,.14,.5,.9,1,.82,.5,.2],
     [.1,.3,.7,1,.1,.3,.7,1,.1,.3,.7,1,.1,.3,.7,1],
     [.1,.7,.1,1,.15,.8,.2,.65,.1,1,.1,.75,.2,.6,.1,.9],
     [.05,.15,.3,.5,.7,.9,1,.8,.6,.4,.2,.1,.3,.55,.85,1]],
    [[.08,.6,1,.2,.85,.15,.35,.95,.1,.7,.2,1,.1,.9,.45,.2],
     [1,.12,.4,.85,.08,.65,1,.25,.15,.9,.35,.1,1,.2,.75,.1],
     [.1,.7,.2,.85,.12,.9,.2,1,.08,.85,.1,.6,.1,.95,.3,.75],
     [.1,.2,.8,.3,1,.15,.65,.1,.75,.25,.95,.15,.45,.9,.2,.7]]]

def recipes():
    result=[]
    for g,(code,genre,prefix,bpm,styles) in enumerate(GENRES):
        count=0;roles={1:0,2:0,3:0};beat=60000/bpm
        for group_index,(group,material,modes,moves) in enumerate(GROUPS):
            for j,(mode,move) in enumerate(zip(modes,moves)):
                depth=['Gentle','Medium','Heavy'][(j+group_index+g)%3]
                goal={'Gentle':2.,'Medium':4.5,'Heavy':9.}[depth]+g*.1+j*.025
                source=material
                if group=='808 / Bass' and (j+g)%2:source='bass2'
                if group=='Vocals':source=['vocal_lead1','vocal_lead2','vocal_lead3'][g%3]
                if group=='Master':source=['mixdown_boom_bap_3','mixdown_sad_trap','mixdown_Ardnold','mixdown_Ardnold'][g]
                instrument=group.split(' / ')[0]
                values={'engine':1,'mode':mode,'enabled':1,'inputDb':0,'gainDb':0,'outputDb':0,'mix':1,'matchedDb':0}
                role={1:'Match',2:'Ride',3:'Motion'}[mode]
                if mode==1:
                    measure=int(group in ('Vocals','Keys / Piano','Samples / Melody','Master') and (g+j)%2==1)
                    if group=='Master':measure=int(j==2)
                    values.update(measure=measure,targetDb=-18-goal,matchBoost=3+g+j*.25,matchCut=18+g+j,matchCeiling=-1.2-g*.1,peakGuard=1)
                    purpose=('Integrated LUFS' if measure else 'RMS')+' reference; Learn -> Stop -> Apply; no gain applied on load'
                elif mode==2:
                    fast=group in ('Kick','Snare / Clap','Hats / Cymbals','Percussion') or j%3==2
                    values.update(profile=int(fast),strength=.7+.05*((g+j)%6),maxBoost=3 if depth=='Gentle' else 4.5 if depth=='Medium' else 6,
                        maxCut=12 if depth=='Gentle' else 18 if depth=='Medium' else 24,floorDb=-66+g+j*.7,
                        holdMs=beat*(.06+.018*j),idleMs=beat*(.22+.02*g),riseMs=beat*((.02+.009*j) if fast else (.25+.03*j)),
                        fallMs=beat*((.01+.004*j) if fast else (.12+.02*j)),targetDb=-18-goal)
                    if group=='Keys / Piano' and depth=='Heavy':
                        # First run: BB/DnB Heavy Keys could only reach the 6 dB
                        # boost cap, just below the Heavy band. Use a firmer
                        # Natural cut response; keep the band and gain caps.
                        values.update(strength=1,riseMs=beat*.18,fallMs=beat*.07)
                        if genre=='DnB':
                            # The second run still capped at 6 dB on DnB Keys.
                            # A Fast detector follows these short piano attacks.
                            fast=True
                            values.update(profile=1,riseMs=beat*.035,fallMs=beat*.014)
                            # This sparse keys source is -30 dBFS RMS after the
                            # common headroom preparation. Response-only edits
                            # still could not reach Heavy at target >= -40.
                            # Stage the detector up, compensate manual gain and
                            # shift the activity floor equally. Net fixed gain
                            # remains unity; this extends useful detector range.
                            values.update(inputDb=12,gainDb=-12,floorDb=values['floorDb']+12)
                    purpose=('Fast linked' if fast else 'Natural linked')+' ride; retain quiet detail with an activity floor and bounded correction'
                    if values['inputDb']:
                        purpose+='; detector input +12 dB, compensated Gain -12 dB'
                else:
                    pattern=PATTERNS[g][(group_index+j)%4]
                    shift=(j*3+group_index+g)%16
                    levels=[pattern[(i+shift)%16] for i in range(16)]
                    # Different phrases stretch a beat, alternate a bar or leave a longer arc.
                    beats=[1,2,4,.5][(j+group_index)%4]
                    shape=1 if move in ('Arc','Orbit') else 2 if move=='Rhythmic Cut' and g%2 else 0
                    free=group in ('Keys / Piano','Samples / Melody') and j==2
                    values.update(shape=shape,points=16,smooth=int((j+g)%3!=1),sync=int(not free),depthDb=12,beats=beats,
                        freeMs=beat*beats,phase=(g*.07+j*.03125)%1,rightPhase=.5 if group in ('Percussion','Keys / Piano','Samples / Melody') and j%2 else 0,
                        edgeMs=2+g+j*.65)
                    values.update({f'level{i:02}':v for i,v in enumerate(levels)})
                    purpose=('Free texture' if free else f'{beats:g}-beat phrase')+'; rhythmic attenuation with '+('alternating stereo motion' if values['rightPhase'] else 'linked stereo level')
                count+=1;roles[mode]+=1
                result.append(dict(id=f'fader-{code}-{count:02}',name=f'{prefix} {styles[(group_index+j)%6]} {instrument} {move}',
                    genre=genre,group=group,note=f'{role}: {purpose}; {bpm} BPM',material=source,depth=depth,bpm=bpm,goalDb=goal,values=values))
        assert count==50 and roles=={1:11,2:19,3:20},roles
    assert len(result)==200 and len({r['id'] for r in result})==200 and len({r['name'] for r in result})==200
    return result

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--calibration',type=Path);args=parser.parse_args()
    bank=recipes()
    if args.calibration:
        data=json.loads(args.calibration.read_text(encoding='utf-8-sig'))
        assert data['passed']==data['total']==200
        assert data['identity']['mode']=='calibrate'
        def normalized_sha(path):return hashlib.sha256(path.read_bytes().replace(b'\r\n',b'\n')).hexdigest()
        dsp_files=sorted(p for p in (ROOT/'Source/DSP').rglob('*') if p.is_file())
        dsp='FaderPresetDSPV1'+''.join(normalized_sha(p) for p in dsp_files)
        for name in ('Source/Standalone/ModulePluginProcessor.cpp','Source/Standalone/ModulePluginProcessor.h','Source/Standalone/FaderHostControls.cpp','Source/Standalone/FaderHostControls.h','Source/Standalone/ModuleVariant.cpp'):
            dsp+=normalized_sha(ROOT/name)
        method='FaderPresetMethodV1'+''.join(normalized_sha(ROOT/name) for name in ('Tests/F5LibraryTests.cpp','docs/F5_PRESETS.md'))
        assert data['identity']['dsp_sha256']==hashlib.sha256(dsp.encode()).hexdigest(),'Calibration belongs to another DSP'
        assert data['identity']['method_sha256']==hashlib.sha256(method.encode()).hexdigest(),'Calibration method changed'
        source_bank=json.loads(args.calibration.with_name('uncalibrated.json').read_text(encoding='utf-8-sig'))
        assert source_bank==bank,'Calibration belongs to different recipes'
        for material in data['identity']['materials'].values():
            assert hashlib.sha256(Path(material['source']).read_bytes()).hexdigest()==material['source_sha256'],'Calibration material changed'
        calibrated={r['id']:r for r in data['recipes']}
        assert set(calibrated)=={r['id'] for r in bank}
        for recipe in bank:
            row=calibrated[recipe['id']]
            assert row['passed'] and row['name']==recipe['name'] and row['depth']==recipe['depth']
            assert abs(row['goal_db']-recipe['goalDb'])<1e-6
            expected='depthDb' if recipe['values']['mode']==3 else 'targetDb'
            assert row['parameter']==expected
            recipe['values'][expected]=row['value']
    def quote(x):return json.dumps(x,ensure_ascii=True)
    lines=['// Owned Fader genre recipes. Generated by Tools/fader_presets.py.']
    for recipe in bank:
        values=', '.join('{ '+quote(k)+', '+format(v,'.12g')+' }' for k,v in recipe['values'].items() if k!='mix')
        fields=', '.join(quote(recipe[k]) for k in ('id','genre','group','note','material','depth'))
        lines.append('{ '+quote(recipe['name'])+', 1.0f, { '+values+' }, '+fields+', '+str(recipe['bpm'])+', '+str(recipe['goalDb'])+' },')
    (ROOT/'Source/Core/FaderGenrePresets.inc').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    (ROOT/'Tools/fader_presets.json').write_text(json.dumps(bank,indent=2)+'\n',encoding='utf-8')
    print('Generated200 owned presets'+(' with measured calibration' if args.calibration else ' (uncalibrated recipes)'))

if __name__=='__main__':main()
