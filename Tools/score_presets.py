"""Authored Echo recipes. Genre profiles change DSP, not just Mix/Output.

Permanent IDs depend on genre/category/position, never the displayed name.
Calibration is deliberately a separate, source-pinned operation.
"""
import argparse,json,hashlib,re
from pathlib import Path

# name, material, depth, intervals, rhythm divisions, action, space, bands, loop step, texture
# Rhythms: 5=eighth, 6=dotted eighth, 7=quarter triplet, 8=quarter, 9=dotted quarter,
# 10=half, 11=whole. Deliberate named recipes; no random parameter generation.
RECIPES={
'Vocal':[
('Close Slap','vocal_lead1','Gentle',[0],[0],0,0,0,0,0),
('Soft Double','vocal_lead2','Gentle',[0,0],[0,0],0,1,0,0,0),
('Whisper Sides','vocal_back1','Gentle',[0,0],[5,6],0,2,1,0,0),
('Dotted Throw','vocal_lead1','Medium',[0,0,0],[6,9,11],0,1,0,0,0),
('Answer Across','vocal_lead2','Medium',[0,0,0,0],[5,8,9,10],0,0,1,0,0),
('Low Answer','vocal_lead3','Medium',[-12,0,-12],[8,9,11],0,0,0,0,0),
('Fifth Halo','vocal_back2','Heavy',[0,7,12],[5,8,10],0,2,0,0,0),
('Reverse Breath','vocal_lead3','Medium',[0,0],[8,10],1,2,0,0,0),
('Descending Words','vocal_lead1','Heavy',[0,-12],[8,10],0,1,0,-12,0),
('Curtain Behind','vocal_back3','Heavy',[0,12,7,0],[6,8,10,11],0,2,1,0,2)],
'Drums':[
('Rim Room','perc','Gentle',[0],[0],0,0,0,0,0),
('Snare Plate','snare','Medium',[0,0],[0,5],0,1,0,0,0),
('Hat Ticks','hihat','Gentle',[0,0,0],[1,2,5],0,0,1,0,0),
('Triplet Percussion','perc','Medium',[0,7,0],[4,7,10],0,0,0,0,0),
('Clap Back','snare','Gentle',[0,0],[0,0],0,1,0,0,0),
('Stutter Snare','snare','Heavy',[0,0],[5,8],2,1,0,0,0),
('Reverse Crash','hihat','Medium',[0,-12],[8,10],1,2,0,0,0),
('Low Tom Orbit','kick','Medium',[0,-12,7],[5,8,9],0,0,1,0,0),
('Metal Ping','perc','Medium',[12,7,12],[2,5,8],0,1,0,0,0),
('Half Time Break','drumbus1','Heavy',[0,0,-12],[8,10,11],0,0,1,0,3),
('Filtered Drum Dub','drumbus1','Heavy',[0,0],[6,10],0,1,1,0,0),
('Ghost March','drumbus1','Medium',[0,0,0,0],[2,3,5,6],0,0,0,0,1)],
'Samples / Keys':[
('Dust Keys','keys','Gentle',[0,0],[0,5],0,0,0,0,0),
('Chord Answers','keys2','Medium',[0,7,0],[6,8,10],0,1,0,0,0),
('Low Keys','keys','Medium',[-12,-12],[8,11],0,0,1,0,0),
('Broken Memory','guitar','Heavy',[0,0,0],[5,8,10],2,0,0,0,1),
('Reverse Chords','keys2','Heavy',[0,7],[8,10],1,2,0,0,0),
('Taped Sample','guitar','Medium',[0,0],[6,9],3,1,0,0,0),
('Prism Chords','keys','Medium',[0,0,0],[5,8,11],0,1,1,0,0),
('Fifth Mosaic','keys2','Heavy',[0,7,12,-12],[5,8,10,11],0,0,0,0,0),
('Vanishing Phrase','guitar','Gentle',[0,0],[6,10],0,2,0,0,5),
('Descending Memory','keys','Heavy',[0,-12],[8,11],0,2,0,-12,0)],
'Synth':[
('Arp Echo','synth','Medium',[0,0,0,0],[2,5,6,8],0,0,0,0,0),
('Fifth Steps','synth','Heavy',[-12,-5,0,7],[5,8,9,10],0,1,0,0,0),
('Octave Rain','synth','Heavy',[0,12,24,12],[5,8,10,11],0,2,0,0,0),
('Low Orbit','bass1','Medium',[0,-12,0],[5,9,11],0,0,1,0,0),
('Acid Rattle','synth','Medium',[0,0,7],[2,4,5],2,1,0,0,0),
('Spectral Weave','synth','Heavy',[0,0,0],[5,8,10],0,2,1,0,0),
('Reverse Lead','synth','Medium',[0,7],[6,10],1,1,0,0,0),
('Climbing Loop','synth','Heavy',[0],[8],0,2,0,12,0)],
'Atmosphere / FX':[
('Room Into Mist','keys2','Medium',[0,0],[8,10],0,2,1,0,0),
('Reverse Horizon','synth','Heavy',[0,-12,12],[8,10,11],1,2,0,0,0),
('Shattered Cloud','guitar','Heavy',[0,7],[8,11],0,2,0,0,3),
('Falling Sky','synth','Heavy',[-12,0],[9,11],0,2,0,-12,0),
('Rising Sparks','hihat','Medium',[0,12],[5,10],0,1,0,12,0),
('Stopped World','keys','Heavy',[0,0],[8,11],3,2,0,0,4),
('Empty Rooms','perc','Gentle',[0,0,0],[8,10,11],0,0,1,0,5),
('Backward Dust','guitar','Medium',[-12,0],[6,10],1,0,1,0,2),
('Split Distance','vocal_back3','Heavy',[0,7,0],[8,9,11],0,2,1,0,0),
('Endless Geometry','synth','Heavy',[0,7,12,-12],[6,8,10,11],0,2,1,0,1)]}

