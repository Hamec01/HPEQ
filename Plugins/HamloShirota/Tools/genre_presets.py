"""Authored spatial recipes; deterministic genre physics, never random presets."""
import argparse
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

# name, depth, material, musical purpose, sound controls. Each row is a recipe.
GROUPS = [
('Kick',[
 ('Click Halo','Gentle','kick','A small high-frequency halo with a stable low punch',dict(v2Width=1.7,v2Generate=.9,v2Space=.25,v2MonoBass=100,v2LowWidth=.3)),
 ('Room Edge','Gentle','kick','Short room edges around the kick attack',dict(v2Space=.7,v2SpaceType=0,v2Depth=.2,v2Damping=5000)),
 ('Top Split','Gentle','kick','Keep the fundamental centered and separate the beater',dict(v2Width=2.8,v2HighWidth=1.6,v2MonoBass=180,v2Generate=1.1,v2Space=.2)),
 ('Offset Tap','Gentle','kick','A small delayed reflection behind the kick',dict(v2Space=.8,v2Predelay=12,v2Depth=.3,v2Size=.65)),
 ('Soft Perimeter','Gentle','kick','Diffuse upper edges rather than hard pan movement',dict(v2Generator=2,v2Generate=1.25,v2Space=.5,v2Damping=2800)),
 ('Side Pocket','Medium','kick','An audible room accent while leaving the low body centered',dict(v2Space=.95,v2SpaceType=1,v2Generate=1.4,v2Width=3.4,v2MonoBass=80)),
]),
('Snare / Clap',[
 ('Steel Rim','Medium','snare','Tight bright early reflections for a sharp rim',dict(v2Space=.85,v2SpaceType=0,v2Size=.6,v2Damping=14500)),
 ('Back Wall','Medium','snare','Place a soft reflected clap behind the listener',dict(v2Mode=2,v2PositionOn=1,v2Position=180,v2Source=1,v2Space=.4)),
 ('Split Hands','Medium','snare','Separate clap edges using micro timing',dict(v2Generator=1,v2Generate=1.35,v2Width=3.2,v2Space=.45)),
 ('Velour Dust','Gentle','snare','Darker diffuse sides under the snare body',dict(v2Generator=2,v2Generate=1.25,v2Space=.8,v2Damping=2200,v2Size=1.4)),
 ('Answer Arc','Medium','snare','A beat-locked left-right answer around snare hits',dict(v2Mode=2,v2PositionOn=1,v2Motion=1,v2Nodes=2,v2Space=.35)),
 ('Broken Fan','Heavy','snare','A pronounced binaural scatter accent for fills',dict(v2Mode=2,v2PositionOn=1,v2Position=-125,v2Spread=75,v2Space=.95,v2SpaceType=2,v2Profile=2)),
]),
('Hats / Cymbals',[
 ('Silver Wings','Medium','hihat','Wide high bands with a protected low center',dict(v2Width=3.5,v2HighWidth=1.5,v2Generate=1.3,v2Space=.3)),
 ('Fine Grain','Gentle','hihat','A compact diffuse shimmer around hat transients',dict(v2Generator=2,v2Generate=1.2,v2Space=.65,v2Size=.55)),
 ('Alternating Steps','Medium','hihat','Alternate two sides at a musical subdivision',dict(v2Mode=2,v2PositionOn=1,v2Motion=1,v2Unit=1,v2Nodes=2,v2Rate=2)),
 ('High Ceiling','Medium','hihat','Open reflections with an airy damped far edge',dict(v2Space=.95,v2SpaceType=1,v2Size=1.8,v2Depth=.85,v2Damping=12000)),
 ('Rear Sparks','Heavy','hihat','Move four cymbal accents around the horizontal head plane',dict(v2Mode=2,v2PositionOn=1,v2Motion=1,v2Nodes=4,v2Profile=1,v2Space=.65,v2SpaceType=2)),
]),
('Percussion',[
 ('Hand Circle','Medium','perc','Four stopped positions for hand percussion',dict(v2Mode=2,v2PositionOn=1,v2Motion=1,v2Nodes=4,v2Space=.25)),
 ('Wood Chamber','Medium','perc','A dark short chamber for wooden hits',dict(v2Space=.95,v2SpaceType=0,v2Size=.7,v2Damping=1900,v2Generator=2)),
 ('Open Tiles','Medium','perc','Hard scattered early accents in an open field',dict(v2Space=.9,v2SpaceType=2,v2Damping=15000,v2Predelay=19,v2Width=3)),
 ('Side Conversation','Gentle','perc','A restrained two-side question and answer',dict(v2PositionOn=1,v2Motion=1,v2Nodes=2,v2Spread=65,v2Space=.4)),
]),
('Drum Bus',[
 ('Parallel Walls','Gentle','drumbus1','Add parallel room edges without moving the drum center',dict(v2Space=.85,v2SpaceType=0,v2Width=1.8,v2MonoBass=170)),
 ('Break Panorama','Medium','drumbus1','Separate break transients over three frequency bands',dict(v2Width=3,v2LowWidth=.65,v2MidWidth=1.25,v2HighWidth=1.6,v2Generate=1)),
 ('Dark Air','Gentle','drumbus1','Diffuse dark room energy beneath dry drum attacks',dict(v2Space=.95,v2SpaceType=1,v2Damping=2300,v2Generator=2)),
 ('Scatter Fill','Medium','drumbus1','Use bright scattered edges for a drum fill',dict(v2Space=1,v2SpaceType=2,v2Size=1.5,v2Predelay=21,v2Generate=1.2)),
 ('Center Anchor','Gentle','drumbus1','Keep bass mono while opening upper drum layers',dict(v2Width=2.2,v2LowWidth=0,v2MidWidth=1.2,v2HighWidth=1.8,v2MonoBass=220,v2Space=.3)),
 ('Moving Break','Medium','drumbus1','A four-node horizontal break rotation',dict(v2Mode=2,v2PositionOn=1,v2Motion=1,v2Nodes=4,v2Space=.45,v2Spread=120)),
 ('Rear Room','Medium','drumbus1','A fixed rear room blended beneath the drum bus',dict(v2Mode=2,v2PositionOn=1,v2Position=165,v2Profile=1,v2Space=.8,v2SpaceType=1)),
]),
('808 / Bass',[
 ('Upper Outline','Gentle','bass1','Give bass harmonics width above a centered fundamental',dict(v2MonoBass=100,v2LowWidth=.2,v2Width=2.8,v2Generate=1.4,v2Space=.25)),
 ('Cabinet Edges','Gentle','bass2','A small dark reflected cabinet around the bass',dict(v2Space=.9,v2Damping=1800,v2Size=.6,v2MonoBass=90)),
 ('Micro Chorus','Gentle','bass1','Micro-separated upper bass with restrained room',dict(v2Generator=1,v2Generate=1.45,v2Width=3,v2MonoBass=80,v2Space=.35)),
 ('Harmonic Clouds','Medium','bass2','Diffuse harmonic edges for melodic bass',dict(v2Generator=2,v2Generate=1.45,v2Space=.9,v2SpaceType=1,v2Width=3.5,v2MonoBass=65)),
 ('Bass Answer','Medium','bass2','Blend a binaural side answer beneath the bass line',dict(v2Mode=2,v2PositionOn=1,v2Position=110,v2Source=1,v2Space=.35,v2MonoBass=110)),
]),
('Vocals',[
 ('Lead Frame','Gentle','vocal_lead1','A close frame around the lead without moving its center',dict(v2Generator=1,v2Generate=1.2,v2Space=.6,v2Size=.65,v2Predelay=23)),
 ('Soft Doubles','Medium','vocal_back1','Diffuse width for supporting vocal doubles',dict(v2Generator=2,v2Generate=1.4,v2Space=.75,v2SpaceType=1,v2Damping=4200)),
 ('Far Reply','Medium','vocal_lead2','Place a vocal reply behind the listener',dict(v2Mode=2,v2PositionOn=1,v2Position=-170,v2Source=1,v2Profile=1,v2Space=.8)),
 ('Call And Answer','Medium','vocal_back2','Two timed vocal positions with clear holds',dict(v2Mode=2,v2PositionOn=1,v2Motion=1,v2Nodes=2,v2Space=.45,v2Spread=45)),
 ('Dream Walk','Heavy','vocal_lead3','A soft four-node vocal walk around the head',dict(v2Mode=2,v2PositionOn=1,v2Motion=1,v2Nodes=4,v2Profile=2,v2Space=.95,v2SpaceType=1)),
]),
('Keys / Piano',[
 ('Felt Sides','Gentle','keys','Dark diffuse edges for felt keys',dict(v2Generator=2,v2Generate=1.2,v2Space=.85,v2Damping=2600,v2Size=1.25)),
 ('Ivory Spread','Medium','keys2','A broad three-band piano image',dict(v2Width=3.2,v2LowWidth=.6,v2MidWidth=1.2,v2HighWidth=1.6,v2Space=.55)),
 ('Window Room','Medium','keys','Open delayed reflections around sustained keys',dict(v2Space=1,v2SpaceType=1,v2Size=1.8,v2Depth=.9,v2Predelay=27)),
 ('Orbit Chords','Heavy','keys2','Slow chord movement around four horizontal positions',dict(v2Mode=2,v2PositionOn=1,v2Motion=1,v2Nodes=4,v2Rate=.5,v2Space=.8,v2Profile=1)),
]),
('Samples / Melody',[
 ('Sample Frame','Gentle','guitar','A wide phase-shaped frame for a chopped sample',dict(v2Generator=0,v2Generate=1.3,v2Width=2.8,v2Space=.55)),
 ('Dub Corner','Medium','synth','Damped delayed room edges for melodic fragments',dict(v2Space=1,v2SpaceType=2,v2Predelay=37,v2Damping=3100,v2Size=1.65)),
 ('Passing Texture','Medium','synth','A slow stereo trajectory for a background texture',dict(v2PositionOn=1,v2Motion=1,v2Nodes=4,v2Spread=90,v2Space=.7)),
 ('Rear Constellation','Heavy','guitar','A full four-node binaural sample constellation',dict(v2Mode=2,v2PositionOn=1,v2Motion=1,v2Nodes=4,v2Space=.95,v2SpaceType=2,v2Profile=2)),
]),
('Master',[
 ('Fine Panorama','Gentle','mixdown_boom_bap_3','A restrained frequency-shaped stereo panorama',dict(v2Width=1.9,v2LowWidth=.5,v2MidWidth=1.1,v2HighWidth=1.3,v2Generate=.35,v2Space=0)),
 ('Center Foundation','Gentle','mixdown_sad_trap','Center low energy while opening high stereo content',dict(v2Width=2,v2LowWidth=.2,v2MonoBass=140,v2Generate=.5,v2Space=.15)),
 ('Air Perimeter','Gentle','mixdown_Ardnold','Add a small diffuse upper perimeter to a mix',dict(v2Width=1.65,v2HighWidth=1.6,v2Generator=2,v2Generate=.6,v2Space=.5,v2Damping=6500)),
 ('Parallel Depth','Gentle','mixdown_Ardnold','A quiet open reflection field behind the mix',dict(v2Width=1.5,v2Space=.85,v2SpaceType=1,v2Depth=.8,v2Predelay=31,v2MonoBass=170)),
])]


