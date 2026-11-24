"""Hand-authored rack decisions. Helpers encode units/routing, never invent recipes.

These are design candidates. No runtime state or audio-calibration claim is made.
All text shipped in the product/repository is English.
"""
from copy import deepcopy


def module(name, amount=1, **params):
    return dict(plugin="Hamlo"+name, amount=amount, parameters=params)


def C(t, ratio, attack, release, model=1, source=1, amount=1, knee=6):
    return module("Comp",amount,thresholdDb=t,ratio=ratio,attackMs=attack,releaseMs=release,
                  model=model,source=source,kneeDb=knee,detector=1,character=0,autoMakeup=0,makeupDb=0,mix=1)


def U(snap, body, speed=.6, style=0, amount=1, **extra):
    return module("Udarnik",amount,**dict(dict(attackDb=snap,sustainDb=body,speed=speed,
        style=style,engine=1,protect=1,lookahead=1,listen=0,rise=1,decay=1,sensitivity=.8,preserve=1,mix=1,outputDb=0),**extra))


def M(model, drive, amount=1, **extra):
    return module("Mag",amount,**dict(dict(model=model,driveDb=drive,mix=1,trimDb=0,
        oversampling=3,autoGain=1,harmonics=.35,tone=0,evenOdd=0,tapeBias=.5,speed=0,
        wow=0,hiss=0,hum=0,crackle=0,noiseFollow=1,punish=0,thump=0,steep=0,
        sampler=0,phoneBass=0,subProtectHz=0,dynamics=0,match=0,delta=0),**extra))


def V(space, decay, predelay, amount, engine=1, hp=180, lp=8000, **extra):
    return module("Verb",amount,**dict(dict(space=space,engine=engine,decaySeconds=decay,
        predelayMs=predelay,predelaySync=0,lowCutHz=hp,highCutHz=lp,bassMult=.75,
        hfDecayRatio=.5,width=1.1,modDepth=.1,duck=.15,routing=1,mix=1,keepDry=0,
        wetDb=0,outputDb=0,earlyDb=0,lateDb=0,timeShape=0,shimmerAmount=0,freeze=0),**extra))


def W(low=1, mid=1, high=1.2, mono=120, amount=1, **extra):
    return module("Shirota",amount,**dict(dict(v2Mode=1,v2Width=1,v2LowWidth=low,
        v2MidWidth=mid,v2HighWidth=high,v2LowSplit=300,v2HighSplit=3000,v2MonoBass=mono,
        v2Generate=0,v2Guard=1,v2Floor=.15,v2Mix=1,v2Output=0,v2Motion=0,
        v2PositionOn=0,v2Mono=0),**extra))


def E(sync, feedback, amount, color=0, hp=180, lp=8000, taps=None, **extra):
    p=dict(echoMode=1,quickEnabled=0,scoreRouting=1,scoreMix=1,scoreOutput=0,
        scoreSource=60,scoreFeedback=feedback,scoreFeedbackSync=sync,scoreFeedbackMs=500,
        scoreFeedbackPitch=0,scoreColor=color,scoreDrive=0,scoreLowCut=hp,scoreHighCut=lp,
        scoreWidth=1,scoreDuck=.2,scoreFreeze=0,scoreSpaceMix=0,scoreGlitch=0,
        scoreGlitchAmount=0,scoreBands=0,scoreSwing=0)
    for n in range(1,9): p[f"tap{n}On"]=0
    taps=taps or [(sync,0,-6,0,0)]
    for n,(division,note,gain,pan,action) in enumerate(taps,1):
        p.update({f"tap{n}On":1,f"tap{n}Sync":division,f"tap{n}Time":250*n,
            f"tap{n}Note":60+note,f"tap{n}Gain":gain,f"tap{n}Pan":pan,
            f"tap{n}Shape":action,f"tap{n}Fragment":125,f"tap{n}Repeats":3,
            f"tap{n}Low":hp,f"tap{n}High":lp,f"tap{n}SpaceSend":0})
    p.update(extra)
    return module("Echo",amount,**p)


def F(mode=0, gain=0, target=-18, amount=1, **extra):
    return module("Fader",amount,**dict(dict(engine=1,enabled=1,mode=mode,gainDb=gain,
        targetDb=target,mix=1,polarity=0,mute=0,solo=0,inputDb=0,outputDb=0,leftDb=0,
        rightDb=0,matchedDb=0,peakGuard=1,matchCeiling=-1,strength=.55,maxBoost=3,
        maxCut=6,floorDb=-55,holdMs=100,riseMs=250,fallMs=140,depthDb=0,
        rightPhase=0),**extra))


def motion(depth, beats, levels, **extra):
    p=dict(shape=0,points=len(levels),sync=1,beats=beats,depthDb=depth,edgeMs=3,smooth=1)
    for n,v in enumerate(levels): p[f"level{n:02d}"]=v
    p.update(extra)
    return F(3,**p)


def Z(threshold, reduction, hold, release, engine=1, amount=1, **extra):
    return module("Zastava",amount,**dict(dict(v2_engine=engine,v2_enabled=1,
        v2_threshold=threshold,v2_range=reduction,v2_hold=hold,v2_release=release,
        v2_attack=.3,v2_hysteresis=4,v2_knee=6,v2_external=0,v2_expand=1,v2_ratio=2,
        v2_link=1,v2_mix=1,v2_output=0,v2_body=hold,v2_tail=release,v2_lookahead=2,
        v2_hp=0,v2_lp=0,v2_listen=0),**extra))


def rhythm(levels, beats=.25, amount=1):
    p=dict(v2_steps=16,v2_beats=beats,v2_sync=1,v2_attack=2,v2_release=8,v2_range=24)
    for i in range(1,33): p[f"v2_step{i}"]=levels[(i-1)%len(levels)]
    return Z(-40,24,0,8,engine=4,amount=amount,**{k:v for k,v in p.items() if k!="v2_range"})


def L(threshold=-2, release=80, style=0, **extra):
    return module("RedWall",**dict(dict(engine=1,thresholdDb=threshold,ceilingDb=-1,
        releaseMs=release,autoRelease=0,lookaheadMs=2,link=1,truePeak=1,style=style,
        outputDb=0,drive=0,tube=0,monitor=0,mix=1),**extra))


def R(amount=.3, time=None, glitch=None, motion_steps=None, target=1, shape=None, throw=None, **extra):
    lanes=[[0]*16 for _ in range(7)]
    for i,v in (time or {}).items(): lanes[0][i]=v
    for i,v in (glitch or {}).items(): lanes[1][i]=v
    if motion_steps is not None: lanes[2]=list(motion_steps)
    if shape: lanes[3]=list(shape[1])
    if throw:
        for i,v in throw[1].items(): lanes[5][i]=v
    p=dict(mix=1,free=0,grid=2,triplet=0,swing=0,humanize=0,fadeMs=3,
        order=0,scene=0,listen=0,timeOn=int(any(lanes[0])),glitchOn=int(any(lanes[1])),
        motionOn=int(any(lanes[2])),timeMix=1,timeDepth=1,glitchMix=1,glitchDepth=.5,
        motionMix=1,motionDepth=.35,motionSmooth=.5,motionTarget=target,timeStretch=1,
        glitchEngine=1,shapeAOn=int(bool(shape)),shapeBOn=0,throwAOn=int(bool(throw)),throwBOn=0)
    if shape: p.update(shapeATarget=shape[0],shapeADepth=.4,shapeASmooth=.6)
    if throw: p.update(throwATarget=throw[0],throwAMix=.3,throwATail=.25,throwATime=5)
    p.update(extra)
    m=module("Revolution",amount,**p)
    m["scene"]=dict(lanes=lanes,locks=[65535]*3,allow_masks=[63,1023,65535],
        lane_names=["TIME","BLOCK GLITCH","MOTION","SHAPE A","SHAPE B","THROW A","THROW B"],
        encoding="semantic_scene_v1; encode with pinned native scene serializer")
    return m


def eq(shape,hz,gain=0,q=.707,dynamic=None):
    v=dict(shape=shape,hz=hz,gain_db=gain,q=q,slope_db_oct=18 if shape=="LowCut" else 12)
    if dynamic:
        v["dynamic"]=dict(range_db=dynamic,threshold_db=-28,ratio=3,attack_ms=3,release_ms=90,detect="Band")
    return v


def hp(hz): return eq("LowCut",hz)
def lp(hz): return eq("HighCut",hz)
def bell(hz,db,q=1,dynamic=None): return eq("Bell",hz,db,q,dynamic)
def lo(hz,db): return eq("LowShelf",hz,db)
def hi(hz,db): return eq("HighShelf",hz,db)
def band(lo,hi,*chain): return dict(low_hz=lo,high_hz=hi,chain=list(chain))


MATERIAL={"Kick":"isolated kick hits including soft and hard strikes",
    "Snare":"snare or clap hits including ghost notes and a flam",
    "Hats":"closed/open hats, an accent and a dense sixteenth roll",
    "Percussion":"a sparse percussion loop with velocity variation",
    "Drum Bus":"a stereo unprocessed drum loop with kick, snare, hats and ghost notes",
    "Bass":"a monophonic bass phrase with low and high notes and sustained tails",
    "Vocal":"a dry lead phrase with plosives, sibilance and a quiet ending",
    "Keys":"a dry chord phrase with staccato and held notes",
    "Samples":"a musical sample loop with gaps and a clear phrase ending",
    "Master":"an unmastered stereo mix with headroom and a busy chorus",
    "Lead Vocal":"a dry sung/spoken lead phrase with quiet and loud words",
    "Backing Vocal":"a dry backing stack with consonant unisons and held vowels",
    "Guitar":"a dry guitar phrase with pick attacks, ringing notes and a stop",
    "Synth":"a dry synth phrase with a held note and a transient passage",
    "Mix Bus":"a balanced stereo premaster with kick, bass and lead vocal",
    "Utility":"an impulse, tone burst and representative stereo musical phrase"}