PROFILES=[('bb','Boom Bap','BB',90,2,.24,5400),('trap','Trap','Trap',140,0,0,12500),('techno','Techno','Techno',132,3,.08,7200),('dnb','DnB','DnB',174,1,.04,10500)]
SLUGS={'Vocal':'vocal','Drums':'drums','Samples / Keys':'keys','Synth':'synth','Atmosphere / FX':'fx'}

def recipes():
    out=[]
    for genre_index,(slug,genre,prefix,bpm,color,swing,hicut) in enumerate(PROFILES):
        for group,rows in RECIPES.items():
            for position,(name,material,depth,notes,rhythm,action,space,bands,step,texture) in enumerate(rows,1):
                ident=f'echo.{slug}.{SLUGS[group]}.{position:02}'
                v={'echoMode':1,'scoreSource':60,'scoreMix':.5,'scoreOutput':0,'scoreColor':color,'scoreSwing':swing,
                   'scoreFeedback':.22+.035*(position%6),'scoreFeedbackSync':8 if group!='Atmosphere / FX' else 10,
                   'scoreFeedbackPitch':step,'scoreDrive':2+genre_index,'scoreLowCut':75 if group!='Drums' else 45,
                   'scoreHighCut':hicut,'scoreDuck':.35 if group=='Vocal' else .08,'scoreWidth':1.05+.1*(genre_index%3),
                   'scoreSpaceMix':.12 if space==0 else .33 if space==1 else .65,'scoreSpaceType':space,
                   'scoreSpaceRoute':(position+genre_index)%3,'scoreSpaceDecay':(.45 if space==0 else 1.15 if space==1 else 3.6)*(1+.1*genre_index),
                   'scoreSpaceSize':.7+.13*genre_index,'scoreSpaceDamping':hicut*.7,'scoreBands':bands,
                   'scoreGlitch':texture,'scoreGlitchAmount':.3 if texture else 0,'scoreGlitchTarget':1,
                   'scoreGlitchMs':60000/bpm*(.25 if group=='Drums' else .5),'scoreFeedbackMs':60000/bpm}
                for t in range(8):v[f'tap{t+1}On']=int(t<len(notes))
                # Heavy recipes need sufficient level before the output trim;
                # retain an intelligible direct component in pre-space routing.
                # First calibration:28/200 required >+12dB. Fix authored balance,
                # not the energy bands or the public output range.
                if depth=='Heavy' and v['scoreSpaceRoute']==2:v['scoreSpaceMix']=min(v['scoreSpaceMix'],.5)
                for t,interval in enumerate(notes):
                    key=f'tap{t+1}';v.update({key+'Note':60+interval,key+'Sync':rhythm[t],key+'Shape':action,
                        key+'Time':(73+17*t+8*genre_index+3*position) if rhythm[t]==0 else 60000/bpm,
                        key+'Gain':(0 if depth=='Heavy' else -7)-1.6*t,key+'Pan':0 if len(notes)==1 else (-1 if t%2==0 else 1)*(.35+.12*genre_index),
                        key+'Fragment':min(2000,60000/bpm*(.25 if group=='Drums' else 1)),key+'Repeats':3 if genre=='DnB' else 4,
                        key+'Low':80 if group=='Vocal' else 10,key+'High':hicut,key+'SpaceSend':(.3+.7*t/max(1,len(notes)-1)) if group=='Vocal' else 1})
                if bands:
                    for b,(scale,fb,pan) in enumerate([(.75,.20,-.3),(1,.32,0),(1.5,.23,.3)],1):
                        v.update({f'band{b}Time':scale+(genre_index*.125 if b==3 else 0),f'band{b}Feedback':fb,
                            f'band{b}Pan':pan,f'band{b}Gain':-2 if b==1 and group=='Vocal' else 0})
                description=f"{name}: {('backward fragments' if action==1 else 'chopped fragments' if action==2 else 'slowing fragments' if action==3 else 'note echoes')}; {('split bands' if bands else 'full band')}, {('Room','Plate','Cloud')[space]}. Source FL C5 / MIDI 60."
                fingerprint=hashlib.sha256(json.dumps({k:x for k,x in v.items() if k not in ['scoreMix','scoreOutput']},sort_keys=True).encode()).hexdigest()
                out.append(dict(id=ident,name=f'{prefix} {name}',genre=genre,group=group,note=description,material=material,depth=depth,bpm=bpm,values=v,recipe_sha256=fingerprint))
    assert len(out)==200 and len({r['id'] for r in out})==200 and len({r['recipe_sha256'] for r in out})==200
    return out