def recipes():
    result=[]
    genres=[('bb','Boom Bap','BB',90,['Classic','Dusty','Jazzy','Gold','Underground','Lofi']),
            ('trap','Trap','Trap',140,['ATL','Dark','Cloud','Rage','Memphis','Luxury']),
            ('techno','Techno','Techno',132,['Peak','Hypnotic','Minimal','Detroit','Dub','Acid','Hard']),
            ('dnb','DnB','DnB',174,['Modern','Roller','Liquid','Neuro','Jump-Up','Break'])]
    for genreIndex,(code,genre,prefix,bpm,styles) in enumerate(genres):
        slot=0
        for groupIndex,(group,rows) in enumerate(GROUPS):
            for rowIndex,(title,depth,material,note,controls) in enumerate(rows):
                values=dict(v2Mode=1,v2Page=0,v2Width=2.3,v2LowWidth=.6,v2MidWidth=1,v2HighWidth=1.2,
                            v2LowSplit=[260,190,340,280][genreIndex],v2HighSplit=[2600,4200,3300,5100][genreIndex],
                            v2MonoBass=120,v2Generator=genreIndex%3,v2Generate=.8,v2SpaceType=0,v2Space=.4,
                            v2Size=[1.15,.85,1.35,.7][genreIndex],v2Depth=.5,v2Damping=[5600,11500,7800,14200][genreIndex],
                            v2Predelay=60000/bpm/64,v2Mix=1,v2Output=0)
                values.update(controls)
                if 'v2Damping' in controls:values['v2Damping']=min(18000,controls['v2Damping']*[.88,1.1,1,.96][genreIndex])
                if 'v2Size' in controls:values['v2Size']=max(.5,min(2,controls['v2Size']*[1.08,.92,1.04,.88][genreIndex]))
                if values.get('v2Motion'):
                    values.update(v2Unit=1,v2Loop=1)
                    count=values.get('v2Nodes',2)
                    angles=([-75,75] if count==2 else [-90,180,90,0])
                    for node,angle in enumerate(angles):
                        hold=[.5,.25,1,.125][genreIndex];travel=[1,.5,2,.5][genreIndex]
                        values.update({f'v2Node{node}Angle':angle,f'v2Node{node}HoldBeat':hold,
                                       f'v2Node{node}TravelBeat':travel,f'v2Node{node}Hold':hold*60000/bpm,
                                       f'v2Node{node}Travel':travel*60000/bpm,f'v2Node{node}Arc':0 if count==2 else 1,
                                       f'v2Node{node}Curve':0 if group in ['Hats / Cymbals','Percussion'] else 1})
                # First BB Bass Answer reached -12.097dB at full Mix, below the
                # -12dB calibration center. Strengthen its room, not the target.
                if genreIndex==0 and group=='808 / Bass' and title=='Bass Answer':
                    values.update(v2Space=.8,v2Predelay=60000/bpm/32)
                name=f'{prefix} {styles[slot%len(styles)]} {title}'
                identifier=f'shirota.v2.{code}.{groupIndex+1:02}.{rowIndex+1:02}'
                result.append(dict(id=identifier,name=name,genre=genre,group=group,note=note+'.',material=material,depth=depth,bpm=bpm,values=values))
                slot+=1
        assert slot==50
    assert len(result)==200
    return result


def generate(calibration=None):
    rows=recipes();lines=['// Generated by Tools/genre_presets.py. Stable IDs; original programs precede these.']
    for row in rows:
        values=dict(row['values'])
        if calibration is not None:
            record=calibration[row['id']];values.update(v2Mix=record['mix'],v2Output=record['output_db'])
        fields=[json.dumps(row[k]) for k in ['id','name','genre','group','note','material','depth']]
        settings=','.join('{'+json.dumps(k)+','+repr(float(v))+'}' for k,v in sorted(values.items()))
        lines.append('{'+','.join(fields)+','+repr(float(row['bpm']))+',{'+settings+'}},')
    (ROOT/'Source/Standalone/GenrePresets.inc').write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    return rows


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--calibration',type=Path);a=p.parse_args()
    records=json.loads(a.calibration.read_text())['presets'] if a.calibration else None
    rows=generate(records);print('Generated',len(rows),'genre programs')
