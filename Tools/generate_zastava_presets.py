"""Curated intent templates, genre timing/patterns, and deterministic C++ export."""
import argparse
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
JSON=ROOT/'Source/Presets/ZastavaRecipes.json'
HEADER=ROOT/'Source/Presets/GeneratedZastavaPresets.h'
GENRES=[('bb','BB','Boom Bap',88,1.10),('tr','TR','Trap',144,.85),('tc','TC','Techno',132,.70),('db','DB','DnB',174,.55)]
# name, material, depth, profile, expansion, attack, hold, release, ratio, HP, LP
CLEAN=[
('Close Voice','vocal_lead1','Gentle',0,1,2,25,110,2,90,0),
('Breath Keeper','vocal_lead2','Gentle',0,1,1.2,50,220,1.6,70,0),
('Verse Cuts','vocal_lead3','Strong',1,0,.6,30,90,4,100,12000),
('Backing Pocket','vocal_back1','Medium',0,0,1.2,35,120,3,110,11000),
('Stack Hush','vocal_back2','Strong',1,1,1,30,100,3.5,140,10000),
('Airy Harmony','vocal_back3','Gentle',0,1,3,70,200,1.8,60,0),
('Guitar Stops','guitar','Strong',1,0,.5,30,70,4,65,6000),
('String Decay','guitar','Gentle',0,1,2,70,240,2,45,8000),
('Key Room','keys','Medium',0,1,1.5,40,180,2.5,75,9000),
('Chop Floor','keys2','Strong',1,0,.3,10,50,4,130,7000),
('Bass Pause','bass1','Medium',0,1,3,30,120,2.4,35,3000),
('Sub Reset','bass2','Strong',1,0,2,25,100,4,20,1000),
('Brush Bus','drumbus1','Gentle',0,1,1,20,70,1.8,80,13000),
('Tight Room','drumbus1','Strong',1,0,.4,15,60,4,180,10000),
('Silk Lead','vocal_lead1','Medium',0,1,2,50,180,2.2,85,14000),
('Held Sample','keys2','Medium',0,0,1.5,60,210,4,55,7500)]
# name,material,depth,profile,attack,body,tail,HP,LP
DRUM=[
('Kick Frame','kick','Medium',1,.25,24,100,30,2400),('Kick Cushion','kick','Gentle',0,.7,32,150,25,1800),
('Kick Staccato','kick','Strong',1,.15,10,55,40,2000),('Sub Body','kick','Medium',0,1,40,180,20,1300),
('Snare Rim','snare','Strong',1,.2,16,75,160,14000),('Snare Velvet','snare','Gentle',0,.6,40,180,120,10000),
('Snare Pocket','snare','Medium',0,.4,28,120,180,12000),('Clap Snap','snare','Strong',1,.15,22,95,250,15000),
('Hat Lace','hihat','Gentle',0,.35,10,50,1200,0),('Hat Blips','hihat','Strong',1,.15,5,28,1800,16000),
('Hat Dust','hihat','Medium',0,.5,16,65,800,13000),('Perc Flecks','perc','Strong',1,.2,9,45,350,11000),
('Perc Swing','perc','Medium',0,.6,24,100,150,9000),('Perc Bloom','perc','Gentle',0,.9,36,160,100,14000)]
DUCK=[('Bass Step','bass1','Medium',1,2,15,.30,40,2200),('Sub Handoff','bass2','Strong',0,3,22,.38,30,1500),
('Keys Breathe','keys','Gentle',0,5,12,.42,55,3000),('Sample Lift','keys2','Medium',0,3.5,18,.34,70,2500),
('Synth Pump','synth','Strong',1,1.2,16,.25,90,4000),('Soft Pad Wake','synth','Gentle',0,7,25,.45,45,2000),
('Voice Space','vocal_lead1','Gentle',0,4,14,.32,100,3500),('Stack Dip','vocal_back1','Medium',1,2.5,20,.28,80,2800)]
RHYTHM=[('Quarter Posts','synth'),('Offbeat Mesh','keys'),('Broken Glass','keys2'),('Triplet Hint','synth'),
('Voice Chops','vocal_lead1'),('Half Windows','synth'),('Double Teeth','keys'),('Late Answer','vocal_back1'),
('Small Echoes','keys2'),('Long Lanes','synth'),('Syllable Lace','vocal_lead2'),('Open Weave','synth')]
PATTERNS=[
[1,0,0,0,1,0,0,0,1,0,0,0,1,0,.5,0], [0,0,1,0,0,.5,1,0,0,0,1,0,.25,0,1,0],
[1,0,.5,1,0,0,1,0,.25,1,0,.5,1,0,0,0], [1,0,.5,0,0,1,0,.5,0,0,1,0,.5,0,0,1],
[1,.5,0,0,1,0,.25,0,1,.5,0,1,0,0,.5,0], [1,1,0,0,1,1,0,0,1,1,0,.5,1,0,0,0],
[1,0,1,0,.5,0,1,0,1,0,.25,0,1,0,1,0], [1,0,0,.5,0,1,0,1,0,0,.25,1,0,1,.5,0],
[1,.25,0,0,.5,0,1,0,1,0,.5,0,.25,1,0,0], [1,1,1,0,0,0,1,.5,1,1,0,0,.25,0,0,0],
[1,0,.75,0,.25,1,0,0,1,.5,0,0,.5,0,1,0], [1,.5,0,1,0,.25,1,0,.5,0,1,0,1,0,0,.5]]


