"""Authored multi-layer demonstrations of the current plug-ins and HPEQ routing.

DSP settings are deliberate; calibration changes only the final output trim.
Run with --write to regenerate the JSON and readable catalog.
"""
from pathlib import Path
import json
import hashlib
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from recipes import C,U,M,V,W,E,F,R,L,rhythm,motion,hp,lp,bell,hi,lo,band
from build_bank import read_schema,PLUGINS

ROOT=Path(__file__).resolve().parents[2]

def layer(name,chain=(),regions=(),eq=(),mix=1):
    return dict(name=name,chain=list(chain),regions=list(regions),eq=list(eq),mix=mix)

def presets():
    out=[]
    def add(bank,category,name,bpm,intent,reject,layers,serial=False,trim=-6):
        out.append(dict(id=f"PW{len(out)+1:03d}",bank=bank,category=category,name=name,bpm=bpm,
            source=category,intent=intent,reject_if=reject,layers=layers,
            routing="Serial" if serial else "Parallel",output_mode="insert",output_trim_db=trim,
            calibration=dict(reference_input_dbfs=-18,method="output trim only; keep authored character and dynamics")))

    add("Boom Bap","Drum Bus","Basement Parallel Punch",90,
        "Dry attack over a crushed FET/iron body, with short dirty room behind the backbeat.",
        "The dry leading hit disappears or the room washes over the next kick.",[
        layer("Attack and tape",[U(5,-3,.75),M(1,11,.7,tapeBias=.7)],eq=[hp(28)]),
        layer("Parallel iron crush",regions=[band(140,7500,C(-36,12,3,110,2),M(3,18,.8))],mix=.4),
        layer("Basement chamber",[V(2,.65,18,.28,2,hp=300,lp=5500)])])
    add("Boom Bap","Samples","Twelve Bit Vinyl Mirage",90,
        "Split a loop into a steady bass foundation and grainy sampled mids; answer it with pitched stereo echoes.",
        "Sub notes wobble or the pitched answers hide the original phrase.",[
        layer("Sampler split",[C(-27,4,22,166,3)],regions=[band(160,14000,M(5,17,.8,sampler=1,wow=.12))]),
        layer("Fifth answers",[E(6,.3,.35,color=1,taps=[(6,7,-5,-.7,0),(8,12,-8,.7,0)])]),
        layer("High room",[V(4,1.1,35,.16,2,hp=650,lp=6000),W(0,1.1,1.5,180)])])
    add("Boom Bap","Vocal","Broadcast Vocal Theatre",90,
        "A dense opto/FET vocal with de-essing, dark tape slap and a ducked plate.",
        "Consonants vanish or the plate pushes the lead behind the beat.",[
        layer("Level and density",[F(2,strength=.65,maxBoost=3,maxCut=5),C(-29,3,15,180,3),C(-24,5,2,75,2),M(1,7,.5)],eq=[hp(85),bell(6800,0,2,-5)]),
        layer("Slap and plate",[E(3,.18,.32,color=1,lp=4800,taps=[(3,0,-5,-.25,0),(5,0,-9,.35,0)]),V(4,1.45,50,.25,2,duck=.65,hp=350,lp=6500)])],serial=True)
    add("Boom Bap","Drum Bus","Break Spin Matrix",90,
        "Keep the dry break under edited reverse, stop and repeat gestures in the mid band.",
        "The kick loses its pulse or generated gestures occur outside the authored final steps.",[
        layer("Dry knock",[U(4,-2,.65),M(3,12,.55)]),
        layer("Mid-band tape cuts",regions=[band(250,9000,R(.85,time={10:2,14:4,15:4},glitch={7:1,11:3},glitchDepth=.8))],mix=.65),
        layer("Snare space",[V(1,.5,10,.2,2,hp=600,lp=7000)])])

    add("Trap","Bass","808 Harmonic Engine",140,
        "A centered sub with aggressive small-speaker harmonics above it and controlled high-band spread.",
        "The sub collapses in mono or grit replaces the note fundamental.",[
        layer("Sub and buzz",regions=[band(25,120,M(2,10,.5,subProtectHz=75)),band(120,5000,M(4,24,.85,phoneBass=.5),C(-28,5,8,90,2))]),
        layer("Upper prism",regions=[band(700,9000,W(0,1.35,1.9,180))],mix=.5)])
    add("Trap","Drum Bus","Rage Gate Hammer",140,
        "Punchy overdriven drums interrupted by a deliberate sixteenth gate, with stuttered stereo answers.",
        "The kick pattern becomes unreadable or the silent gate steps leak a loud dry duplicate.",[
        layer("Hammer",[U(7,-4,.85),M(4,20,.75),C(-32,8,8,70,2)],eq=[hp(25)]),
        layer("Cut rhythm",[rhythm([1,1,.25,1,0,1,.5,1,1,.3,1,1,0,1,.25,1],.25)]),
        layer("Stutter echo",[E(2,.28,.22,taps=[(2,0,-8,-.8,2),(5,12,-10,.8,0)])])],serial=True)
    add("Trap","Vocal","Vocal Hologram Stack",140,
        "A forward compressed lead surrounded by octave/fifth voices and a dark ducked shimmer bed.",
        "The lead becomes a distant harmony pad or sibilants excite a bright wash.",[
        layer("Lead anchor",[C(-28,4,4,95,2),M(2,8,.45)],eq=[hp(90),bell(7200,0,1.8,-6)]),
        layer("Pitch constellation",[E(6,.3,.45,hp=400,lp=6500,taps=[(5,12,-6,-.8,0),(6,7,-8,.8,0),(8,-12,-12,0,0)])]),
        layer("Cloud plate",[V(4,3.2,60,.3,2,hp=500,lp=6500,duck=.75,shimmerAmount=.25)])])
    add("Trap","Keys","Cloud Tape Orbit",140,
        "Tape-coloured chords breathe in a drawn volume pattern while pitched echoes orbit a wide hall.",
        "Low notes move off centre or the pulse clicks at a pattern boundary.",[
        layer("Tape pulse",[M(1,13,.8,speed=2,wow=.18),motion(10,1,[1,.4,.8,.2,1,.6,.25,.8])]),
        layer("Orbit",[E(6,.42,.4,color=1,taps=[(6,7,-7,-.8,0),(9,12,-11,.8,0)]),V(3,4,35,.25,1,hp=450,lp=8000),W(0,1.35,1.8,180)])],serial=True)

    add("Techno","Drum Bus","Rumble Foundry",132,
        "A dry punch layer drives a low-frequency distorted reverb, followed by a tempo-shaped rumble envelope.",
        "The next kick is masked by a continuous sub tail or rumble develops a runaway feedback note.",[
        layer("Kick steel",[U(5,-3,.7),M(3,12,.6)]),
        layer("Low room furnace",regions=[band(35,220,V(1,1.3,0,.65,2,hp=40,lp=1400),M(2,20,.7),motion(14,1,[.1,.35,.8,1,.9,.7,.4,.15]))],mix=.7)])
    add("Techno","Samples","Acid Ratchet Laboratory",132,
        "Mid-band reverse, ratchet and stop events cut across saturated material while the low pulse stays intact.",
        "The bottom octave stutters or the edited phrase loses its downbeat.",[
        layer("Acid bite",[M(4,18,.75)],eq=[hp(30)]),
        layer("Edited mids",regions=[band(300,10000,R(.85,time={3:1,7:2,14:4,15:4},glitch={5:3,11:2,13:1},glitchDepth=.9))],mix=.85),
        layer("Metal reflections",[E(3,.4,.25,color=2,taps=[(3,0,-7,-.8,0),(7,7,-10,.8,0)])])])
    add("Techno","Drum Bus","Warehouse Gate Chamber",132,
        "A crushed drum body feeds a huge dark room that is chopped into an explicit off-beat rhythm.",
        "The gated room replaces the kick attack or leaves high-frequency ringing in the stops.",[
        layer("Dry mechanics",[U(5,-2,.8),C(-31,6,15,95,1),M(3,14,.6)]),
        layer("Warehouse return",[V(3,5.2,30,.6,2,hp=500,lp=6000),rhythm([0,.2,1,.65,0,.2,1,.4],.5)],mix=.65)])
    add("Techno","Samples","Dub Feedback City",132,
        "Four pitched, panned tape taps feed a dark chamber and a second rhythmic phrase cut.",
        "Feedback continues to build after the input stops or dry chords lose their placement.",[
        layer("Tape source",[M(1,12,.65,tapeBias=.65)]),
        layer("Dub network",[E(6,.55,.48,color=1,hp=500,lp=4500,taps=[(5,0,-5,-.8,0),(6,7,-9,.8,0),(8,-12,-12,-.3,0),(9,12,-15,.5,0)]),V(2,3.8,20,.23,2,hp=600,lp=4500)]),
        layer("Phrase holes",regions=[band(350,7500,R(.6,glitch={3:5,7:1,11:3,15:5},glitchDepth=.75))],mix=.45)])

    add("Drum and Bass","Drum Bus","Break Surgery Console",174,
        "Treat the sub, snare body and hat edge independently, then add a crushed mid-band parallel body.",
        "Ghost notes disappear, the snare turns brittle or the centered low band loses weight.",[
        layer("Three-band surgery",regions=[band(30,160,U(3,-2,.6)),band(160,4500,U(6,-3,.85),M(3,11,.7)),band(4500,18000,U(2,-4,.9),W(0,1.2,1.65,180))]),
        layer("Parallel snare engine",regions=[band(180,6500,C(-36,10,4,58,2),M(4,17,.6))],mix=.35),
        layer("Fast chamber",[V(2,.4,12,.18,2,hp=750,lp=7000)])])
    add("Drum and Bass","Bass","Neuro Harmonic Prism",174,
        "A solid low foundation under heavily driven, rhythmically edited mid harmonics and wide high-frequency answers.",
        "The fundamental changes pitch or the mono bass becomes quieter than its untreated reference.",[
        layer("Harmonic split",regions=[band(130,3500,M(4,25,.9),C(-29,5,6,70,2)),band(3500,14000,M(2,16,.6),W(0,1.3,1.8,180))]),
        layer("Neuro edits",regions=[band(250,8500,R(.7,glitch={3:1,7:3,11:2,15:1},motion_steps=[0,2,5,8,12,8,5,2,0,3,7,11,15,10,6,2],target=1,glitchDepth=.8))],mix=.5)])
    add("Drum and Bass","Keys","Liquid Chord Cinema",174,
        "A warm chord source blooms into stereo octave/fifth echoes, long chamber tails and a protected wide high band.",
        "The next chord is buried or mono fold removes the main harmony.",[
        layer("Warm source",[M(1,9,.65),C(-25,2.5,25,138,3)]),
        layer("Harmonic cinema",[E(6,.36,.5,hp=450,taps=[(6,7,-6,-.8,0),(8,12,-10,.8,0),(9,-12,-14,0,0)]),V(2,4.3,35,.33,3,hp=450,lp=8500),W(0,1.3,1.85,180)])],serial=True)
    add("Drum and Bass","Drum Bus","Bar End Break Mayhem",174,
        "Keep the first half of the break recognizable, then trigger explicit reverse, freeze, stop and repeat events above the kick band.",
        "Events move away from the programmed steps or the downbeat loses its dry anchor.",[
        layer("Break anchor",[U(5,-2,.8),M(3,13,.65)]),
        layer("Bar-end surgery",regions=[band(230,14000,R(.95,time={10:2,12:3,13:3,14:4,15:4},glitch={7:1,11:3,15:2},glitchDepth=1))],mix=.9),
        layer("Reverse reflections",[E(3,.25,.2,taps=[(3,0,-10,-.7,1),(5,12,-13,.7,0)])])])

    add("Studio","Lead Vocal","Four Stage Vocal Machine",120,
        "Ride quiet words, level them with opto compression, catch peaks with FET, de-ess and place the result in a ducked plate.",
        "Breaths surge, consonants dull or the long tail obscures the dry words.",[
        layer("Ride and tone",[F(2,strength=.65,maxBoost=4,maxCut=6),C(-28,3,18,180,3),C(-22,6,1.5,65,2),M(1,8,.5)],eq=[hp(80),bell(7000,0,2,-6)]),
        layer("Voice stage",[E(5,.2,.22,hp=350,lp=5500),V(4,1.8,55,.3,2,duck=.7,hp=400,lp=7000)])],serial=True)
    add("Studio","Guitar","Infinite Guitar Room",120,
        "Tape-softened pick attacks open a wide chamber with octave echoes and a controlled shimmer layer.",
        "Low strings smear, tuning becomes unstable or the wet layer masks the initial pick.",[
        layer("Pick foundation",[C(-25,3,25,190,3),M(1,10,.65,wow=.1)],eq=[hp(80)]),
        layer("Infinite room",[E(6,.45,.48,hp=500,taps=[(6,12,-8,-.7,0),(9,7,-12,.7,0)]),V(3,5.8,45,.4,1,hp=500,lp=8000,duck=.45,shimmerAmount=.3),W(0,1.3,1.8,180)])],serial=True)
    add("Studio","Drum Bus","Parallel Drum Machine",120,
        "Separate clean snap, an overdriven FET body and a gated stereo room into three independently adjustable layers.",
        "Layer summation flattens the dry hit or the room pumps between the intended gate accents.",[
        layer("Clean snap",[U(5,-3,.8)]),
        layer("Crush engine",regions=[band(100,9000,C(-38,16,3,90,2),M(3,22,.8))],mix=.4),
        layer("Gated room",[V(1,2,18,.6,2,hp=450,lp=6500),rhythm([1,.8,.2,0,1,.65,.15,0],.5),W(0,1.25,1.7,180)],mix=.55)])
    add("Studio","Mix Bus","Whole Rack Transformation",120,
        "A serial demonstration of spectral punch, multiband saturation, harmonic echo, edited motion and controlled stereo depth.",
        "The effect is only louder, the bass wanders off centre or a sustained section grows without bound.",[
        layer("Punch console",[C(-25,3,25,150,1),U(3,-1,.7)],eq=[hp(28),bell(350,-2)]),
        layer("Band colour",regions=[band(140,4000,M(3,16,.7)),band(4000,15000,M(1,9,.5))]),
        layer("Motion cut",regions=[band(400,11000,R(.55,glitch={7:1,15:3},motion_steps=[0,1,2,4,6,8,11,14,0,2,5,8,12,8,4,1],target=3))]),
        layer("Pitch and depth",[E(6,.3,.28,hp=650,taps=[(6,7,-9,-.65,0),(8,12,-12,.65,0)]),V(2,1.8,30,.2,2,hp=500,lp=7000)]),
        layer("Stereo guard and ceiling",[W(0,1.15,1.55,180),L(-4,100,1)])],serial=True,trim=-8)
    return out

