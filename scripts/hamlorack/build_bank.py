"""Build/check authored HamloRack design candidates. No DSP or audio claims.

The declaration reader intentionally accepts literal numeric declarations and
explicit known parameter-family loops only. It is not a C++ interpreter. Every
parameter used by a recipe must resolve; unknown fields fail, never disappear.
"""
from __future__ import annotations

import argparse
import ast
import collections
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from recipes import recipes

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / "Assets/Presets/HamloRack/Design"
DOCS = ROOT / "docs/hamlorack"
PLUGINS = ["Comp", "Udarnik", "RedWall", "Mag", "Echo", "Verb", "Zastava", "Shirota", "Fader", "Revolution"]
SYNC = ["Free", "1/32", "1/16", "1/16 D", "1/8 T", "1/8", "1/8 D", "1/4 T", "1/4", "1/4 D", "1/2", "1/1"]


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def digest(value):
    return hashlib.sha256(canonical(value)).hexdigest()


def number(text):
    text = re.sub(r"(?<=[\d.])[fF]\b", "", text.strip())
    tree = ast.parse(text, mode="eval")
    def run(n):
        if isinstance(n, ast.Constant) and type(n.value) in (int, float):
            return n.value
        if isinstance(n, ast.UnaryOp) and isinstance(n.op, (ast.USub, ast.UAdd)):
            return (-1 if isinstance(n.op, ast.USub) else 1) * run(n.operand)
        if isinstance(n, ast.BinOp) and isinstance(n.op, (ast.Add, ast.Sub, ast.Mult, ast.Div)):
            a, b = run(n.left), run(n.right)
            return {ast.Add: lambda: a+b, ast.Sub: lambda: a-b, ast.Mult: lambda: a*b, ast.Div: lambda: a/b}[type(n.op)]()
        raise ValueError(text)
    return run(tree.body)


def split_args(text):
    out, start, level, quoted, escaped = [], 0, 0, False, False
    for i, c in enumerate(text):
        if quoted:
            if c == '"' and not escaped:
                quoted = False
            escaped = c == "\\" and not escaped
        elif c == '"':
            quoted = True
        elif c in "({[":
            level += 1
        elif c in ")}]":
            level -= 1
        elif c == "," and level == 0:
            out.append(text[start:i].strip()); start = i+1
    out.append(text[start:].strip())
    return out


def calls(text):
    for m in re.finditer(r"\b(knob|number|add|choice|sync|shape|throwKnob)\s*\(", text):
        pos, level, quoted, escaped = m.end(), 1, False, False
        for end in range(pos, len(text)):
            c = text[end]
            if quoted:
                if c == '"' and not escaped: quoted = False
                escaped = c == "\\" and not escaped
            elif c == '"': quoted = True
            elif c == "(": level += 1
            elif c == ")":
                level -= 1
                if level == 0:
                    yield m.group(1), split_args(text[pos:end]), m.start()
                    break