def seeds():
    rows=[]
    def add(g,prefix,genre,bpm,role,index,name,material,depth,values,purpose):
        values={k:round(v,6) for k,v in values.items()};values.update(v2_engine={'Clean':1,'Drum':2,'Duck':3,'Rhythm':4}[role],v2_enabled=1,v2_mix=1,v2_output=0,v2_listen=0)
        rows.append(dict(program=7+len(rows),id=f'za.{g}.{role.lower()}.{index+1:02}',name=f'{prefix} {name}',genre=genre,role=role,material=material+'.wav',depth=depth,bpm=bpm,note=f'{role} / {depth} / {bpm} BPM',description=purpose,values=values))
    for gi,(g,prefix,genre,bpm,factor) in enumerate(GENRES):
        beat=60000/bpm
        for i,(name,mat,depth,profile,expand,a,h,r,ratio,hp,lp) in enumerate(CLEAN):
            values=dict(v2_profile=profile,v2_expand=expand,v2_external=0,v2_rms=i%4==1,v2_threshold=-42+(i%3-1)*2,v2_range={'Gentle':8,'Medium':18,'Strong':36}[depth],v2_ratio=ratio+gi*.1,v2_knee=6+(i%3),v2_hysteresis=3+i%4,v2_attack=max(.2,a*(.95+gi*.03)),v2_hold=h*factor,v2_release=r*factor,v2_detectorMs=2+i%4,v2_lookahead=5,v2_hp=hp+gi*5,v2_lp=lp,v2_link=1)
            add(g,prefix,genre,bpm,'Clean',i,name,mat,depth,values,f'Preserve {name.lower()} detail while lowering pauses in {genre}; adapt threshold to the recording.')
        for i,(name,mat,depth,profile,a,body,tail,hp,lp) in enumerate(DRUM):
            body=body*factor;tail=min(tail*factor,beat*.5*.73-body-a)
            values=dict(v2_profile=profile,v2_external=0,v2_threshold=-36,v2_range={'Gentle':8,'Medium':18,'Strong':36}[depth],v2_attack=a+gi*.03,v2_body=body,v2_tail=tail,v2_onset=8+i%3,v2_retrigger=20+i%4*5,v2_lookahead=5,v2_hp=hp,v2_lp=lp,v2_link=1)
            add(g,prefix,genre,bpm,'Drum',i,name,mat,depth,values,f'{genre} {name.lower()}: preserve the hit, hold its body and shorten the following tail; retain quieter hits.')
        for i,(name,mat,depth,profile,a,h,r,hp,lp) in enumerate(DUCK):
            values=dict(v2_profile=profile,v2_external=1,v2_threshold=-34,v2_range={'Gentle':2,'Medium':4.5,'Strong':9}[depth],v2_attack=a+gi*.15,v2_hold=h*(.9+gi*.03),v2_release=beat*r,v2_detectorMs=3,v2_hysteresis=3+i%3,v2_lookahead=5,v2_hp=hp,v2_lp=lp,v2_link=1)
            add(g,prefix,genre,bpm,'Duck',i,name,mat,depth,values,f'Route a kick to External Key; make {mat} yield and recover in the {genre} pocket at about {bpm} BPM.')
        for i,(name,mat) in enumerate(RHYTHM):
            depth=['Gentle','Medium','Strong'][i%3];steps=32 if i in [3,7,9,11] else 16;pattern=PATTERNS[i];shift=(gi*(i+1))%16;pattern=pattern[shift:]+pattern[:shift]
            second=pattern[4:]+pattern[:4];levels=(pattern+second)[:32]
            # Genre-specific accents alter active recipes, not just names/tempo.
            at=(gi*3+i*5)%16
            if levels[at] not in [0,1]:levels[at]=[.25,.4,.6,.75][gi]
            values=dict(v2_profile=(i+gi)%2,v2_external=0,v2_sync=0 if i in [5,10] else 1,v2_steps=steps,v2_beats=.125 if i in [6,8] else .25,v2_ms=beat*(.125 if i in [6,8] else .25),v2_attack=.5+(i%4)*.35+gi*.08,v2_release=1.5+(i%5)*.4+gi*.1,v2_range={'Gentle':8,'Medium':18,'Strong':36}[depth])
            values.update({f'v2_step{n+1}':v for n,v in enumerate(levels)})
            add(g,prefix,genre,bpm,'Rhythm',i,name,mat,depth,values,f'{genre} {name.lower()} texture for {mat}; editable {steps}-step accents, floor and full openings.')
    assert len(rows)==200;return rows


def export(rows):
    HEADER.parent.mkdir(parents=True,exist_ok=True);lines=['// Generated from ZastavaRecipes.json; edit recipes/generator, not this file.']
    q=lambda x:json.dumps(x,ensure_ascii=True)
    for r in rows:
        values=', '.join('{ '+q(k)+', '+str(v)+' }' for k,v in sorted(r['values'].items()))
        fields=[q(r[k]) for k in ['id','genre','role','note','description','material','depth']]
        lines.append('{ '+q(r['name'])+', 1.0f, { '+values+' }, '+', '.join(fields)+', '+str(r['bpm'])+' },')
    HEADER.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('action',choices=['seed','export']);a=p.parse_args()
    if a.action=='seed':
        assert not JSON.exists(),'Refusing to replace recipes';rows=seeds();JSON.parent.mkdir(parents=True,exist_ok=True);JSON.write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8',newline='\n')
    else:rows=json.loads(JSON.read_text())
    export(rows);print('Exported',len(rows),'recipes')