def recipes():
    out=[]
    def bank(name,bpm):
        def add(group,title,intent,reject,eqs=(),chain=(),regions=(),output="insert"):
            chain=deepcopy(list(chain))
            if group=="Master":
                # Keep the existing <= -1 dBTP gate. The previous bank needed
                # individual -0.05 dB ceiling repairs for independent-meter
                # overshoot; author that margin before this calibration run.
                assert chain[-1]["plugin"]=="HamloRedWall"
                chain[-1]["parameters"]["ceilingDb"]=-1.05
            n=len(out)+1
            r=dict(id=f"HR{n:03d}",bank=name,category=group,name=title,bpm=bpm,
                source=MATERIAL[group],intent=intent,reject_if=reject,eq=list(eqs),chain=list(chain),
                regions=list(regions),output_mode=output,
                calibration=dict(status="not_run",reference_input_dbfs=-14 if group in ("Master","Mix Bus") else -18,
                    input_peak_max_dbfs=-3,loudness_match_tolerance_lu=.5,
                    output_true_peak_max_dbtp=-1 if group=="Master" else None,
                    intent_check=intent+" Assess against: "+reject,
                    required_clips=2,required_level_offsets_db=[-6,0,6]))
            if any(m["plugin"]=="HamloFader" and m["parameters"].get("mode")==1 for m in chain):
                r["calibration"]["required_action"]="Capture native Fader Match on an active clip window; persist measured matchedDb; silence must not produce a boost."
            out.append(r)
        return add

    a=bank("Boom Bap",90)
    a("Kick","BB Classic Front Knock","Keep the first knock forward and remove cardboard behind it.","The 50-90 Hz body falls or the leading hit softens.",[hp(28),bell(280,-2,1.2)],[U(2,-1),M(3,5,.45)])
    a("Kick","BB Dusty Short Foot","Shorten a long dusty kick while retaining its low fundamental.","The tail masks the next bass note or the fundamental becomes thin.",[hp(26),hi(6500,-2)],[Z(-42,12,45,80,2),M(1,7,.5,sampler=2)])
    a("Kick","BB Jazzy Felt Weight","Round the top of a soft acoustic kick without blunting its low pulse.","The felt click becomes brighter or 60-120 Hz loses weight.",[lo(85,1),bell(3000,-2)],[U(-1,1,.4),C(-23,2,25,167,3)])
    a("Kick","BB Gold Mid Knock","Add mid harmonics to a sub-heavy kick for small-speaker audibility.","The new midrange buzz overwhelms the dry sub.",[hp(25)],[M(2,8,.4,phoneBass=.2,subProtectHz=65),C(-20,2,18,145)])
    a("Kick","BB Underground Parallel Heel","Blend a crushed mid-band under the original attack.","The dry transient is quieter after loudness matching.",[bell(420,-1.5)],[C(-31,7,8,125,2,amount=.25)], [band(95,1800,M(3,11,.35))])
    a("Kick","BB Lofi Cassette Pillow","Soften a brittle sampled kick with a slower dark tape response.","Wow bends the bass pitch or the kick becomes a flat thud.",[hp(30),lp(9000)],[M(1,6,.65,tone=-.25,speed=2),U(-1,.5,.45)])
    a("Snare","BB Classic Wooden Crack","Recover stick attack above a short wooden body.","The 2-4 kHz crack becomes painful or ghost notes disappear.",[hp(110),bell(850,-1.5)],[U(2.5,-1,.65),M(3,5,.3)])
    a("Snare","BB Dusty Sampler Rim","Give a clean rim a restrained sampled edge and dark room.","Aliased grit dominates the rim pitch or the room reaches the next backbeat.",[hp(140),hi(9000,-2)],[M(1,5,.4,sampler=1),V(1,.35,5,.09,2,hp=300,lp=5500)])
    a("Snare","BB Jazzy Ghost Keeper","Control accents gently while leaving ghost notes readable.","Ghost-note/accent contrast collapses or the brush tail pumps.",[hp(95)],[C(-24,1.8,20,180,3,knee=12),V(2,.55,8,.08,3)])
    a("Snare","BB Gold Plate Halo","Put a short plate behind a dry forward backbeat.","The plate hides the first 20 ms or adds low-mid mud.",[hp(110),bell(210,1)],[C(-22,2.5,16,140,1),V(4,.7,24,.13,2,hp=350,lp=6500)])
    a("Snare","BB Basement Parallel Bark","Add a dirty parallel body with the clean crack intact.","The matched snare loses its crest or the bark rings at 600 Hz.",[bell(600,-2,1.3)],[M(4,12,.2),C(-30,8,12,110,2,amount=.3)])
    a("Snare","BB Lofi Brush Room","Make a long brush sample softer and more intimate.","The brush sweep turns into gated chatter.",[hp(140),lp(10000)],[U(-2,-1,.35),V(1,.42,0,.12,3,hp=250,lp=5000)])
    a("Hats","BB Silk Closed Hats","Remove abrasive upper peaks while keeping the timekeeping edge.","The 5-8 kHz accents still spit or every tick becomes dull.",[hp(500),bell(7200,0,1.3,-2.5)],[M(1,3,.35,tone=-.1)])
    a("Hats","BB Dusty Twelve Bit Hats","Add sampled grain above the kick without broadband noise.","Idle gaps gain hiss or the top becomes a continuous hash.",[hp(700),lp(12000)],[M(1,5,.35,sampler=1)])
    a("Hats","BB Jazzy Open Hat Air","Let open hats breathe in a small stereo room.","The mono fold loses the tick or tails smear the swung gaps.",[hp(600),hi(11000,1)],[V(6,.45,4,.08,1,hp=900,lp=10000),W(0,1,1.15,180)])
    a("Hats","BB Swing Echo Tick","Add one quiet swung eighth response after the hat.","A repeat lands on the following main tick and hides the pocket.",[hp(850)],[E(5,.12,.12,2,hp=1000,lp=6000,scoreSwing=.18)])
    a("Hats","BB Soft Offbeat Tuck","Reduce front-edge aggression and push hats behind the snare.","Offbeats become inaudible at the matched level.",[hp(650),hi(8500,-1.5)],[U(-2,.5,.5),F(0,-1)])
    a("Percussion","BB Conga Palm Body","Emphasize hand tone and trim hard slap peaks.","The low hand pitch disappears or slaps get louder.",[hp(100),bell(230,1.5)],[U(-1,1.5,.45),M(2,4,.3)])
    a("Percussion","BB Shaker Back Wall","Place a sharp shaker behind the beat in a dark room.","The shaker sounds closer or the room sustains through every gap.",[hp(950),hi(10000,-2)],[V(1,.38,0,.14,3,hp=1000,lp=5500),W(0,1,1.1,200)])
    a("Percussion","BB Rim Dub Answer","Answer a dry rim with a narrow filtered quarter delay.","The delay carries sub energy or competes with the next snare.",[hp(180)],[E(8,.28,.18,3,hp=600,lp=4200)])
    a("Percussion","BB Last Step Shuffle","Change only the final two sixteenths of a percussion bar.","The downbeat or main backbeat changes.",[hp(180)],[R(.3,glitch={14:6,15:1}),M(1,4,.25)])
    a("Drum Bus","BB Classic Bus Cement","Join the drum body with slow-attack VCA reduction.","The matched kick/snare attacks lose more contrast than the body.",[hp(25)],[C(-22,2,30,167,1),M(1,3,.35)])
    a("Drum Bus","BB Dusty Parallel Basement","Layer dense sampler crunch under a clean drum loop.","Ghost notes vanish or the parallel return overtakes the clean snare.",[bell(350,-1)],[M(1,8,.35,sampler=1),C(-30,6,12,180,2,amount=.25)])
    a("Drum Bus","BB Jazzy Room Glue","Give a sparse jazz break a low-density shared room.","Ride cymbals turn splashy or kick low end grows in the room.",[hp(27)],[C(-24,1.5,35,200,3),V(2,.6,15,.07,3,hp=350,lp=6500)])
    a("Drum Bus","BB Gold Snare Shoulder","Add body only around the snare register and leave the sub alone.","Kick sub grows or the snare becomes boxier after matching.",[bell(470,-1.5)],[C(-21,2,25,155,4)], [band(160,2500,U(1,1,.55,amount=.6))])
    a("Drum Bus","BB Underground Punch Return","Bring short gritty sustain under intact transients.","The matched loop is merely louder or hats dominate the return.",[hi(9500,-.5)],[C(-32,8,15,130,2,amount=.22),M(3,4,.3)])
    a("Drum Bus","BB Lofi Worn Break","Darken a bright break and add restrained tape instability.","The kick pitch wanders audibly or swing timing becomes unclear.",[hp(30),lp(12000)],[M(1,6,.55,tone=-.2,wow=.04),U(-.8,.5,.45)])
    a("Drum Bus","BB Bar End Turn","Keep the groove and make a restrained end-of-bar reply on the tops.","Any low kick note changes or early-bar groove is chopped.",[hp(25)],[C(-23,1.8,28,170)], [band(1200,16000,R(.22,glitch={14:1,15:7}))])
    a("Bass","BB Upright Round Fingers","Level an upright line while retaining finger articulation.","The note tails pump or the finger noise becomes brittle.",[hp(28),bell(280,-1.5)],[C(-25,2.5,28,220,3,source=0),M(2,3,.3)])
    a("Bass","BB Electric Pocket Bass","Tighten an electric bass into the kick pocket with modest harmonics.","The low note sustains into the next kick or pick attack vanishes.",[hp(26),bell(800,1)],[C(-24,3,20,167,1,source=0),M(3,5,.35,subProtectHz=75)])
    a("Bass","BB Sub On Small Speakers","Add upper harmonics while keeping the sub centered.","The 40-80 Hz fundamental changes pitch or the mono fold cancels it.",[hp(24)],[M(2,6,.35,phoneBass=.3,subProtectHz=90),W(0,.9,1,160)])
    a("Bass","BB Tape Note Sustain","Support short bass-note tails with gentle tube compression.","Noise rises between notes or the next attack ducks for too long.",[hp(25),lp(7000)],[C(-26,2.2,30,240,4,source=0),M(1,4,.4)])
    a("Bass","BB Muted Sample Bass","Remove upper spill from a sampled bass phrase and contain its tail.","The line loses pitch definition or the expander chatters on held notes.",[hp(30),lp(2400)],[Z(-48,9,80,180,v2_hp=30,v2_lp=350),C(-23,2,25,190,3,source=0)])
    a("Vocal","BB Dry Front Rap","Keep consonants close with FET control and little ambience.","Sibilance rises or endings are pinned down after loudness matching.",[hp(80),bell(320,-1.5),bell(6500,0,1,-3)],[C(-25,3,8,115,2,3),V(6,.35,18,.045,2,hp=300,lp=6000)])
    a("Vocal","BB Jazzy Intimate Verse","Ride quiet syllables gently before soft opto leveling.","Breaths swell in gaps or phrase dynamics disappear.",[hp(75),hi(11000,.7)],[F(2,target=-21,strength=.35,maxBoost=2,maxCut=4),C(-23,2,18,210,3,3)])
    a("Vocal","BB Tape Slap Story","Add a quiet free-time slap under a warm spoken lead.","The slap is heard as a second speaker or obscures words.",[hp(85),bell(400,-1)],[M(2,3,.25),E(0,0,.15,2,hp=350,lp=4800,tap1Time=105,scoreFeedbackMs=105)])
    a("Vocal","BB Backroom Hook","Place a doubled hook behind the lead with a dark chamber.","The hook moves ahead of the lead or mono consonants blur.",[hp(150),hi(8000,-1)],[C(-26,3,12,170,3,3),W(0,1.15,1.25,180),V(2,.9,10,.15,2,hp=350,lp=6000)])
    a("Vocal","BB Phrase Tail Answer","Let a ducked quarter echo emerge after the rap phrase.","Repeats compete with active words or accumulate across several lines.",[hp(90),bell(6700,0,1.1,-2.5)],[C(-24,2.5,10,135,2,3),E(8,.25,.18,2,scoreDuck=.55,hp=450,lp=5000)])
    a("Keys","BB Felt Chord Bed","Round sharp piano hammers and give chords a small shared chamber.","Hammer definition vanishes or low-mid sustain masks the bass.",[hp(110),bell(330,-1.5)],[U(-1,1,.4),M(1,3,.3),V(2,.8,12,.1,3,hp=300,lp=6500)])
    a("Keys","BB Rhodes Tube Cushion","Add soft even harmonics under a dry electric piano.","Chord peaks fuzz or tremolo depth changes unintentionally.",[hp(90),hi(7000,-1)],[M(2,6,.45,evenOdd=.35),C(-25,2,25,240,3,2)])
    a("Keys","BB Sampler Piano Slice","Give chopped piano a restrained sampled texture without extending gaps.","The slice edge clicks or the silent spaces fill with residue.",[hp(120),lp(10500)],[M(1,5,.4,sampler=2),U(1,-1,.55)])
    a("Keys","BB Wide Chord Reply","Send a filtered eighth answer wide while the original chords remain centered.","The dry piano widens too far or the mono chord loses its root.",[hp(140)],[E(5,.18,.18,3,taps=[(5,0,-7,-.6,0),(8,0,-12,.6,0)])])
    a("Samples","BB Dusty Soul Frame","Darken a bright soul loop and retain the chopped envelope.","The bass becomes unstable or crackle fills the sample gaps.",[hp(65),lp(12500)],[M(1,5,.55,sampler=1,tone=-.12),C(-24,1.8,30,190,3,2)])
    a("Samples","BB Jazzy Horn Seat","Tame a nasal horn loop and seat it in a short room.","The brass loses its entrance or the room enlarges the low end.",[hp(150),bell(1600,-2,1.4)],[C(-22,2,18,165,3,2),V(1,.5,8,.1,2,hp=400,lp=6500)])
    a("Samples","BB Last Word Reverse","Reverse only the final eighth of the sample phrase.","The start of the next bar is reversed or cut off.",[hp(100)],[R(.45,time={14:2,15:2}),M(1,3,.25)])
    a("Samples","BB Filtered Pan Conversation","Move the mids across the phrase while leaving its low anchor steady.","The stereo motion steals the low fundamental or folds down badly.",[hp(70)],[M(1,4,.3)], [band(450,9000,R(.35,shape=(4,[0,0,3,5,8,11,13,15,15,13,11,8,5,3,0,0])))])
    a("Master","BB Classic Open Master","Finish a warm boom-bap premaster with minimal reduction and intact snare.","Final 8x true peak exceeds -1 dBTP or limiter work exceeds 3 dB.",[hp(23),bell(320,-.6)],[M(1,2,.2),C(-19,1.5,40,180,1,0),L(-2,130)])
    a("Master","BB Dusty Album Cohesion","Join a dusty mix gently without raising noise or narrowing the sample bed.","Idle noise grows or the matched chorus loses snare contrast.",[hp(25),hi(11000,-.4)],[C(-21,1.6,35,200,3,0),M(3,2,.15),L(-2.5,145)])
    a("Master","BB Jazzy Headroom Master","Preserve spacious live dynamics with a conservative final ceiling.","Limiter work exceeds 2 dB or the room pulls inward on kick hits.",[hp(22)],[C(-18,1.3,45,230,3,0),W(.95,1,1.05,100),L(-1,180)])
    a("Master","BB Gold Low End Frame","Center only the low end and trim a crowded low-mid band before limiting.","Bass fold-down changes notes or the kick loses low-end authority.",[hp(24),bell(260,-.8,.8)],[W(0,1,1.08,110),C(-20,1.5,35,167,1,0),L(-2.2,120)])

    a=bank("Trap",140)
    a("Kick","Trap ATL Click Over Eight","Create a compact upper click that separates the kick from an 808.","The boost adds sub masking or click becomes a sharp spit.",[hp(35),bell(380,-2),bell(3200,1.5)],[U(2,-1.5,.75),C(-21,2.5,14,85)])
    a("Kick","Trap Dark Low Hammer","Keep a dark low kick weighty and shorten its overlap with the bass.","The low note rings beyond a sixteenth or loses its initial weight.",[hp(26),hi(6500,-1.5)],[Z(-40,14,35,55,2),M(3,6,.35,subProtectHz=55)])
    a("Kick","Trap Cloud Soft Knock","Make a hard kick gentler for a floating melodic beat.","The kick disappears under the pad or gains extra low-tail sustain.",[hp(30),bell(2800,-1.5)],[U(-1.5,-.5,.55),M(2,3,.25)])
    a("Kick","Trap Rage Mid Bite","Add controlled midrange edge while holding the kick sub clean.","The dry sub distorts or the added edge resembles broadband noise.",[hp(28),bell(450,-2)],[M(4,8,.25,subProtectHz=90),U(1.5,-1,.8)])
    a("Kick","Trap Memphis Short Crunch","Make a short kick feel sampled and compressed without a long release.","The next hat ducks or the sampled edge dominates the attack.",[hp(32),lp(11000)],[M(1,5,.35,sampler=3),C(-25,4,7,65,2,amount=.45)])
    a("Kick","Trap Luxury Clean Heel","Tidy the kick low mids with a clean high-crest transient.","The first 10 ms lose contrast or the low shelf overfills the 808.",[hp(27),bell(240,-1.8),lo(65,.7)],[U(1,-1,.65),C(-20,1.8,22,105)])
    a("Snare","Trap ATL Clap Stack","Bring clap onset forward while reducing long upper tails.","The stack turns into a click or flam spacing becomes unclear.",[hp(180),bell(1000,-1)],[U(2,-2,.8),M(3,4,.25)])
    a("Snare","Trap Dark Rim Needle","Focus a narrow rim without exaggerating its nasal ring.","The 900-1500 Hz ring persists or rim attack becomes harsh.",[hp(200),bell(1250,-2.5,1.8)],[C(-23,2.5,5,70,2),V(6,.28,8,.055,4,hp=600,lp=5500)])
    a("Snare","Trap Cloud Air Clap","Spread the airy clap tail around a dry center.","The mono clap loses its center or air hangs into the next snare.",[hp(220),bell(7800,0,1,-2)],[V(4,.65,18,.12,3,hp=600,lp=10000),W(0,1.1,1.35,250)])
    a("Snare","Trap Rage Distorted Backbeat","Blend a rough upper snare layer while keeping body clean.","Low body flattens or distortion raises the matched peak excessively.",[hp(120)],[U(1.5,-.5,.8)], [band(900,11000,M(4,12,.25),C(-29,6,4,60,2,amount=.4))])
    a("Snare","Trap Memphis Gated Plate","End a synthetic plate sharply after the clap.","Plate opening swallows the dry clap or the gate snaps audibly.",[hp(160)],[M(1,4,.25),V(4,.8,10,.16,2,hp=500,lp=6500,timeShape=1,shapeTimeMs=150,gateReleaseMs=25)])
    a("Snare","Trap Luxury Snap Body","Add a little body below a bright clean snap.","The lower body masks vocal low mids or snap loses height.",[hp(130),bell(220,1.2),bell(4300,-1)],[U(1,1,.65),C(-22,2,16,90,1)])
    a("Hats","Trap ATL Roll Definition","Keep rapid ticks separate by shortening bodies rather than boosting treble.","Sixteenth rolls fuse or the hats become brighter after matching.",[hp(950),bell(8500,0,1.2,-2)],[U(.7,-2,.85)])
    a("Hats","Trap Dark Metal Tame","Suppress sharp metal resonances while retaining quiet roll notes.","Accent resonance still rings or low-velocity notes are gated out.",[hp(700),bell(6500,0,2,-3.5)],[C(-26,2,2,45,1,source=0,knee=10)])
    a("Hats","Trap Cloud Wide Tick","Place only high hat energy wider with a stable dry center.","Mono fold loses the roll or the center moves between hits.",[hp(1100)],[W(0,1,1.3,300,v2Generate=.12,v2Generator=2),V(6,.3,0,.045,3,hp=1000,lp=9000)])
    a("Hats","Trap Rage End Roll","Add one short BLOCK roll at the last sixteenth.","The roll fires on an earlier beat or cuts the next downbeat.",[hp(1000)],[R(.35,glitch={15:2},glitchDepth=.35)])
    a("Hats","Trap Triplet Echo Ghost","Put a quiet triplet response around straight hats.","The triplet becomes louder than the original or timing sounds unsynced.",[hp(1200)],[E(4,.1,.1,0,hp=1400,lp=8500)])
    a("Percussion","Trap Bell Ping Seat","Seat a hard bell with a damped offset reply.","The reply clashes with the bell pitch or overtakes the next strike.",[hp(350),bell(2400,-1.5)],[M(2,3,.2),E(6,.18,.12,1,hp=600,lp=5500)])
    a("Percussion","Trap Memphis Cowbell Grit","Add a sampled rough edge with its fundamental held steady.","The cowbell pitch wavers or upper aliasing dominates.",[hp(250),lp(10000)],[M(3,7,.35,sampler=2),U(.5,-1,.75)])
    a("Percussion","Trap Texture Stereo Dots","Separate dry percussion from small left-right echoes.","The echo shifts the dry event or mono loses its attack.",[hp(500)],[E(5,.1,.15,0,taps=[(5,0,-9,-.7,0),(8,0,-14,.7,0)])])
    a("Percussion","Trap Short Reverse Accent","Reverse the final small percussion gesture only.","A full bar reverses or the following bar starts late.",[hp(300)],[R(.4,time={15:2},timeMix=.7),V(6,.25,0,.05,4,hp=800,lp=6500)])
    a("Drum Bus","Trap ATL Dry Glue","Bind kick and snare with a fast recovery while preserving the front edge.","The 808 slot in the groove feels smaller or hats pump on kicks.",[hp(25),bell(350,-.7)],[C(-22,2,22,100,1),M(3,3,.2)])
    a("Drum Bus","Trap Dark Parallel Pressure","Add compressed mid-body under clean drum peaks.","The dry crest drops or the parallel path raises cymbal wash.",[bell(500,-1)],[C(-30,6,10,85,2,amount=.25)], [band(120,6500,M(2,7,.25))])
    a("Drum Bus","Trap Cloud Open Tops","Keep low drums centered and give upper percussion modest air.","The kick changes in mono or the top overshadows the vocal.",[hp(28),hi(10500,.5)],[C(-21,1.5,30,125,3),W(0,1.05,1.2,150)])
    a("Drum Bus","Trap Rage Hit Contrast","Increase hit/body contrast without adding full-range clipping.","The loudest snare clips or quieter ghost hits vanish.",[hp(30)],[U(1.5,-1,.8,1,amount=.65),M(3,4,.25,subProtectHz=100)])
    a("Drum Bus","Trap Memphis Sample Bus","Make a clean drum bus cohere around a restrained sampled midrange.","The entire beat loses high-frequency separation.",[hp(28),lp(14000)],[M(1,5,.35,sampler=3),C(-23,2.5,18,95,1)])
    a("Drum Bus","Trap Last Beat Chop","Chop only the upper drum band at the last two subdivisions.","The low kick is gated or the chop bleeds into the next bar.",[hp(25)],[C(-21,1.7,25,100)], [band(800,16000,R(.3,glitch={14:4,15:2},glitchDepth=.4))])
    a("Drum Bus","Trap Luxury Peak Discipline","Contain occasional snare spikes with short limiter recovery.","Every kick is limited or the next hit remains attenuated.",[hp(26)],[C(-20,1.6,28,110,1),L(-1,22)])
    a("Bass","Trap ATL Eight Translation","Generate harmonics above a clean 808 fundamental.","Glides become rough below 100 Hz or the bass loses mono stability.",[hp(24)],[M(2,6,.4,phoneBass=.35,subProtectHz=100),W(0,.95,1,180)])
    a("Bass","Trap Dark Long Eight","Level long 808 notes without compressing away the initial drop.","The note breathes audibly or the glide tail is cut short.",[hp(22),bell(240,-1)],[C(-25,2.5,35,180,3,0),M(3,4,.3,subProtectHz=70)])
    a("Bass","Trap Cloud Soft Sub","Soften upper buzz on a melodic 808 while maintaining pitch.","The bass vanishes on a small speaker or low notes become quieter.",[hp(24),lp(6500)],[M(1,3,.3,tone=-.2,phoneBass=.12,subProtectHz=85),C(-23,1.7,30,210,3,0)])
    a("Bass","Trap Rage Upper Growl","Distort the bass upper band while leaving the sub undriven.","The fundamental clips or the growl covers the kick click.",[hp(25)],[W(0,1,1.05,170)], [band(150,5000,M(4,13,.4),C(-28,4,8,80,2,0))])
    a("Bass","Trap Memphis Short Eight","Contain an 808 tail and add compact sampled midrange.","The envelope clicks on low notes or ends before pitch is established.",[hp(25),lp(9000)],[Z(-46,12,90,130,v2_hp=25,v2_lp=400),M(3,5,.3,sampler=2,subProtectHz=65)])
    a("Vocal","Trap ATL Front Lead","Hold a dry rap vocal forward with fast FET control and restrained air.","Sibilants rise or the next word ducks after a shout.",[hp(85),bell(350,-1.5),bell(7200,0,1.1,-3)],[F(2,strength=.45,maxBoost=2),C(-25,3.5,6,90,2,3),M(2,2,.15)])
    a("Vocal","Trap Cloud Floating Hook","Let a ducked bright hall bloom after the hook words.","Hall covers active consonants or low reverb overlaps the 808.",[hp(110),bell(6500,0,1,-2.5)],[C(-24,2.2,14,170,3,3),V(3,1.6,38,.18,3,hp=450,lp=10000,duck=.5)])
    a("Vocal","Trap Dark Telephone Adlib","Band-limit an adlib and place a gritty echo behind it.","Words lose intelligibility or the echo runs indefinitely.",[hp(420),lp(4200),bell(1500,1)],[M(4,7,.25),E(6,.25,.18,3,hp=650,lp=3500)])
    a("Vocal","Trap Rage Parallel Shout","Add rough density under a controlled shout without boosting ess sounds.","The shout sounds louder only because of unmatched level or its esses tear.",[hp(100),bell(7000,0,1,-4)],[C(-27,5,4,70,2,3,amount=.5),M(4,9,.2),V(6,.3,12,.05,4,hp=500,lp=6000)])
    a("Vocal","Trap Luxury Pitch Replies","Place a quiet fifth-up reply after the dry phrase.","The pitched reflection sounds like a doubled lead during words.",[hp(95),bell(6800,0,1,-2.5)],[C(-23,2.5,12,120,3,3),E(6,.18,.12,0,taps=[(6,7,-12,-.35,0)],scoreDuck=.6)])
    a("Keys","Trap Cloud Bell Bed","Keep bell attack clear above a dark softly spreading tail.","The bell loses pitch or ambience hides the next arpeggio note.",[hp(180),bell(4000,-1)],[U(-.5,.5,.6),V(3,1.3,30,.16,3,hp=550,lp=8500),W(0,1.1,1.25,200)])
    a("Keys","Trap Dark Piano Space","Remove muddy piano sustain and add a dark chamber.","The piano bottom covers the 808 or the chamber sounds detached.",[hp(150),bell(350,-2),hi(10000,-1)],[C(-24,2,22,180,3,2),V(2,1.1,24,.15,5,hp=400,lp=5500)])
    a("Keys","Trap Rage Pluck Edge","Add controlled iron harmonics to a hard synth-key pluck.","The pluck becomes broadband distortion or masks consonants.",[hp(180),bell(2800,-1)],[M(3,7,.35,dynamics=.3),U(1,-1,.8)])
    a("Keys","Trap Luxury Eighth Sparkle","Place light dotted-eighth answers around electric keys.","The echo shifts the chord root or dominates the original pattern.",[hp(140)],[M(2,3,.2),E(6,.18,.13,0,hp=700,lp=9500),W(0,1.05,1.2,200)])
    a("Samples","Trap Memphis Tape Scene","Make a sample darker and more compact without adding continuous noise.","Idle gaps gain hum or the chopped rhythm stops feeling clear.",[hp(120),lp(11000)],[M(1,6,.45,sampler=1),C(-24,2,20,120,3,2)])
    a("Samples","Trap Cloud Octave Mist","Add sparse octave-up echo with a quiet soft space tail.","The octave becomes the dominant melody or feedback changes the root repeatedly.",[hp(160)],[E(8,.16,.14,0,taps=[(8,12,-14,.4,0)],scoreSpaceMix=.18,tap1SpaceSend=.5,scoreSpaceDecay=1.8)])
    a("Samples","Trap Last Word Tape Stop","Stop the final sample subdivision and resume cleanly on the downbeat.","The next bar starts pitched down or has missing samples.",[hp(90)],[R(.65,time={15:4},timeStretch=0)])
    a("Samples","Trap Gated Melody Pocket","Carve a syncopated melody envelope around the kick and snare.","Gate edges click or the rhythm obscures the melody's main accents.",[hp(180)],[rhythm([1,.6,0,.4,1,0,.6,0,1,.5,0,.7,1,0,.5,0],amount=.65),V(6,.35,0,.06,3,hp=500,lp=8000)])
    a("Master","Trap ATL Sub Anchor","Keep the sub mono and let kick and vocal peaks survive gentle limiting.","Final 8x true peak exceeds -1 dBTP or bass distorts before 3 dB limiter work.",[hp(22),bell(280,-.6)],[W(0,1,1.06,130),C(-18,1.4,35,107,1,0),L(-2.2,90)])
    a("Master","Trap Cloud Air Master","Open a soft hook mix while containing upper harshness dynamically.","The airy band hisses or the matched vocal becomes recessed.",[hp(23),hi(11500,.5),bell(7200,0,1,-1.5)],[C(-19,1.4,40,160,3,0),W(.9,1.03,1.1,120),L(-1.8,125)])
    a("Master","Trap Dark Density Master","Support midrange density without saturating the deepest 808.","Sub harmonics buzz audibly or limiter work exceeds 3 dB.",[hp(22),bell(420,-.6)],[M(3,3,.18,subProtectHz=110),C(-20,1.6,30,110,1,0),L(-2.5,95)])
    a("Master","Trap Luxury Clean Ceiling","Finish a balanced clean mix with little colour and bounded stereo width.","Mono fold loses the hook or limiting changes the 808 envelope.",[hp(24)],[W(0,1.02,1.07,140),C(-17,1.3,45,140,1,0),L(-1.5,105)])

    a=bank("Techno",132)
    a("Kick","Techno Peak Front Pillar","Keep a four-on-floor kick firm with low-mid space and an intact transient.","The next quarter loses impact or low-mid buildup remains.",[hp(26),bell(300,-2)],[U(1.5,-1,.75),C(-22,2.5,20,114)])
    a("Kick","Techno Hypnotic Round Pulse","Round a sharp kick and let its low body carry the pulse.","The upper click still dominates or low body blooms past the next beat.",[hp(25),bell(4200,-2)],[M(1,5,.4,subProtectHz=55),U(-.8,-.5,.6)])
    a("Kick","Techno Minimal Dry Foot","Remove excess tail and leave room for a sparse bass pattern.","The shortened envelope clicks or the kick becomes hollow.",[hp(30),bell(420,-1.5)],[Z(-42,16,35,60,2),U(.7,-1.5,.8)])
    a("Kick","Techno Detroit Iron Center","Give a clean electronic kick restrained iron midrange.","The fundamental buzzes or harmonics bury the hi-hat.",[hp(25),lo(70,.7)],[M(3,7,.4,subProtectHz=80),C(-21,2,24,125)])
    a("Kick","Techno Dub Dark Rumble Seed","Derive a filtered room bloom from the kick without a long bright tail.","The bloom builds across bars or destabilizes the dry kick pitch.",[hp(28)],[M(1,4,.3)], [band(70,420,V(1,.6,20,.14,5,hp=80,lp=1800))])
    a("Kick","Techno Hard Parallel Hammer","Blend aggressive upper kick colour below a clean low anchor.","The sub is distorted or the matched transient loses punch.",[hp(26),bell(550,-2)],[U(1,-1,.8)], [band(130,4500,M(4,15,.35),C(-29,5,10,75,2,amount=.45))])
    a("Snare","Techno Peak Clap Hit","Emphasize clap attack and pull its tail off the next hat.","The clap turns papery or the hat space remains crowded.",[hp(150),bell(600,-1.5)],[U(2,-1.5,.8),C(-22,2.5,12,95)])
    a("Snare","Techno Hypnotic Rim Chamber","Place a dry rim in a small dark chamber with a clear center.","The chamber becomes a second rhythmic event or dulls the rim.",[hp(200),bell(1600,-1)],[M(2,4,.25),V(2,.55,12,.12,2,hp=650,lp=5000)])
    a("Snare","Techno Minimal Tight Snap","Contain a synthetic snap with a fast drum envelope.","Quiet hits disappear or the release adds chatter.",[hp(220),bell(8000,0,1.2,-2)],[Z(-43,12,20,45,2),U(1,-1,.85)])
    a("Snare","Techno Detroit Short Plate","Give a machine snare a metallic but bounded plate tail.","The plate obscures the clap grid or low mids ring.",[hp(140),bell(280,1)],[M(3,4,.25),V(4,.65,15,.14,4,hp=500,lp=7500)])
    a("Snare","Techno Dub Snare Throw","Send a damped offbeat repeat behind a dry snare.","The repeat makes the next snare late or feedback grows each bar.",[hp(160)],[E(6,.38,.18,3,hp=700,lp=4000,scoreDuck=.35)])
    a("Snare","Techno Hard Gate Burst","Add a short dense room burst behind a hard snare.","The gate clicks or its burst masks the initial crack.",[hp(130),bell(450,-1.5)],[C(-25,4,8,80,2,amount=.6),V(1,.9,10,.14,4,hp=500,lp=6500,timeShape=1,shapeTimeMs=110,gateReleaseMs=18)])
    a("Hats","Techno Peak Open Hat Seat","Shorten overlong offbeat hats while keeping their high lift.","The open hat loses its offbeat accent or still masks the next kick.",[hp(650),bell(8000,0,1,-2)],[U(.5,-2,.75),M(1,3,.2)])
    a("Hats","Techno Hypnotic Metallic Dots","Keep a repetitive closed hat focused with subtle dark echoes.","Echoes replace the main tick or create a constant top layer.",[hp(1100),hi(12500,-1)],[E(2,.08,.08,3,hp=1400,lp=5000)])
    a("Hats","Techno Minimal Tick Transient","Remove body from a small machine tick without expanding stereo width.","The tick becomes a click artifact or vanishes on quiet passages.",[hp(1200),lp(14000)],[U(1,-2.5,.85),F(0,-.5)])
    a("Hats","Techno Detroit Tape Air","Round brittle hats and keep a modest high-band stereo spread.","Mono loses timekeeping or tape tone dulls every accent.",[hp(800)],[M(1,4,.3,tone=-.1),W(0,1,1.2,300)])
    a("Hats","Techno Hard Last Step Rush","Add one rising BLOCK burst at the phrase edge.","The rise repeats every early beat or the next downbeat is cut.",[hp(1000)],[R(.3,glitch={14:3,15:2},glitchDepth=.3)])
    a("Percussion","Techno Hypnotic Tom Focus","Keep low tom notes distinct and soften excessive stick bite.","The tom tail overlaps the next note or fundamental shifts.",[hp(65),bell(1800,-1.5)],[U(-.5,-1.5,.65),C(-24,2,22,114,1,0)])
    a("Percussion","Techno Minimal Wood Reply","Add a quiet short delay around a dry wooden hit.","The reply sounds like a flam on the original transient.",[hp(300),bell(900,1)],[E(3,.12,.1,1,hp=600,lp=6000)])
    a("Percussion","Techno Dub Chord Perc Space","Create a filtered ping-pong response with a stable dry hit.","The feedback creates low buildup or the dry center shifts.",[hp(250)],[E(6,.4,.2,3,hp=500,lp=4200,taps=[(6,0,-8,-.6,0),(8,0,-13,.6,0)])])
    a("Percussion","Techno Industrial Mid Scrape","Add parallel hard colour to metallic percussion without brittle treble.","The upper band turns into hiss or the clean metal pitch disappears.",[hp(350),lp(10000)],[M(4,14,.25),C(-27,4,6,65,2,amount=.4)])
    a("Drum Bus","Techno Peak Quarter Glue","Let the compressor recover inside a quarter while keeping the kick front.","The detector pumps the hats excessively or all kicks flatten.",[hp(25),bell(320,-.7)],[C(-22,2,28,114,1),M(3,3,.2)])
    a("Drum Bus","Techno Hypnotic Tape Bed","Bind a rolling machine beat with modest tape density.","Loop accents lose their hierarchy or low end turns woolly.",[hp(26),hi(12000,-.5)],[M(1,4,.35),C(-23,1.8,30,170,3)])
    a("Drum Bus","Techno Minimal Clean Space","Control the envelope lightly while preserving sparse gaps.","Gate noise replaces the gaps or the hats sound closer than the clap.",[hp(28)],[C(-20,1.5,35,130,1),V(6,.25,3,.035,1,hp=900,lp=6500)])
    a("Drum Bus","Techno Detroit Parallel Circuit","Add an energetic mid-band parallel layer beneath clean machine transients.","The snare crest falls or the layer muddies the kick-bass pocket.",[hp(26)],[C(-29,6,15,100,2,amount=.25)], [band(170,6000,M(3,8,.35))])
    a("Drum Bus","Techno Dub Tops Feedback","Create a dark upper-band delay bed while the low rhythm stays dry.","The low kick is echoed or feedback outlasts the intended phrase gaps.",[hp(25)],[C(-22,1.7,30,150)], [band(700,12000,E(6,.32,.12,3,hp=800,lp=4000))])
    a("Drum Bus","Techno Hard Controlled Density","Increase parallel drum density with protected transient contrast.","The matched beat loses attack/body contrast or master peaks jump above plan.",[hp(28),bell(400,-1)],[U(1.2,-.5,.8,1,amount=.6),M(4,8,.18),C(-27,4,15,90,2,amount=.3)])
    a("Drum Bus","Techno Phrase Tops Shuffle","Shuffle the final upper-drum fragments while preserving the four-on-floor.","The kick timing changes or the next bar lacks its first hat.",[hp(25)],[C(-22,1.8,25,115)], [band(900,16000,R(.35,glitch={12:6,14:1,15:2}))])
    a("Bass","Techno Peak Rolling Bass","Level a rolling line without blurring its sixteenth-note spaces.","The bass sustains into the kick or loudness matching reveals less articulation.",[hp(28),bell(280,-1.5)],[C(-24,3,18,90,1,0),M(3,4,.3,subProtectHz=80)])
    a("Bass","Techno Hypnotic Tempo Dip","Cut a gentle tempo-shaped pocket at the start of each quarter.","The dip misses the grid or the bass fails to recover before the offbeat.",[hp(25)],[motion(6,1,[0,.35,.85,1]),M(1,4,.25,subProtectHz=70)])
    a("Bass","Techno Acid Upper Bite","Add controlled resonance-band grit above an undriven sub.","The filter peak tears or the low fundamental is saturated.",[hp(30),bell(3200,0,1.8,-2)],[W(0,1,1,140)], [band(180,7000,M(4,9,.35),C(-25,3,6,85,2,0))])
    a("Bass","Techno Dub Round Sustain","Smooth a soft dub-bass line without generating extra low tails.","Notes swell after their attack or low sub level becomes inconsistent.",[hp(24),lp(4000)],[C(-25,2.5,30,210,3,0),M(2,4,.3,subProtectHz=65)])
    a("Bass","Techno Hard Mono Grind","Add a compact mid grind with mono low end and short releases.","Stereo artifacts appear below 150 Hz or the bass overshadows the kick click.",[hp(28)],[M(3,9,.4,subProtectHz=100,phoneBass=.1),C(-24,3.5,15,85,1,0),W(0,1,1.05,160)])
    a("Vocal","Techno Spoken Close Command","Keep a spoken phrase dry, focused and stable in front of a loud beat.","Breaths surge between commands or the bandpass destroys intelligibility.",[hp(130),bell(450,-2),bell(6500,0,1,-2.5)],[F(2,strength=.4,maxBoost=2),C(-25,3,7,100,2,3)])
    a("Vocal","Techno Dub Word Echo","Let a damped dotted echo emerge between spoken words.","Repeats obscure commands or build endlessly at phrase boundaries.",[hp(180),lp(10000)],[C(-23,2.5,10,130,1,3),E(6,.38,.2,3,scoreDuck=.6,hp=650,lp=4500)])
    a("Vocal","Techno Gated Chant","Shape a held chant into a repeating syncopated figure.","Gate edges click or consonants become impossible to identify.",[hp(160),bell(7000,0,1,-3)],[rhythm([1,1,0,.4,1,0,.6,0,1,.7,0,0,1,0,.7,0],amount=.75),V(1,.4,8,.07,4,hp=600,lp=5500)])
    a("Vocal","Techno Warehouse Call","Put a clear dry call ahead of a ducked dark warehouse tail.","The tail masks the next call or adds low-mid boom.",[hp(140),bell(300,-1.5)],[C(-25,3,10,115,2,3),V(3,1.8,45,.19,5,hp=550,lp=5000,duck=.55)])
    a("Vocal","Techno Last Word Throw Lane","Send only the final two steps to a Revolution echo throw.","The whole phrase is repeated or the throw has a pitched leading artifact.",[hp(150),bell(6000,0,1,-2)],[R(.4,throw=(0,{14:10,15:15}))])
    a("Keys","Techno Detroit Chord Punch","Keep a chord stab short, warm and distinct from the kick.","Chord body spills into the next stab or the top becomes fuzzy.",[hp(180),bell(450,-1.5)],[U(1,-1.5,.7),M(3,5,.3),C(-24,2,18,115,1,2)])
    a("Keys","Techno Dub Chord Tunnel","Create a damped syncopated chord echo with a small chamber return.","Low repeats accumulate or the dry chord loses its placement.",[hp(220)],[E(6,.42,.2,3,hp=450,lp=4000),V(2,.9,16,.08,2,hp=600,lp=5000)])
    a("Keys","Techno Minimal Pluck Seat","Tuck bright plucks behind the rhythm with restrained early reflections.","The plucks become diffuse or sharp accents still dominate.",[hp(200),bell(5000,0,1.4,-2)],[U(-.5,-.5,.7),V(6,.3,5,.06,1,hp=800,lp=7000)])
    a("Keys","Techno Peak Chord Breathing","Give a sustained chord a gentle beat-linked dip and recovery.","The dip sounds like an external-key promise or makes the chord disappear.",[hp(160),bell(350,-1.5)],[motion(5,1,[0,.55,1,1]),W(0,1.1,1.2,200),V(3,1.2,25,.08,3,hp=500,lp=8000)])
    a("Samples","Techno Hypnotic Filter Step","Move only the sample upper mids through a repeating low-pass gesture.","Its low pulse moves or filter steps click audibly.",[hp(100)],[M(1,3,.2)], [band(600,14000,R(.45,motion_steps=[0,0,2,4,6,8,10,12,12,10,8,6,4,2,0,0],target=1))])
    a("Samples","Techno Industrial Broken Loop","Add two explicit BLOCK chops near the end of an industrial loop.","The chop density obscures the loop or carries into the following bar.",[hp(160),lp(13000)],[M(4,8,.2),R(.35,glitch={10:4,14:1,15:9})])
    a("Samples","Techno Dub Stereo Field","Give an ambient texture width and a slow beat-linked pan journey.","The mono fold loses the texture or the low anchor shifts sides.",[hp(180)],[W(0,1.15,1.3,220,v2PositionOn=1,v2Motion=1,v2Unit=1,v2Nodes=2,v2Node0Angle=-35,v2Node1Angle=35,v2Node0HoldBeat=1,v2Node1HoldBeat=1,v2Node0TravelBeat=3,v2Node1TravelBeat=3)])
    a("Samples","Techno One Beat Reverse Swell","Add a quiet reverse-shaped reverb approaching the next sample hit.","The effect changes the dry timing or arrives after its target hit.",[hp(200)],[V(3,1.2,0,.2,6,hp=700,lp=6500,timeShape=2,shapeSync=10,gateReleaseMs=30)])
    a("Master","Techno Peak Clean Pressure","Finish a club premaster with a stable low center and controlled limiter work.","Final 8x true peak exceeds -1 dBTP or kick flattening begins before 3 dB work.",[hp(24),bell(330,-.7)],[C(-19,1.5,40,114,1,0),W(0,1,1.06,110),L(-2.4,100)])
    a("Master","Techno Hypnotic Longform","Preserve small evolving accents while adding gentle tape cohesion.","Long compression causes audible hat pumping or subtle accents disappear.",[hp(23),hi(13000,-.3)],[M(1,2,.15),C(-20,1.4,45,190,3,0),L(-1.8,145)])
    a("Master","Techno Dub Depth Master","Keep dub space deep and protect low-mid headroom.","The room folds inward on kicks or low delay returns dominate the mix.",[hp(25),bell(220,-.8,.8)],[W(.9,1.02,1.08,120),C(-19,1.4,40,170,1,0),L(-1.6,150)])
    a("Master","Techno Hard Transient Reserve","Keep aggressive midrange while reserving clean space for kick peaks.","The hard character vanishes or limiting adds low-note distortion.",[hp(26),bell(4000,0,1.2,-1.5)],[C(-18,1.5,35,95,1,0),M(3,2,.12,subProtectHz=120),L(-2.8,85)])

    a=bank("Drum and Bass",174)
    a("Kick","DnB Modern Fast Punch","Keep a short kick attack distinct inside a fast break.","The body masks the next ghost hit or click loses definition.",[hp(30),bell(300,-2),bell(2700,1)],[U(2,-1.5,.9),C(-22,2.5,14,65)])
    a("Kick","DnB Roller Low Pocket","Tighten a rolling kick tail without stripping its low-note weight.","The low note is cut before a full cycle or the tail overlaps the bass.",[hp(25),bell(420,-1.5)],[Z(-42,12,32,45,2),M(3,4,.25,subProtectHz=65)])
    a("Kick","DnB Liquid Soft Front","Soften an aggressive kick for an open liquid arrangement.","The softened kick disappears under bass or gains excess sustain.",[hp(28),bell(3400,-1)],[U(-1.2,-.5,.6),C(-21,1.8,24,100,3)])
    a("Kick","DnB Neuro Mid Click","Add a compact mid click above clean low-end energy.","Upper distortion masks the snare or the sub becomes gritty.",[hp(28)],[U(1,-1,.85)], [band(500,6000,M(3,7,.3),C(-24,3,5,55,2,0))])
    a("Kick","DnB Jump Up Solid Knock","Emphasize the audible low-mid punch without bloated sub sustain.","The 100-180 Hz area clouds the bass line or the front edge clips.",[hp(32),bell(130,1),bell(400,-2)],[U(1.5,-2,.85),M(2,5,.3)])
    a("Kick","DnB Break Resampled Foot","Blend sampled texture into an acoustic kick with fast release.","Texture erases low-velocity hits or the kick sounds continuously crushed.",[hp(30),lp(12000)],[M(1,4,.25,sampler=3),C(-24,3,10,60,1,amount=.5)])
    a("Snare","DnB Modern Snare Whip","Increase attack/body contrast without making the snare louder by default.","The matched hit lacks crack or the upper snap becomes painful.",[hp(125),bell(700,-1.5)],[U(2.5,-1.5,.9,1,amount=.75),M(3,4,.25)])
    a("Snare","DnB Roller Dry Backbeat","Trim a lingering snare tail while preserving ghost notes.","The gate removes ghost notes or the snare tail still covers the next hat.",[hp(130),bell(240,1)],[Z(-47,10,30,55,2),C(-22,2.2,14,70,1)])
    a("Snare","DnB Liquid Chamber Lift","Put a small dark chamber behind a clean light snare.","The room overtakes ghosts or moves the backbeat behind the groove.",[hp(150),hi(11500,.5)],[U(1,.5,.7),V(2,.55,18,.11,3,hp=500,lp=7500)])
    a("Snare","DnB Neuro Parallel Snap","Add a narrow rough snare edge under the original dry hit.","The dry hit flattens or 3-6 kHz roughness dominates.",[hp(120),bell(4800,0,1.4,-2.5)],[C(-29,6,6,55,2,amount=.3),M(4,9,.18)])
    a("Snare","DnB Jump Up Short Plate","Give a machine backbeat size with a quickly gated plate.","Plate persists into the next sixteenth or sounds like an abrupt click.",[hp(140),bell(260,1.2)],[U(1,-1,.85),V(4,.65,12,.14,4,hp=650,lp=6500,timeShape=1,shapeTimeMs=95,gateReleaseMs=15)])
    a("Snare","DnB Break Ghost Detail","Level hard accents gently enough to expose break ghost articulation.","Accent/ghost ratio collapses or ghosts start pumping.",[hp(110),bell(900,-1)],[C(-25,2,18,86,3,knee=12),M(1,3,.2)])
    a("Hats","DnB Modern Fast Silk","Reduce upper bite while keeping dense rolls legible.","Roll notes merge or the transient becomes duller than the body.",[hp(950),bell(8500,0,1.2,-2.5)],[U(.5,-1.5,.95)])
    a("Hats","DnB Roller Shuffled Tops","Add a quiet eighth-triplet ghost around a straight top loop.","The ghost crowds the snare or sounds like free-time drift.",[hp(1100)],[E(4,.08,.075,1,hp=1600,lp=6500)])
    a("Hats","DnB Liquid Wide Air","Widen soft cymbal air while keeping the low-mid stereo anchor.","The mono fold loses the hat pulse or the room smears fast notes.",[hp(800),hi(12000,.6)],[W(0,1,1.25,300),V(6,.28,2,.045,3,hp=1000,lp=11000)])
    a("Hats","DnB Neuro Metallic Tuck","Control a hard metallic top resonance with short envelope action.","The resonance rings past the next subdivision or ticks vanish.",[hp(1000),bell(6400,0,2,-3)],[C(-25,2.5,1,35,1,0,knee=8),U(-.5,-.5,.9)])
    a("Hats","DnB Break End Ratchet","Put a single BLOCK roll at the end of the top phrase.","The roll affects the low kick or repeats across the following bar.",[hp(1100)],[R(.28,glitch={15:2},glitchDepth=.25)])
    a("Percussion","DnB Modern Tom Separation","Shorten tom tails and expose their attack inside a busy fill.","The tom pitch becomes too short to identify or the fill loses velocity detail.",[hp(70),bell(500,-1.5)],[U(1.5,-2,.85),Z(-46,10,30,50,2)])
    a("Percussion","DnB Roller Wood Groove","Keep woody syncopations compact with a modest iron body.","The loop loses swing articulation or low-mid body clouds the snare.",[hp(250),bell(1300,-1)],[M(3,4,.3),C(-24,2,12,75,1)])
    a("Percussion","DnB Liquid Bell Glints","Place two quiet stereo bell responses around a dry event.","Pitched bell identity changes or mono loses the original attack.",[hp(400)],[E(5,.12,.1,0,taps=[(5,0,-11,-.5,0),(8,0,-15,.5,0)],hp=700,lp=9000)])
    a("Percussion","DnB Neuro Fragment Answer","Repeat the final percussion fragment once with a restrained wet throw.","The phrase becomes a continuous glitch or the next downbeat is obscured.",[hp(350)],[R(.35,glitch={14:1},throw=(0,{15:9}))])
    a("Drum Bus","DnB Modern Crest Glue","Glue a fast break with release short enough to recover before the next hit.","Matched crest shrinks too far or the compressor stays down between hits.",[hp(26),bell(350,-.6)],[C(-22,2,24,70,1),U(.8,-.5,.8,amount=.6)])
    a("Drum Bus","DnB Roller Parallel Weight","Add mid-body weight under a clean rolling break.","Kick/snare attacks flatten or the parallel path lifts hat wash.",[hp(25)],[C(-29,6,12,65,2,amount=.25)], [band(130,5500,M(3,6,.3))])
    a("Drum Bus","DnB Liquid Open Break","Retain room dynamics and add only a small high-band spread.","The snare loses its location or mono cymbals thin out.",[hp(28),bell(430,-.7)],[C(-21,1.6,30,105,3),W(0,1.03,1.16,150),V(6,.25,5,.035,3,hp=1000,lp=9500)])
    a("Drum Bus","DnB Neuro Tight Drum Wall","Shorten drum bodies before a restrained parallel hard-colour layer.","Ghost notes vanish or sharp edges overshoot the peak-growth plan.",[hp(28),bell(4800,0,1,-1.5)],[U(1,-1.5,.9,1,amount=.65),M(4,7,.16),C(-26,3.5,14,60,1,amount=.4)])
    a("Drum Bus","DnB Jump Up Front Back","Favor kick/snare front edges without increasing cymbal sustain.","Cymbal wash rises or the snare becomes thin after matching.",[hp(30),bell(280,-.7)],[U(1.5,-1,.9),M(3,4,.25),C(-22,2,20,80,1)])
    a("Drum Bus","DnB Break Sampler Cohesion","Bring a chopped acoustic break together with restrained sampler colour.","The resampled top aliases excessively or original ghost timing changes.",[hp(28),lp(14000)],[M(1,4,.3,sampler=2),C(-23,2.2,22,86,1)])
    a("Drum Bus","DnB Last Beat Break Edit","Use explicit upper-band reverse and repeat steps at the phrase end.","Low kick timing moves or the first beat of the next phrase is missing.",[hp(25)],[C(-22,1.8,26,75)], [band(1000,16000,R(.32,glitch={13:7,15:1}))])
    a("Bass","DnB Modern Sub Level","Hold a sine-like sub steady with no stereo motion below the crossover.","The compressor modulates pitch or low stereo energy remains.",[hp(24),lp(5000)],[C(-25,2.5,30,130,3,0),W(0,1,1,180)])
    a("Bass","DnB Roller Mid Translation","Add restrained audible harmonics above an undriven low sub.","The added mids hide the snare or the deepest note distorts.",[hp(26)],[M(3,6,.35,phoneBass=.2,subProtectHz=95),C(-24,2.5,20,86,1,0)])
    a("Bass","DnB Liquid Warm Sustain","Level a soft bass phrase with smooth opto recovery and tape warmth.","The next note ducks noticeably or low sustain becomes woolly.",[hp(25),bell(300,-1)],[C(-25,2.2,35,170,3,0),M(1,3,.25,subProtectHz=75)])
    a("Bass","DnB Neuro Clean Sub Dirty Top","Separate aggressive growl treatment from a mono sub anchor.","Distortion leaks into the low fundamental or growl width cancels in mono.",[hp(25)],[W(0,1.08,1.15,170)], [band(180,8000,M(4,14,.4),C(-28,4,8,65,2,0))])
    a("Bass","DnB Jump Up Tempo Pocket","Dip the bass envelope on quarter-note starts without requiring a key bus.","The dip drifts with BPM or the offbeat never returns to full level.",[hp(28)],[motion(5.5,1,[0,.5,1,1]),M(2,5,.3,subProtectHz=85),W(0,1,1.05,180)])
    a("Vocal","DnB Modern Topline Seat","Control a sung lead with fast recovery and bounded sibilance.","The topline pumps at drum hits or sibilants become piercing.",[hp(100),bell(380,-1.5),bell(7000,0,1,-3)],[F(2,strength=.45,maxBoost=2.5),C(-24,2.8,9,90,2,3)])
    a("Vocal","DnB Liquid Hall Lift","Give a melodic lead a ducked hall that blooms in phrase gaps.","The hall hides fast words or low tail covers the bass.",[hp(110),bell(6500,0,1,-2.5)],[C(-24,2.3,16,150,3,3),V(3,1.5,35,.17,3,hp=450,lp=9000,duck=.5)])
    a("Vocal","DnB Roller MC Close","Keep a fast MC phrase dry and intelligible with limited leveling boost.","Quiet word endings disappear or breaths are raised into the groove.",[hp(105),bell(300,-1.5),bell(7200,0,1.2,-3.5)],[Z(-50,8,50,80,v2_attack=1),C(-25,3,5,65,2,3),M(2,2,.15)])
    a("Vocal","DnB Neuro Short Radio Call","Make a compact gritty band-limited call with a single sync reply.","The call becomes unintelligible or repeats obscure the next snare.",[hp(450),lp(4800)],[M(4,7,.22),E(5,.1,.12,3,hp=700,lp=4000)])
    a("Vocal","DnB Break Reversed Tail","Reverse a quiet end-of-phrase response without reversing dry words.","Dry consonants reverse or the response arrives before its capture is available.",[hp(140),bell(6800,0,1,-2.5)],[C(-23,2.2,12,100,3,3),E(8,.08,.13,0,taps=[(8,0,-12,.2,1)],scoreDuck=.5)])
    a("Keys","DnB Liquid Piano Bloom","Keep piano hammers clean while a filtered hall supports the chords.","Hall masks new chord attacks or low-mid sustain competes with bass.",[hp(140),bell(330,-1.3)],[C(-23,1.8,28,170,3,2),V(3,1.4,26,.12,3,hp=400,lp=8500)])
    a("Keys","DnB Roller Stab Cut","Make chord stabs short enough for a fast rolling arrangement.","Stab body disappears or release pumps on the next sixteenth.",[hp(200),bell(550,-1.5)],[U(1,-2,.85),M(3,4,.25),C(-23,2,14,70,1,2)])
    a("Keys","DnB Neuro Pluck Focus","Constrain a bright synth pluck before adding mid-band grit.","Hard harmonics dominate the pluck pitch or its attack disappears.",[hp(180),bell(5200,0,1.4,-2.5)],[M(4,8,.25,subProtectHz=180),U(.8,-1,.85)])
    a("Keys","DnB Liquid Rhodes Drifts","Add quiet alternating eighth echoes behind a warm Rhodes chord.","Echoes shift chord pitch or mask the following harmony.",[hp(150)],[M(2,3,.2),E(5,.16,.1,1,taps=[(5,0,-11,-.5,0),(8,0,-15,.5,0)],hp=600,lp=7000)])
    a("Samples","DnB Break Ghost Preservation","Glue a vintage break sample while preserving low-level ghost articulation.","The sampler erases soft notes or accents lose their natural hierarchy.",[hp(35),bell(450,-1)],[M(1,3,.25,sampler=2),C(-25,1.8,24,95,3,2,knee=12)])
    a("Samples","DnB Neuro Last Eighth Flip","Reverse only the final eighth of a melodic sample.","Reversal changes the next phrase start or low notes phase-cancel.",[hp(120)],[R(.5,time={14:2,15:2},timeStretch=2)])
    a("Samples","DnB Liquid Skyline Texture","Make a quiet atmospheric sample broad above a clean low cut.","Mono fold loses the main texture or reverb overwhelms transients.",[hp(250),hi(11000,.5)],[W(0,1.2,1.35,280,v2Generate=.08,v2Generator=2),V(3,2,32,.16,6,hp=600,lp=9000)])
    a("Samples","DnB Roller Rhythmic Fragments","Cut selected off-grid sample fragments into an explicit 16-step response.","The fragment pattern sounds randomly dense or loses downbeat identity.",[hp(180)],[R(.4,glitch={3:1,7:4,11:1,15:2},glitchDepth=.3),V(6,.24,0,.035,4,hp=800,lp=6000)])
    a("Master","DnB Modern Drum Headroom","Finish a fast full mix with gentle bus control and restrained final limiting.","Final 8x true peak exceeds -1 dBTP or snare contrast drops before 3 dB work.",[hp(24),bell(330,-.6)],[C(-18,1.4,35,86,1,0),W(0,1,1.05,120),L(-2.2,75)])
    a("Master","DnB Liquid Open Finish","Retain the open piano and vocal field with a stable low center.","Mono narrows the hook excessively or the hall pumps on each snare.",[hp(23),hi(11500,.4)],[C(-19,1.4,40,130,3,0),W(.9,1.02,1.1,110),L(-1.7,110)])
    a("Master","DnB Neuro Controlled Edge","Tame upper-mid density before a conservative peak ceiling.","The growl loses identity or limiter work exceeds 3 dB to hit target loudness.",[hp(25),bell(3600,0,1,-1.5)],[M(3,2,.12,subProtectHz=130),C(-19,1.5,32,86,1,0),L(-2.3,70)])
    a("Master","DnB Roller Low End Finish","Keep the sub mono and prevent low-mid buildup from dominating the limiter.","Bass note balance changes in mono or kicks become softer after matching.",[hp(24),bell(240,-.8,.85)],[W(0,1,1.04,140),C(-20,1.6,30,95,1,0),L(-2,85)])

    a=bank("Studio",120)
    a("Lead Vocal","Studio Transparent Lead","Stabilize phrase level gently before clean vocal compression.","Quiet breaths rise between words or loud notes lose expression.",[hp(75),bell(300,-1),bell(7000,0,1,-2.5)],[F(2,strength=.35,maxBoost=2,maxCut=4),C(-23,2,15,150,1,3)])
    a("Lead Vocal","Studio Intimate Soul","Smooth close-miked soul singing with soft harmonics and a tiny chamber.","Chest body becomes woolly or the vocal sounds farther from the mic.",[hp(65),bell(240,-1),hi(11000,.5)],[C(-24,2.3,22,230,3,3),M(2,3,.2),V(2,.55,20,.055,3,hp=350,lp=6500)])
    a("Lead Vocal","Studio Rock Front Edge","Blend FET density into a rock lead while keeping strong consonants.","Shouts flatten or the 3 kHz area becomes painful at matched level.",[hp(90),bell(450,-1.5),bell(7500,0,1,-3)],[C(-27,4,8,100,2,3,amount=.6),M(3,5,.2),V(4,.75,28,.08,2,hp=500,lp=6500)])
    a("Lead Vocal","Studio Spoken Warmth","Reduce room noise gently and level spoken phrases without a radio crush.","The gate removes word endings or breaths audibly swell.",[hp(70),bell(220,-1),bell(6200,0,1,-2)],[Z(-52,8,90,150,v2_attack=2),F(2,target=-21,strength=.4,maxBoost=2),C(-23,2,18,180,3,3)])
    a("Lead Vocal","Studio Dry Fast Verse","Make a fast dry rap verse readable with quick release and no added space.","Fast consonants smear or the final word ducks after an accent.",[hp(85),bell(350,-1.8),bell(7100,0,1.2,-3.5)],[C(-25,3.2,6,80,2,3),M(2,2,.12)])
    a("Lead Vocal","Studio Ballad Open Hall","Keep the lead ahead of a wide hall that opens in sustained gaps.","Consonants sink into the hall or the tail clouds the next line.",[hp(80),bell(6800,0,1,-2.5)],[C(-24,2,25,240,3,3),V(3,2.2,55,.18,6,hp=450,lp=9000,duck=.55)])
    a("Lead Vocal","Studio Whisper Support","Support quiet whisper syllables with a bounded rider and soft air control.","Mouth noise rises or the rider hunts during silence.",[hp(110),bell(8500,0,1.2,-3)],[F(2,target=-22,strength=.3,maxBoost=2,maxCut=3,floorDb=-48,riseMs=350,fallMs=220),C(-26,1.8,18,200,3,3)])
    a("Lead Vocal","Studio Small Room Truth","Put a dry vocal in a believable short room without washing out detail.","Room sounds like slapback or obscures word onsets.",[hp(85),bell(350,-1)],[C(-23,2,16,160,1,3),V(1,.38,8,.075,1,hp=350,lp=8000,earlyDb=1,lateDb=-4)])
    a("Lead Vocal","Studio Telephone Bridge","Make an intentional band-limited bridge voice with controlled drive.","The bridge words stop being intelligible or resonant mids hurt.",[hp(350),lp(3700),bell(1600,1),bell(2600,-1)],[C(-26,3.5,5,90,2,3),M(4,8,.2)])
    a("Lead Vocal","Studio Fifth Answer","Answer the end of a phrase with a quiet fifth-up sync reflection.","Pitch-shifted words compete with the dry line or create dissonant continuous feedback.",[hp(100),bell(7000,0,1,-2.5)],[C(-24,2.5,12,135,3,3),E(8,.12,.12,0,taps=[(8,7,-13,.25,0)],scoreDuck=.6)])
    a("Lead Vocal","Studio Short Double Width","Use quiet offset unpitched reflections as a restrained vocal double.","The lead moves sideways or mono loses consonant intelligibility.",[hp(95),bell(6700,0,1,-2.5)],[C(-24,2.3,14,140,1,3),E(0,0,.1,0,taps=[(0,0,-12,-.65,0),(0,0,-14,.65,0)],tap1Time=24,tap2Time=37,hp=400,lp=7000)])
    a("Lead Vocal","Studio Parallel Shout Bed","Add a rough low-level density bed while retaining the dry shout crest.","The matched dry shout feels smaller or saturated esses dominate.",[hp(100),bell(7200,0,1,-4)],[C(-30,7,5,85,2,3,amount=.25),M(4,10,.16),V(6,.3,14,.045,4,hp=700,lp=5500)])
    a("Backing Vocal","Studio Backing Soft Seat","Place a backing stack behind the lead with controlled upper attack.","The backing consonants become more forward than the lead.",[hp(160),hi(9000,-1.5)],[C(-26,3,15,180,3,3),U(-1,.5,.5),V(2,.85,12,.12,3,hp=450,lp=7000)])
    a("Backing Vocal","Studio Chorus Side Halo","Make the high backing field wider while preserving a centered vowel body.","The mono chorus loses the vowel or center shifts on consonants.",[hp(170),bell(500,-1)],[C(-25,2.5,20,200,3,3),W(0,1.1,1.35,250),V(4,1.1,22,.1,3,hp=600,lp=8500)])
    a("Backing Vocal","Studio Dark Gang Stack","Keep a gang shout compact, rough and slightly behind the lead.","The stack turns into flat distortion or low mids crowd the lead.",[hp(190),lp(9000),bell(800,-1.5)],[C(-28,4,10,130,2,3,amount=.65),M(3,6,.25),V(1,.45,5,.1,2,hp=600,lp=5500)])
    a("Backing Vocal","Studio Air Choir Bloom","Add a quiet octave shimmer only to the filtered reverb return.","A pitched dry lead appears or the shimmer builds across several chords.",[hp(230),bell(8000,0,1,-2)],[C(-25,2,22,230,3,3),V(3,2.4,40,.18,6,hp=700,lp=10000,shimmerAmount=.18,shimmerPitch=12,shimmerFeedback=.2,duck=.35)])
    a("Backing Vocal","Studio Reverse Choir Swell","Create a quiet reverse-shaped reverb rise behind held backing vowels.","The dry consonants reverse or the swell arrives after the intended chord change.",[hp(200)],[V(3,1.8,0,.16,6,hp=600,lp=8500,timeShape=2,shapeSync=10,shapeTimeMs=500,gateReleaseMs=40),W(0,1.1,1.25,250)])
    a("Backing Vocal","Studio Rhythmic Hook Gate","Shape a backing hook into explicit soft sixteenth-note openings.","Gate clicks appear or key lyrical accents are removed.",[hp(180),hi(9500,-1)],[rhythm([1,1,0,.5,1,0,.5,0,1,1,0,.5,1,0,.7,0],amount=.6),V(6,.4,8,.07,3,hp=700,lp=7000)])
    a("Guitar","Studio Acoustic Pick Balance","Reduce pick bite and stabilize acoustic strums without flattening them.","String accents lose movement or the guitar gains low-mid boom.",[hp(80),bell(220,-1.5),bell(3800,0,1.5,-2)],[C(-23,2,25,180,3,2),V(1,.45,12,.07,1,hp=350,lp=8500)])
    a("Guitar","Studio Fingerstyle Detail","Retain finger transients while supporting quiet ringing notes.","Fret noise rises between notes or the sustained body pumps.",[hp(70),bell(300,-1)],[F(2,target=-21,strength=.25,maxBoost=1.5,maxCut=3),C(-24,1.8,32,240,3,2)])
    a("Guitar","Studio Funk Short Chank","Keep muted electric chords tight with a little iron midrange.","Chord stops click or low-mid sustain covers the groove.",[hp(140),bell(450,-1.5)],[U(1,-2,.75),M(3,4,.25),C(-23,2.5,18,110,1,2)])
    a("Guitar","Studio Clean Amp Slap","Put a filtered short slap behind a clean electric lead.","The slap becomes a second distinct melody or combs out the mono lead.",[hp(100),bell(280,-1)],[M(2,3,.2),E(0,0,.12,2,hp=450,lp=5500,tap1Time=115,scoreFeedbackMs=115)])
    a("Guitar","Studio Grit Lead Sustain","Add a parallel rough edge and smooth sustain under a guitar lead.","The dry pick flattens or distortion hides bent-note pitch.",[hp(90),bell(3200,0,1,-2)],[C(-27,4,12,190,2,2,amount=.55),M(4,8,.2),V(4,.85,25,.08,2,hp=500,lp=6500)])
    a("Guitar","Studio Wide Rhythm Pair","Widen the high part of a recorded guitar pair without inventing low stereo.","Mono loses riff body or the pair balance drifts.",[hp(120),bell(350,-1.5)],[C(-22,1.8,28,160,1,2),W(0,1.1,1.25,220)])
    a("Guitar","Studio Ambient Guitar Trail","Let a filtered delay feed a quiet long hall after clean guitar notes.","Feedback muddies the next harmony or the dry pick is buried.",[hp(140)],[E(6,.28,.15,1,hp=500,lp=6500),V(3,2.8,38,.13,6,hp=650,lp=8000,duck=.4)])
    a("Guitar","Studio Garage Sample Edge","Give a dry guitar sample a compact sampled edge and a small room.","The sample loses chord identity or noise fills the stops.",[hp(140),lp(11000)],[M(1,5,.35,sampler=2),C(-25,3,18,135,2,2,amount=.5),V(1,.35,5,.06,2,hp=500,lp=5500)])
    a("Keys","Studio Grand Clear Body","Trim piano low-mid congestion while leaving natural hammer dynamics.","Piano sounds scooped or sustain becomes obviously compressed.",[hp(55),bell(280,-1.5),hi(10000,.5)],[C(-22,1.6,35,260,3,2),V(2,.9,20,.08,1,hp=300,lp=10000)])
    a("Keys","Studio Felt Close Piano","Soften felt-piano mechanical bite and bring the room closer.","The felt attack disappears or quiet key noise rises audibly.",[hp(75),bell(320,-1),hi(8000,-1)],[U(-1,.5,.4),M(2,3,.2),V(1,.4,5,.09,3,hp=350,lp=6000)])
    a("Keys","Studio Rhodes Velvet","Smooth electric-piano chords with restrained even harmonics.","The loud chords fuzz or tremolo gets an unintended pumping envelope.",[hp(85),bell(400,-1.3)],[M(2,5,.35,evenOdd=.25),C(-24,2,28,260,3,2)])
    a("Keys","Studio Organ Iron Seat","Give a steady organ soft iron density and a bounded upper field.","The organ becomes a static wall or mono loses the held chord.",[hp(100),bell(1800,-1)],[M(3,5,.3),W(0,1.08,1.2,200),V(2,.7,10,.08,2,hp=450,lp=7500)])
    a("Keys","Studio Wide Piano Early Room","Use early room reflections to widen piano above its low body.","The mono hammer cancels or low piano notes lean to one side.",[hp(60),bell(300,-.8)],[V(1,.5,8,.1,1,hp=350,lp=10000,earlyDb=2,lateDb=-6),W(.8,1.05,1.15,140)])
    a("Keys","Studio Tape Keys Drift","Add mild tape instability to a sustained key texture.","Chord pitch becomes seasick or dry gaps gain noise.",[hp(120),lp(12500)],[M(1,4,.45,wow=.05,tone=-.08),V(2,.75,16,.07,2,hp=450,lp=7000)])
    a("Keys","Studio Piano Reverse Reply","Add a quiet reversed echo after the piano chord attack.","The dry hammer reverses or the reply smears the next chord change.",[hp(130)],[E(8,.08,.11,0,taps=[(8,0,-13,-.2,1)],hp=500,lp=7500),V(6,.4,0,.04,3,hp=700,lp=7000)])
    a("Keys","Studio Chord Soft Pulse","Make a sustained chord breathe gently over a two-beat custom curve.","The chord hard-gates or pulse phase drifts with transport.",[hp(140),bell(420,-1)],[motion(3,2,[.3,.6,1,.8]),W(0,1.08,1.2,200)])
    a("Synth","Studio Lead Focus Cut","Tame upper-mid lead resonance and add a small filtered sync response.","The lead loses pitch focus or echoes occupy the next melody note.",[hp(120),bell(3400,0,1.6,-2.5)],[C(-23,2,12,120,1,2),E(6,.15,.09,0,hp=600,lp=6500)])
    a("Synth","Studio Pad Wide Calm","Make a pad wide above a mono low anchor with a slow spatial journey.","The mono fold loses chord identity or motion makes the bass move.",[hp(170)],[W(0,1.15,1.35,220,v2PositionOn=1,v2Motion=1,v2Unit=1,v2Nodes=2,v2Node0Angle=-25,v2Node1Angle=25,v2Node0TravelBeat=6,v2Node1TravelBeat=6),V(3,2,30,.1,3,hp=600,lp=8500)])
    a("Synth","Studio Pluck Falling Notes","Answer a pluck with sparse descending interval reflections.","The echoes replace the main melody or descend repeatedly in uncontrolled feedback.",[hp(180)],[E(5,.12,.14,0,taps=[(5,-7,-13,-.4,0),(8,-12,-17,.4,0)],hp=700,lp=8000)])
    a("Synth","Studio Synth Texture Stair","Use explicit SHAPE drive steps on the upper band of a synth phrase.","The sub distorts or drive transitions create clicks.",[hp(100)],[M(1,2,.15)], [band(600,11000,R(.3,shape=(6,[0,0,2,2,4,4,6,6,8,8,6,6,4,4,2,0])))])
    a("Synth","Studio Short Stereo Spark","Add small diffuse high-band stereo energy to a mono pluck.","The mono pluck loses its central attack or width sounds like a hard echo.",[hp(160)],[W(0,1.05,1.3,250,v2Generate=.18,v2Generator=2),V(6,.28,3,.05,4,hp=1000,lp=10000)])
    a("Synth","Studio End Phrase Half Time","Apply half-time only on the final two subdivisions of a synth phrase.","The next bar stays half-speed or pitch drops despite Clean stretch.",[hp(120)],[R(.5,time={14:1,15:1},timeStretch=2),E(5,.08,.06,0,hp=700,lp=7000)])
    a("Mix Bus","Studio Mix Gentle Glue","Bind a balanced premaster with minimal reduction and no forced loudness.","Matched kick/snare contrast falls or the vocal ducks on bass notes.",[hp(22),bell(300,-.4)],[C(-19,1.5,40,180,1,0),M(3,2,.12)])
    a("Mix Bus","Studio Mix Tape Cohesion","Add restrained tape density while holding the deepest bass clean.","Bass becomes woolly or the top loses separation after matching.",[hp(24)],[M(1,3,.22,subProtectHz=90),C(-20,1.4,35,210,3,0)])
    a("Mix Bus","Studio Mix Low Center","Center only low-frequency mix energy and gently open the top field.","The mono lead loses body or kick/sub notes change in the fold.",[hp(23)],[W(0,1,1.08,120),C(-18,1.3,45,200,1,0)])
    a("Mix Bus","Studio Mix Dynamic Mud","Let the low mids duck only when dense chords build up.","Sparse passages sound scooped or bass-note body is removed continuously.",[hp(22),bell(280,0,.9,-1.8)],[C(-19,1.4,38,170,1,0)])
    a("Mix Bus","Studio Mix Soft Top Control","Control a brittle upper mix band while leaving the rest open.","The lead loses presence or cymbals trigger broadband pumping.",[hp(23)],[C(-18,1.3,42,190,3,0)], [band(5000,16000,C(-25,1.8,3,75,1,0,knee=10))])
    a("Mix Bus","Studio Mix Parallel Body","Blend a little clean parallel density under the full dry premaster.","The dry crest shrinks or the kick-bass pocket clouds at matched loudness.",[hp(24),bell(380,-.5)],[C(-28,4,25,160,1,0,amount=.18),M(2,2,.1)])
    a("Utility","Studio Capture Match Minus Eighteen","Prepare a bounded RMS match capture for comparing source clips at -18 dBFS.","Match runs as an unbounded live rider or raises a silent clip.",[],[F(1,target=-18,measure=0,matchBoost=6,matchCut=18,matchedDb=0)])
    a("Utility","Studio Dual Mono Safe Fold","Fold a stereo source to mono for a deliberate center-only utility.","The fold clips or the result retains a left-right difference.",[],[F(0,routing=1,gain=-3)])
    a("Utility","Studio One Beat Breathing","Apply a clear quarter-note dip and recovery to a sustained input.","The envelope clicks at the grid edge or free-runs after transport reset.",[],[motion(9,1,[0,.2,.75,1],edgeMs=5)])
    a("Utility","Studio Filtered Space Return","Specify a pure wet-only filtered chamber return for an aux bus.","Any original dry impulse remains at the output or low-frequency tails build.",[],[V(2,1.25,18,1,2,hp=450,lp=7500,duck=.2)],output="wet_only")
    return deepcopy(out)