def read_schema(plugin):
    base = ROOT / "Plugins" / plugin
    sources = {
        "HamloComp": ["Plugin/Parameters.h"],
        "HamloUdarnik": ["Plugin/Parameters.h"],
        "HamloRedWall": ["Plugin/Parameters.h"],
        "HamloMag": ["Plugin/Parameters.h"],
        "HamloEcho": ["Standalone/ModuleVariant.cpp", "Standalone/ScoreParameters.cpp"],
        "HamloVerb": ["Standalone/ModuleVariant.cpp", "Standalone/VerbParameters.h"],
        "HamloZastava": ["Standalone/ModuleVariant.cpp", "Standalone/ModernParameters.cpp"],
        "HamloShirota": ["Standalone/ModuleVariant.cpp", "Standalone/V2Parameters.cpp"],
        "HamloFader": ["Standalone/ModuleVariant.cpp", "Standalone/FaderHostControls.cpp"],
        "HamloRevolution": ["Standalone/ModuleVariant.cpp"],
    }[plugin]
    schema, identities = {}, {}
    def put(key, lo, hi, initial, origin, labels=None, step=0):
        schema[key] = dict(minimum=lo, maximum=hi, default=initial, interval=step, origin=origin)
        if labels is not None: schema[key]["choices"] = labels

    for rel in sources:
        path = base / "Source" / rel
        raw = path.read_text(encoding="utf-8-sig")
        relative = path.relative_to(ROOT).as_posix()
        identities[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
        text = re.sub(r"//[^\n]*", "", raw)
        if plugin == "HamloRevolution":
            text = text[text.index("ModuleSpec makeRevolution()"):text.index("float transformed(")]
        if rel.endswith("Parameters.h") and rel.startswith("Plugin/"):
            for m in re.finditer(r'\{\s*"([^"]+)"\s*,\s*"([^"]*)"\s*,\s*([^{}]+),\s*\{([^{}]*)\}', text):
                args = split_args(m.group(3))
                try:
                    lo, hi, initial = map(number, args[:3])
                except (ValueError, SyntaxError): continue
                labels = re.findall(r'"([^"\n]*)"', m.group(4)) or None
                put(m.group(1), lo, hi, initial, relative, labels, 1 if labels else 0)
        for kind, args, pos in calls(text):
            if plugin == "HamloFader" and rel.endswith("FaderHostControls.cpp"):
                args = args[1:]
            if len(args) < 2 or not re.fullmatch(r'"[^"]+"', args[0]): continue
            key = json.loads(args[0])
            try:
                if kind == "choice":
                    if plugin == "HamloFader" and rel.endswith("FaderHostControls.cpp"):
                        labels = json.loads(args[2]).split("|")
                        initial = number(args[3]) if len(args)>3 else 0
                    else:
                        label_arg = next((a for a in args[2:] if a.startswith("{")), None)
                        if label_arg is None and args[-1] in ("throwTargets", "throwTimes", "notes"):
                            var = args[-1]
                            found = re.search(r"\b"+var+r"\s*\{([^}]*)\}", text, re.S)
                            label_arg = "{"+found.group(1)+"}" if found else None
                        if label_arg is None: continue
                        labels = re.findall(r'"([^"\n]*)"', label_arg)
                        initial = (number(args[3]) if len(args)>3 else 0) if args[2].startswith("{") else number(args[2])
                    put(key, 0, len(labels)-1, initial, relative, labels, 1)
                elif kind == "sync":
                    put(key, 0, len(SYNC)-1, 0, relative, SYNC, 1)
                elif kind == "shape":
                    put(key, *map(number, args[3:6]), relative)
                elif kind == "throwKnob":
                    put(key, 0, 1, number(args[3]), relative)
                else:
                    lo, hi, initial = map(number, args[2:5])
                    step = 0
                    if kind == "add" and len(args)>5: step = number(args[5])
                    if plugin == "HamloZastava" and "ModernParameters" in rel:
                        step = number(args[5]) if len(args)>5 else .01
                    if plugin == "HamloFader" and "FaderHostControls" in rel and len(args)>6:
                        step = number(args[6])
                    put(key, lo, hi, initial, relative, step=step)
            except (ValueError, SyntaxError, TypeError):
                continue  # Nonliteral declaration; used keys still fail closed below.

    # Parameter families authored as loops in the exact files hashed above.
    if plugin == "HamloEcho":
        origin = str((base/"Source/Standalone/ScoreParameters.cpp").relative_to(ROOT).as_posix())
        notes = [48,55,60,36,60,60,60,60]
        for t in range(1,9):
            prefix = f"tap{t}"
            for suffix, lo, hi, val in [("On",0,1,int(t<=4)),("Note",0,127,notes[t-1]),("Time",1,4000,250*t),("Gain",-48,0,-12),("Pan",-1,1,0),("Fragment",50,2000,125),("Repeats",2,8,4),("Low",10,2000,10),("High",500,20000,20000),("SpaceSend",0,1,1)]:
                put(prefix+suffix,lo,hi,val,origin,step=1 if suffix in ("On","Note","Repeats") else 0)
            put(prefix+"Sync",0,11,0,origin,SYNC,1)
            put(prefix+"Shape",0,4,0,origin,["Forward","Reverse","Stutter","Tape Stop","Skip"],1)
        for b in range(1,4):
            for suffix,lo,hi,val in [("Time",.25,4,1),("Feedback",0,.9,.25),("Gain",-24,6,0),("Pan",-1,1,0),("On",0,1,1)]:
                put(f"band{b}"+suffix,lo,hi,val,origin,step=1 if suffix=="On" else 0)
    if plugin == "HamloZastava":
        for n in range(1,33): put(f"v2_step{n}",0,1,1,"Plugins/HamloZastava/Source/Standalone/ModernParameters.cpp",step=.001)
    if plugin == "HamloFader":
        for n in range(32): put(f"level{n:02d}",0,1,1,"Plugins/HamloFader/Source/Standalone/FaderHostControls.cpp")
    if plugin == "HamloShirota":
        origin="Plugins/HamloShirota/Source/Standalone/V2Parameters.cpp"
        schema["v2Nodes"]["interval"]=1
        for n in range(16):
            for suffix,lo,hi,initial in [("Angle",-180,180,-90 if n==0 else 90),
                    ("Hold",0,60000,250),("Travel",0,60000,750),
                    ("HoldBeat",0,64,.5),("TravelBeat",0,64,1.5)]:
                put(f"v2Node{n}"+suffix,lo,hi,initial,origin)
            put(f"v2Node{n}Arc",0,2,0,origin,["Shortest","Clockwise","Counterclockwise"],1)
            put(f"v2Node{n}Curve",0,1,1,origin,["Linear","Smooth"],1)
    return schema, identities


def source_manifest():
    out = {}
    prior_file = DEST / "native-contract.json"
    pinned = json.loads(prior_file.read_text(encoding="utf-8"))["sources"] if prior_file.exists() else {}
    def git(*args, cwd=ROOT):
        return subprocess.check_output(["git",*args],cwd=cwd,text=True,encoding="utf-8").strip()
    for name in PLUGINS:
        plugin = "Hamlo"+name
        source = ROOT.parent / ("HamloRevolutionTransitions" if name=="Revolution" else plugin)
        imported = git("rev-parse",f"HEAD:Plugins/{plugin}")
        imported_dirty=git("status","--porcelain=v1","--",f"Plugins/{plugin}")
        if source.is_dir():
            source_tree = git("rev-parse","HEAD^{tree}",cwd=source)
            source_commit = git("rev-parse","HEAD",cwd=source)
            dirty = git("status","--porcelain=v1",cwd=source)
        else:
            record = pinned.get(plugin, {})
            source_tree = record.get("source_tree")
            source_commit = record.get("source_commit")
            dirty = False
            if not source_commit or source_tree != imported:
                raise ValueError(f"No verified source identity for imported payload: {plugin}")
        if dirty or imported_dirty or imported != source_tree:
            raise ValueError(f"Source identity mismatch or dirty source: {plugin}")
        cmake = (ROOT/"Plugins"/plugin/"CMakeLists.txt").read_text(encoding="utf-8-sig")
        version = re.search(r"project\([^\s]+\s+VERSION\s+([\d.]+)",cmake)[1]
        out[plugin] = dict(version=version,source_commit=source_commit,source_tree=source_tree,imported_tree=imported,source_checkout=source.name)
    return out


def validate(data, schemas):
    if len(data)!=250: raise ValueError(f"Expected 250 recipes, got {len(data)}")
    seen_ids, names, sonic, coverage = set(), set(), {}, collections.Counter()
    banks = collections.Counter()
    for r in data:
        for field in ("id","bank","category","name","source","intent","reject_if"):
            if not r.get(field): raise ValueError(f"Missing {field}: {r.get('id')}")
        if r["id"] in seen_ids or r["name"] in names: raise ValueError(f"Duplicate ID/name: {r['id']}")
        seen_ids.add(r["id"]); names.add(r["name"]); banks[r["bank"]]+=1
        if not 40<=r["bpm"]<=240: raise ValueError(f"Invalid BPM: {r['id']}")
        payload = {k:r[k] for k in ("eq","chain","regions","output_mode")}
        key = digest(payload)
        if key in sonic: raise ValueError(f"Duplicate processing: {sonic[key]} / {r['id']}")
        sonic[key]=r["id"]
        if len(r["eq"])>24 or len(r["chain"])>6 or len(r["regions"])>8: raise ValueError(f"Capacity: {r['id']}")
        for eq in r["eq"]:
            if eq["shape"] not in ("LowCut","HighCut","Bell","LowShelf","HighShelf"): raise ValueError("EQ shape")
            if not 20<=eq["hz"]<=20000 or not -18<=eq["gain_db"]<=18 or not .2<=eq["q"]<=8: raise ValueError(f"EQ range: {r['id']}")
            if eq.get("dynamic") and not -12<=eq["dynamic"]["range_db"]<0: raise ValueError("Dynamic range")
        for region in r["regions"]:
            if not 20<=region["low_hz"]<region["high_hz"]<=20000 or not 1<=len(region["chain"])<=6: raise ValueError("Region")
        modules = r["chain"] + [m for region in r["regions"] for m in region["chain"]]
        for module in modules:
            plugin = module["plugin"]; coverage[plugin]+=1
            if not 0<module["amount"]<=1: raise ValueError("Mix")
            for key,value in module["parameters"].items():
                spec = schemas[plugin].get(key)
                if spec is None: raise ValueError(f"Unknown native ID: {r['id']} / {plugin}.{key}")
                if type(value) not in (int,float) or not math.isfinite(value) or not spec["minimum"]-1e-6<=value<=spec["maximum"]+1e-6:
                    raise ValueError(f"Native range: {r['id']} / {plugin}.{key}={value} ({spec})")
                interval = spec["interval"]
                if interval and abs((value-spec["minimum"])/interval-round((value-spec["minimum"])/interval))>1e-4:
                    raise ValueError(f"Native grid: {r['id']} / {plugin}.{key}={value}")
            if plugin=="HamloRevolution":
                scene = module["scene"]
                if len(scene["lanes"])!=7 or any(len(lane)!=16 for lane in scene["lanes"]): raise ValueError("Scene dimensions")
                if len(scene["locks"]) != 3 or len(scene["allow_masks"]) != 3:
                    raise ValueError("Scene pattern masks require TIME, GLITCH and MOTION entries")
                if any(type(v)!=int or not 0<=v<=65535 for v in scene["locks"]):
                    raise ValueError("Scene lock mask range")
                if any(type(v)!=int or not 0<=v<=maximum for v,maximum in zip(scene["allow_masks"],(63,1023,65535))):
                    raise ValueError("Scene allow mask range")
                maxima=[5,9,15,15,15,15,15]
                for i,lane in enumerate(scene["lanes"]):
                    if any(type(v)!=int or not 0<=v<=maxima[i] for v in lane): raise ValueError("Scene action range")
                    on = module["parameters"][["timeOn","glitchOn","motionOn","shapeAOn","shapeBOn","throwAOn","throwBOn"][i]]
                    if bool(on)!=any(lane): raise ValueError(f"Empty or disabled active Revolution lane: {r['id']}/{i}")
            params=module["parameters"]
            required={
                "HamloComp":{"mix":1},
                "HamloUdarnik":{"engine":1,"protect":1,"mix":1},
                "HamloMag":{"mix":1,"match":0,"delta":0},
                "HamloRedWall":{"engine":1,"mix":1,"truePeak":1,"ceilingDb":-1},
                "HamloEcho":{"echoMode":1,"quickEnabled":0,"scoreRouting":1,"scoreMix":1},
                "HamloVerb":{"routing":1,"keepDry":0,"mix":1},
                "HamloFader":{"engine":1,"enabled":1,"mix":1},
                "HamloZastava":{"v2_enabled":1,"v2_external":0,"v2_mix":1},
                "HamloShirota":{"v2Mix":1,"v2Guard":1},
                "HamloRevolution":{"mix":1,"free":0,"scene":0,"glitchEngine":1},
            }[plugin]
            if any(params.get(k)!=v for k,v in required.items()): raise ValueError(f"Native routing/engine contract: {r['id']} / {plugin}")
            selectors={"HamloComp":("model",1),"HamloVerb":("engine",1),"HamloZastava":("v2_engine",1),"HamloShirota":("v2Mode",1)}
            if plugin in selectors:
                selector,minimum=selectors[plugin]
                if params.get(selector,0)<minimum: raise ValueError(f"Beta engine opt-in missing: {r['id']} / {plugin}")
            if plugin=="HamloMag" and any(params.get(k)!=0 for k in ("hiss","hum","crackle")):
                raise ValueError(f"Unplanned noise: {r['id']}")
            if plugin=="HamloFader" and params.get("mode")==1 and not r["calibration"].get("required_action"):
                raise ValueError(f"Fader Match capture action missing: {r['id']}")
            for diagnostic in ("delta","listen","monitor","v2_listen","v2Mono","freeze","scoreFreeze","mute","solo"):
                if params.get(diagnostic,0)!=0: raise ValueError(f"Diagnostic/freeze enabled: {r['id']}")
        if r["output_mode"] not in ("insert","wet_only"): raise ValueError("Output mode")
        if r["output_mode"]=="wet_only" and (r["regions"] or len(r["chain"])!=1 or r["chain"][0]["plugin"] not in ("HamloVerb","HamloEcho") or r["chain"][0]["amount"]!=1):
            raise ValueError("Wet-only return contract")
        if r["category"]=="Master":
            if not r["chain"] or r["chain"][-1]["plugin"]!="HamloRedWall": raise ValueError("Final master ceiling stage missing")
    if banks!={"Boom Bap":50,"Trap":50,"Techno":50,"Drum and Bass":50,"Studio":50}: raise ValueError(banks)
    allocation={"Kick":6,"Snare":6,"Hats":5,"Percussion":4,"Drum Bus":7,"Bass":5,"Vocal":5,"Keys":4,"Samples":4,"Master":4}
    for bank in banks:
        if bank!="Studio" and collections.Counter(r["category"] for r in data if r["bank"]==bank)!=allocation: raise ValueError(f"Allocation: {bank}")
    if set(coverage)!={"Hamlo"+n for n in PLUGINS}: raise ValueError("Module coverage")
    return dict(recipes=len(data),unique_processing=len(sonic),banks=dict(banks),module_instances=dict(sorted(coverage.items())),audio_status="not_run; runtime adapters required")


def render_catalog(data, schemas):
    lines=["# HamloRack: 250 authored preset designs", "", "Status: design candidates. Native parameter declarations checked; audio calibration and runtime integration pending.", "", "Every chain below is ordered. EQ precedes frequency regions; the full-range chain follows. Amount is the rack insert blend or wet-send level; native MIX stays unity. Full exact settings and explicit Revolution scenes are in `Assets/Presets/HamloRack/Design/recipes.json`.", ""]
    for bank in dict.fromkeys(r["bank"] for r in data):
        lines += [f"## {bank}", ""]
        for r in (r for r in data if r["bank"]==bank):
            lines += [f"### {r['id']} - {r['name']}", "", f"**{r['category']} / {r['bpm']} BPM.** Source: {r['source']}", "", r["intent"], ""]
            eq=[]
            for e in r["eq"]:
                text=f"{e['shape']} {e['hz']:g} Hz / {e['gain_db']:+g} dB / Q {e['q']:g}"
                if "dynamic" in e: text+=f" (dynamic {e['dynamic']['range_db']:g} dB)"
                eq.append(text)
            lines += ["EQ: "+("; ".join(eq) or "flat"), ""]
            for title, chain in [("Full range",r["chain"])]+[(f"Band {z['low_hz']:g}-{z['high_hz']:g} Hz",z["chain"]) for z in r["regions"]]:
                if not chain: continue
                lines += [title+":", ""]
                for m in chain:
                    def value_text(k,v):
                        labels=schemas[m["plugin"]][k].get("choices")
                        return f"{k}={labels[int(v)]} ({v:g})" if labels else f"{k}={v:g}"
                    params=", ".join(value_text(k,v) for k,v in m["parameters"].items())
                    lines += [f"- {m['plugin']} (amount {m['amount']:g}): {params}"]
                    if "scene" in m:
                        actions=[["Pass","Half","Reverse","Freeze","Tape Stop","Double"],
                            ["Pass","Repeat","Roll","Rise","Chop","Drop","Shuffle","Reverse","Stop","Gap"]]
                        for i,lane in enumerate(m["scene"]["lanes"]):
                            events=[f"{step+1}: {actions[i][v] if i<2 else v}" for step,v in enumerate(lane) if v]
                            if events: lines += ["  - "+m["scene"]["lane_names"][i]+" (steps 1-16): "+"; ".join(events)]
                lines += [""]
            lines += [f"Reject/revise if: {r['reject_if']}", "", f"Calibration: {r['calibration']['intent_check']}; status **{r['calibration']['status']}**.", ""]
            if "required_action" in r["calibration"]: lines += ["Required native action: "+r["calibration"]["required_action"],""]
    return "\n".join(lines).rstrip()+"\n"


def render_overview(data):
    lines=["# HamloRack preset design overview", "", "250 authored candidates. Design validation passed; native runtime, audio calibration and listening are pending. Full parameter settings and rejection checks: [catalog](PRESET_CATALOG.md). Integration and acceptance: [RACK0](RACK0_DESIGN.md).", ""]
    for bank in dict.fromkeys(r["bank"] for r in data):
        lines += [f"## {bank}", "", "| ID | Category | Name | Intended result | Ordered processing |", "|---|---|---|---|---|"]
        for r in (r for r in data if r["bank"]==bank):
            chain=" > ".join(m["plugin"].removeprefix("Hamlo") for m in r["chain"]) or "flat full range"
            for z in r["regions"]: chain+=f"; {z['low_hz']:g}-{z['high_hz']:g} Hz: "+" > ".join(m["plugin"].removeprefix("Hamlo") for m in z["chain"])
            lines += [f"| {r['id']} | {r['category']} | {r['name']} | {r['intent']} | {chain} |"]
        lines += [""]
    return "\n".join(lines).rstrip()+"\n"


def main():
    parser=argparse.ArgumentParser();parser.add_argument("--check",action="store_true");args=parser.parse_args()
    schemas, files = {}, {}
    for name in PLUGINS:
        schema, hashes=read_schema("Hamlo"+name);schemas["Hamlo"+name]=schema;files.update(hashes)
    manifest=source_manifest()
    data=recipes()
    summary=validate(data,schemas)
    used={p:{k:v for k,v in s.items() if any(m["plugin"]==p and k in m["parameters"] for r in data for m in r["chain"]+[n for z in r["regions"] for n in z["chain"]])} for p,s in schemas.items()}
    native=dict(schema_scope="used literal native IDs plus explicitly expanded source families; not the complete host ABI",sources=manifest,parameter_source_sha256=files,parameters=used)
    bank=dict(format="HamloRackDesign",schema_version=1,status="design_only; no runtime importer",
        project_defaults=dict(used_layers=1,unused_layers="empty and disabled",lenin=False,stalin=False,
            output_gain_db=0,output_gain_status="calibration pending"),native_contract_sha256=digest(native),presets=data)
    receipt=dict(**summary,bank_sha256=digest(bank),native_contract_sha256=digest(native),structural_gates=["S1 source identity", "S2 count/native IDs/ranges/grid", "S3 exact topology uniqueness", "S4 capacities/scene routing"],audio_gates="A1-A9 pending; see RACK0_DESIGN.md")
    outputs={DEST/"recipes.json":json.dumps(bank,indent=2,ensure_ascii=False)+"\n",DEST/"native-contract.json":json.dumps(native,indent=2,ensure_ascii=False,sort_keys=True)+"\n",DOCS/"PRESET_CATALOG.md":render_catalog(data,schemas),DOCS/"PRESET_OVERVIEW.md":render_overview(data),DOCS/"STRUCTURAL_VALIDATION.json":json.dumps(receipt,indent=2,ensure_ascii=False)+"\n"}
    for path,text in outputs.items():
        if args.check:
            if not path.is_file() or path.read_bytes()!=text.encode("utf-8"): raise ValueError(f"Generated artifact stale: {path.relative_to(ROOT)}")
        else:
            path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(text.encode("utf-8"))
    print(json.dumps(receipt,indent=2))


if __name__=="__main__":
    main()