def main():
    p=argparse.ArgumentParser();p.add_argument('--calibration',type=Path);args=p.parse_args();root=Path(__file__).resolve().parents[1]
    rows=recipes();cal=json.loads(args.calibration.read_text()) if args.calibration else {}
    if cal:
        version=re.search(r'project\(HamloEcho VERSION ([\d.]+)',(root/'CMakeLists.txt').read_text()).group(1)
        sources=sorted([path for folder in ['DSP','Standalone'] for path in (root/'Source'/folder).rglob('*') if path.is_file()],key=lambda path:str(path).replace('\\','/'))
        current=hashlib.sha256((version+''.join(hashlib.sha256(path.read_bytes()).hexdigest() for path in sources)).encode()).hexdigest()
        assert len(cal)==200 and all(c.get('dsp_source')==current for c in cal.values()),'Calibration belongs to another DSP version'
    lines=['// Generated by Tools/score_presets.py. Keep IDs and original programs stable.']
    for r in rows:
        v=r['values'].copy()
        if r['id'] in cal:
            c=cal[r['id']];assert c['recipe_sha256']==r['recipe_sha256'] and c['passed'],'Stale or failed recipe calibration';v['scoreOutput']=c['output_db']
        fields=', '.join('{ '+json.dumps(k)+', '+format(x,'.12g')+' }' for k,x in v.items())
        metadata=', '.join(json.dumps(r[k]) for k in ['id','genre','group','note','material','depth'])
        lines.append('{ '+json.dumps(r['name'])+', 0.5f, { '+fields+' }, '+metadata+', '+str(r['bpm'])+' },')
    (root/'Source/Core/ScorePresets.inc').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    (root/'Tools/score_recipes.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
    catalog=['# HamloEcho factory library','', '200 new stable-ID recipes;208 host programs including Default and the seven original programs.','',
        'All note recipes declare manual SOURCE FL C5 (MIDI60). Rhythms follow host tempo. Calibration uses the named owner material at the listed reference tempo. Gentle/Medium/Heavy describe measured wet/dry energy, not a volume guarantee on arbitrary input.','',
        'Methods: [PRESET_METHODS.md](PRESET_METHODS.md). Final compiled-bank measurements: [verification.json](reports/presets/verification.json).','']
    for _,genre,_,_,_,_,_ in PROFILES:
        catalog += ['## '+genre,'','| Program | Permanent ID | Name | Category | Depth | BPM | Material |','|---:|---|---|---|---|---:|---|']
        for program,r in enumerate(rows,8):
            if r['genre']==genre:catalog.append(f"| {program} | {r['id']} | {r['name']} | {r['group']} | {r['depth']} | {r['bpm']} | {r['material']} |")
        catalog.append('')
    (root/'docs/GENRE_PRESETS.md').write_text('\n'.join(catalog),encoding='utf-8')
    print('Generated 200 unique recipes; original seven positions retained.')
if __name__=='__main__':main()