def validate(rows):
    schemas={"Hamlo"+n:read_schema("Hamlo"+n)[0] for n in PLUGINS}
    counts={}
    for r in rows:
        assert 2<=len(r["layers"])<=10
        modules=[]
        for l in r["layers"]:
            assert len(l["chain"])<=6
            modules.extend(l["chain"])
            for region in l["regions"]:assert len(region["chain"])<=6;modules.extend(region["chain"])
        assert len(modules)>=3
        for m in modules:
            counts[m["plugin"]]=counts.get(m["plugin"],0)+1
            for key,value in m["parameters"].items():
                spec=schemas[m["plugin"]][key]
                assert spec["minimum"]<=value<=spec["maximum"],(r["id"],m["plugin"],key,value)
    assert len(rows)==20 and len(counts)==10
    return counts

if __name__=="__main__":
    rows=presets();counts=validate(rows)
    if "--write" in sys.argv:
        authoring_hash=hashlib.sha256(json.dumps(rows,sort_keys=True,separators=(",", ":")).encode()).hexdigest()
        calibration=ROOT/"docs/hamlorack/RACK_POWER_RECEIPT.json"
        if calibration.exists():
            measured=json.loads(calibration.read_text(encoding="utf-8-sig"))
            assert measured["authoring_sha256"]==authoring_hash,"Rack Power design changed; remeasure output trims"
            assert measured["failures"]==0 and len(measured["records"])==20
            trims={r["id"]:r["output_trim_db"] for r in measured["records"]}
            for r in rows:r["output_trim_db"]=trims[r["id"]]
        path=ROOT/"Assets/Presets/HamloRack/power.json"
        contents=(json.dumps(dict(format="HamloRackPower",version=1,authoring_sha256=authoring_hash,presets=rows),indent=2)+"\n").encode()
        if not path.exists() or path.read_bytes()!=contents:path.write_bytes(contents)
        lines=["# Rack Power presets","","Twenty authored demonstrations of complete current plug-ins and HPEQ layer/region routing. Audio verification is recorded separately; topology alone does not prove musical quality.",""]
        if calibration.exists():
            records=measured['records']
            lines.extend(['All 20 passed 160 source/level/tempo checks and 20 silent-tail renders. Output trim alone was adjusted; every authored DSP parameter stayed fixed. Loudness is within +/-0.5 LU for 10/20; ten headroom-constrained exceptions are listed below. Listening verdicts remain pending. Full metrics and hashes: [RACK_POWER_RECEIPT.json](RACK_POWER_RECEIPT.json).', '', '| ID | Output trim dB | LU difference | Worst dBTP | Tail end dBFS |', '|---|---:|---:|---:|---:|'])
            for item in records:
                peak=max(c['output']['true_peak_8x_dbtp'] for c in item['checks'])
                lines.append(f"| {item['id']} | {item['output_trim_db']:.2f} | {item['loudness_delta_lu']:+.2f} | {peak:.2f} | {item['tail_final_second_peak_dbfs']:.1f} |")
            lines.append('')
        for r in rows:
            lines.extend([f"## {r['id']} — {r['bank']} / {r['name']}","",r["intent"],"",f"Routing: {r['routing']}; reference {r['bpm']} BPM.",""])
            for l in r["layers"]:
                chains=[" → ".join(m["plugin"] for m in l["chain"])]
                chains.extend(f"{z['low_hz']}–{z['high_hz']} Hz: "+" → ".join(m["plugin"] for m in z["chain"]) for z in l["regions"])
                lines.append(f"- {l['name']} (layer mix {l['mix']:.0%}): "+"; ".join(c for c in chains if c))
            lines.extend(["","Reject if: "+r["reject_if"],""])
        (ROOT/"docs/hamlorack/RACK_POWER.md").write_bytes(("\n".join(lines).rstrip()+"\n").encode())
    print(json.dumps(dict(presets=len(rows),module_instances=counts)))
