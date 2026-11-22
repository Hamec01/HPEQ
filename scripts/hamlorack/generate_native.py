"""Build-directory-only namespace isolation of pinned complete native wrappers.

DSP tokens change only for hpeq/hamlo namespaces. Strings/comments remain exact.
The additional accessor exposes existing APVTS atoms; editor construction is
replaced in the internal copy. Input payloads are never modified.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

ROOT=Path(__file__).resolve().parents[2]
SIMPLE={"Comp":("comp","HamloCompProcessor"),"Udarnik":("udarnik","HamloUdarnikProcessor"),
    "RedWall":("redwall","HamloRedWallProcessor"),"Mag":("saturator","HamloMagProcessor")}
KINDS={"Verb":"Verb","Echo":"Echo","Zastava":"Zastava","Shirota":"Shirota","Fader":"Fader","Revolution":"Revolution"}
TOKEN=re.compile(r'''("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|//[^\n]*|/\*[\s\S]*?\*/)|\b(hpeq|hamlo)\b''')


def isolated(text,name):
    prefix="hr_"+name.lower()+"_"
    return TOKEN.sub(lambda m:m[1] if m[1] is not None else prefix+m[2],text)


def balanced(text,start):
    level=1
    for i in range(start,len(text)):
        if text[i]=='(':level+=1
        elif text[i]==')':
            level-=1
            if level==0:return text[start:i]
    raise ValueError("Unclosed CMake source list")


def sources(name):
    text=(ROOT/f"Plugins/Hamlo{name}/CMakeLists.txt").read_text(encoding="utf-8-sig")
    pattern=rf"add_library\(Hamlo{name}_Shared\s+STATIC\s+"
    found=re.search(pattern,text)
    if found is None:raise ValueError("No native source list: "+name)
    block=balanced(text,found.end())
    return [p for p in re.findall(r"Source/[A-Za-z0-9_/]+\.cpp",block)
        if "/UI/" not in p and not p.endswith(("Editor.cpp","Entry.cpp"))]


def write(path,text):
    data=text.encode("utf-8")
    if not path.exists() or path.read_bytes()!=data:
        path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)


def main():
    p=argparse.ArgumentParser();p.add_argument("--out",required=True);args=p.parse_args()
    dest=Path(args.out).resolve();manifest=json.loads((ROOT/"Assets/Presets/HamloRack/Design/native-contract.json").read_text(encoding="utf-8"))
    cmake=[];receipt={}
    for plugin,identity in manifest["sources"].items():
        name=plugin.removeprefix("Hamlo");base=ROOT/"Plugins"/plugin
        tree=subprocess.check_output(["git","rev-parse",f"HEAD:Plugins/{plugin}"],cwd=ROOT,text=True).strip()
        dirty=subprocess.run(["git","diff","--quiet","HEAD","--",f"Plugins/{plugin}"],cwd=ROOT).returncode
        if dirty or tree!=identity["source_tree"]:raise ValueError("Native payload differs from pinned RACK0 tree: "+plugin)
        selected=sources(name)
        header=f"Plugin/{SIMPLE[name][1]}.h" if name in SIMPLE else "Standalone/ModulePluginProcessor.h"
        processor=f"Source/{header[:-2]}.cpp"
        if processor not in selected:raise ValueError("Processor absent: "+name)
        hashes={}
        for file in sorted((base/"Source").rglob("*")):
            if not file.is_file() or file.suffix not in (".h",".cpp",".inc"):continue
            rel=file.relative_to(base).as_posix();raw=file.read_text(encoding="utf-8-sig")
            hashes[rel]=hashlib.sha256(file.read_bytes()).hexdigest()
            transformed=isolated(raw,name)
            if rel=="Source/"+header:
                accessor="\n    juce::AudioProcessorValueTreeState& rackNativeState() noexcept { return parameters; }\n"
                transformed,count=re.subn(r"(\npublic:\s*\n)",lambda m:m[1]+accessor,transformed,count=1)
                if count!=1:raise ValueError("Native accessor insertion: "+name)
            if rel==processor:
                transformed=re.sub(r'^#include "[^"\n]*Editor.h"\s*$',"",transformed,flags=re.M)
                transformed,count=re.subn(r"(juce::AudioProcessorEditor\*\s+\w+::createEditor\(\)\s*)\{[^{}]*\}",
                    r"\1{ return new juce::GenericAudioProcessorEditor(*this); }",transformed)
                if count!=1:raise ValueError("Unexpected editor factory: "+name)
            write(dest/plugin/rel,transformed)
        ns="hr_"+name.lower()+"_"
        if name in SIMPLE:
            group,cls=SIMPLE[name];expression=f"std::make_unique<{ns}hamlo::{group}::{cls}>()"
        else:
            expression=f"std::make_unique<{ns}hpeq::standalone::ModulePluginProcessor>({ns}hpeq::standalone::ModuleKind::{KINDS[name]}"
            expression+=",true)" if name=="Revolution" else ")"
        factory=f'''#include "Rack/NativeApi.h"
#include "{ROOT.as_posix()}/Source/Core/FxProjectState.h"
#include "{header}"
namespace hpeq::native {{
Handle create{name}() {{
    auto p = {expression};
    Handle h; h.processor = std::move(p);
    h.bind(static_cast<decltype(p)::element_type*>(h.processor.get())->rackNativeState());
    return h;
}}
}}
'''
        write(dest/plugin/"Factory.cpp",factory)
        if name in KINDS:
            latency=f"return {ns}hpeq::standalone::maximumLatencySamples({ns}hpeq::standalone::ModuleKind::{KINDS[name]},rate);"
        elif name=="Comp":latency="(void)rate;return 0;"
        elif name=="Udarnik":latency=f"return {ns}hamlo::dsp::UdarnikCore::lookaheadSamples(2,rate);"
        elif name=="Mag":
            latency=f"using namespace {ns}hamlo::dsp; FxSlotParams p; p.active=true;p.values[slot::kOversampling]=3;p.values[slot::kSatModel]=1;p.values[slot::kSatWow]=1;p.values[slot::kSatSampler]=1;return MagV2Processor::latencySamples(p,rate);"
        else:
            latency=f"using namespace {ns}hamlo::dsp; RedWallColour colour;colour.prepare(rate,1);LimiterSettings p;p.values[slot::kLookaheadMs]=10;p.values[slot::kLimTruePeak]=1;return colour.latency()+LimiterProcessor::lookaheadSamples(p,rate);"
        factory+=f"\nnamespace hpeq::native {{ int latency{name}(double rate) {{ {latency} }} }}\n"
        if name=="Revolution":
            factory+=f'''
namespace hpeq::native {{
juce::MemoryBlock makeRevolutionScene(const juce::NamedValueSet& values,const Scene& scene,juce::String& error) {{
    error.clear();
    Values checked;
    if(!describe(FxType::MonstaFx,values,48000,checked,error))return {{}};
    constexpr int maxima[] {{5,9,15,15,15,15,15}};
    for(size_t lane=0;lane<scene.lanes.size();++lane)
        for(auto value:scene.lanes[lane])
            if(value>maxima[lane]) {{error="Invalid Revolution scene action";return {{}};}}
    auto h=createRevolution();
    auto& p=*static_cast<{ns}hpeq::standalone::ModulePluginProcessor*>(h.processor.get());
    for(const auto& e:values) {{
        auto* parameter=p.parameter(e.name.toString());
        if(parameter==nullptr) {{error="Unknown scene parameter: "+e.name.toString();return {{}};}}
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(static_cast<double>(e.value))));
    }}
    {ns}hpeq::standalone::ModulePluginProcessor::RevolutionState state;
    for(size_t i=0;i<7;++i) {{
        state.lanes[i].length=16;state.lanes[i].first=scene.lanes[i];
        if(i<3) {{state.pattern.steps[i]=scene.lanes[i];state.pattern.locks[i]=scene.locks[i];state.pattern.allow[i]=scene.allow[i];}}
    }}
    p.resetScenes(state);
    juce::MemoryBlock result;p.getStateInformation(result);return result;
}}
bool readRevolutionScene(const juce::MemoryBlock& bytes,Scene& scene,juce::String& error) {{
    error.clear();
    const auto xml=juce::AudioProcessor::getXmlFromBinary(bytes.getData(),static_cast<int>(bytes.getSize()));
    if(xml==nullptr||xml->getTagName()!="HamloRevolution") {{error="Invalid Revolution scene state";return false;}}
    auto h=createRevolution();
    auto& p=*static_cast<{ns}hpeq::standalone::ModulePluginProcessor*>(h.processor.get());
    p.setStateInformation(bytes.getData(),static_cast<int>(bytes.getSize()));
    const auto decoded=p.getRevolutionState();
    for(size_t lane=0;lane<scene.lanes.size();++lane) {{
        scene.lanes[lane]=lane<3?decoded.pattern.steps[lane]:decoded.lanes[lane].first;
        if(lane<3) {{scene.locks[lane]=decoded.pattern.locks[lane];scene.allow[lane]=decoded.pattern.allow[lane];}}
    }}
    return true;
}}
}}
'''
        write(dest/plugin/"Factory.cpp",factory)
        lines=[f'add_library(HpeqNative{name} STATIC',f'    "${{CMAKE_CURRENT_LIST_DIR}}/{plugin}/Factory.cpp"']
        lines += [f'    "${{CMAKE_CURRENT_LIST_DIR}}/{plugin}/{file}"' for file in selected]
        lines += [")",f'target_include_directories(HpeqNative{name} PRIVATE "${{CMAKE_CURRENT_LIST_DIR}}/{plugin}/Source" "${{CMAKE_CURRENT_LIST_DIR}}/{plugin}" "${{CMAKE_SOURCE_DIR}}/Source" "${{JUCE_MODULES_DIR}}")',
            f'target_compile_definitions(HpeqNative{name} PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)',
            f'target_link_libraries(HpeqNative{name} PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)',
            f'list(APPEND HPEQ_NATIVE_LIBRARIES HpeqNative{name})']
        cmake+=lines
        receipt[plugin]=dict(source_tree=tree,source_commit=identity["source_commit"],version=identity["version"],compiled_sources=selected,input_sha256=hashes)
    write(dest/"NativeTargets.cmake","\n".join(cmake)+"\n")
    write(dest/"source-receipt.json",json.dumps(receipt,indent=2,sort_keys=True)+"\n")
    print("Generated isolated native wrappers:",len(receipt))


if __name__=="__main__":main()
