"""One-time, reviewable extraction from the frozen Zastava M0 sources."""
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]
OLD=ROOT/'out/s1/original/Source'


def load(name):return (OLD/name).read_text(encoding='utf-8')
def save(name,text):
    path=ROOT/name;assert path.resolve().is_relative_to(ROOT.resolve())
    path.parent.mkdir(parents=True,exist_ok=True);path.write_text(text,encoding='utf-8',newline='\n')


def block(text,start):
    at=text.index('{',start);depth=0;quote=None;escape=False
    for end in range(at,len(text)):
        ch=text[end]
        if quote:
            if escape:escape=False
            elif ch=='\\':escape=True
            elif ch==quote:quote=None
            continue
        if ch in ['"',"'"]:quote=ch
        elif ch=='{':depth+=1
        elif ch=='}':
            depth-=1
            if depth==0:return text[at:end+1],end+1
    raise ValueError('Unclosed block')


def function(text,signature):
    start=text.index(signature);_,end=block(text,start);return text[start:end]


def main():
    assert OLD.is_dir(),'Freeze M0 first'
    save('Source/DSP/ZastavaSettings.h','''#pragma once
#include <array>
namespace hpeq
{
// Original numeric slot/type image; no graph or unrelated DSP dependencies.
enum class FxType { Compressor, Limiter, Reverb, Delay, TransientShaper, Saturator, MonstaFx, StereoProcessor, GateExpander, Gain };
namespace dsp
{
struct FxSlotParams {bool active=false;FxType type=FxType::Gain;float mix=1.f;std::array<float,48> values{};};
namespace slot
{
constexpr int kGateThresholdDb=0,kGateRatio=1,kGateRangeDb=2,kGateAttackMs=3,kGateHoldMs=4,kGateReleaseMs=5,kGateHysteresisDb=6,
    kGateStyle=7,kGateLookahead=8,kGateKeyHpHz=9,kGateKeyLpHz=10,kGateKeyListen=11,kGateFlip=12,kGateOutputDb=13;
}
struct Transport {double bpm=120.;bool playing=false,hasPosition=false;double ppqAtBlockStart=0.;};
}
}
''')
    for name in ['DSP/HamloZastava.h','Standalone/ModuleVariant.h','Core/ModulePresets.h']:
        save('Source/'+name,load(name).replace('"DSP/LayerSnapshot.h"','"DSP/ZastavaSettings.h"').replace('"Core/FxProjectState.h"','"DSP/ZastavaSettings.h"'))

    m6=load('DSP/ProcessorsM6.cpp');header=load('DSP/ProcessorsM6.h')
    start=header.index('class GateProcessor');_,end=block(header,start)
    gate=header[start:end+1].replace('GateProcessor','LegacyGateProcessor')
    save('Source/DSP/LegacyGateProcessor.h','#pragma once\n#include "DSP/HamloZastava.h"\nnamespace hpeq::dsp\n{\n'+gate+'\n}\n')
    functions='\n\n'.join(function(m6,signature) for signature in ['bool GateProcessor::usesZastava','int GateProcessor::lookaheadSamples','void GateProcessor::process'])
    helpers='\n\n'.join(function(m6,signature) for signature in ['double coefficientFor','float linkedPeak'])
    save('Source/DSP/LegacyGateProcessor.cpp','#include "DSP/LegacyGateProcessor.h"\n#include <algorithm>\n#include <cmath>\nnamespace hpeq::dsp\n{\nnamespace\n{\n'+helpers+'\n}\n'+functions.replace('GateProcessor::','LegacyGateProcessor::')+'\n}\n')

    save('Source/DSP/LegacyZastavaCore.h','''#pragma once
#include "DSP/LegacyGateProcessor.h"
#include "DSP/DelayLine.h"
#include "DSP/Ramp.h"
namespace hpeq::dsp
{
const char* zastavaS1SourceIdentity() noexcept;
// Original single-insert slot, node delta and layer sum, in the original order.
class LegacyZastavaCore
{
public:
    void prepare(double sampleRate,int maximumBlockSize);
    void setTargets(const FxSlotParams&) noexcept;
    void process(float* const*,int channels,int samples,const Transport& = {}) noexcept;
    int getLatencySamples() const noexcept {return latency;}
    ZastavaCore::Meters getGateMeters(int) const noexcept;
private:
    LegacyGateProcessor gateFx;
    std::array<std::vector<float>,kMaxChannels> chain,work,aligned,bandDelayed;
    AlignmentDelay slotAlign,bandAlign,layerAlign;
    LinearRamp nodeMix,mix;
    FxSlotParams params;
    double fs=48000;
    int mixSteps=1,latency=0;
    bool running=false,hasTargets=false;
};
}
''')
    # The already-reviewed layer-sum extraction is adapted only for gate resets
    # and latency. The gate arithmetic itself above is copied from frozen M0.
    core=(ROOT.parent/'HamloShirota/Source/DSP/LegacyShirotaCore.cpp').read_text()
    core=core[:core.index('ShirotaCore::Meters')]
    core=core.replace('LegacyShirotaCore','LegacyZastavaCore').replace('LegacyStereoProcessor::latencySamples','LegacyGateProcessor::lookaheadSamples').replace('ShirotaCore::safeLookaheadSamples(fs)','ZastavaCore::lookaheadSamples(2,fs)').replace('stereo.prepare','gateFx.prepare').replace('stereo.process','gateFx.process')
    core=core.replace('running=true;mix.reset(0);','running=true;gateFx.reset();mix.reset(0);').replace('running=false;mix.reset(0);','running=false;gateFx.reset();mix.reset(0);')
    core=core.replace('#include <algorithm>','#include <algorithm>\n#include "ZastavaS1Identity.h"')
    core+='''ZastavaCore::Meters LegacyZastavaCore::getGateMeters(int) const noexcept
{return running&&gateFx.zastavaActive()?gateFx.zastava().meters():ZastavaCore::Meters{};}
const char* zastavaS1SourceIdentity() noexcept {return ZASTAVA_S1_SOURCE_ID;}
}
''';save('Source/DSP/LegacyZastavaCore.cpp',core)

    old=load('Standalone/ModuleVariant.cpp')
    helpers='\n\n'.join(function(old,signature) for signature in ['P knob','P choice','ModuleSpec makeZastava','float transformed'])
    variant='#include "Standalone/ModuleVariant.h"\n#include "DSP/LegacyGateProcessor.h"\n#include <cmath>\nnamespace hpeq::standalone\n{\nnamespace\n{\nusing P=ParameterSpec;\n'+helpers+'\n}\n'
    variant+='const ModuleSpec& moduleSpec(ModuleKind) {static const ModuleSpec value=makeZastava();return value;}\n\n'
    variant+=function(old,'juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout')+'\n\n'
    slot=function(old,'dsp::FxSlotParams makeSlot');a=slot.index('    if (kind == ModuleKind::Comp)');b=slot.index('    return slot;',a);slot=slot[:a]+slot[b:]
    variant+=slot+'\n\nint maximumLatencySamples(ModuleKind,double fs) noexcept {return dsp::ZastavaCore::lookaheadSamples(2,fs);}\n}\n'
    save('Source/Standalone/ModuleVariant.cpp',variant)
    old=load('Core/ModulePresets.cpp');start=old.index('const std::vector<ModulePreset> kGate');_,end=block(old,start)
    presets='#include "Core/ModulePresets.h"\nnamespace hpeq::presets\n{\nnamespace\n{\n'+old[start:end+1]+'\n}\n'
    presets+='const std::vector<ModulePreset>& factoryModulePresets(FxType type) {static const std::vector<ModulePreset> empty;return type==FxType::GateExpander?kGate:empty;}\n'
    presets+=function(old,'double presetValue')+'\n}\n';save('Source/Core/ModulePresets.cpp',presets)

    header=load('Standalone/ModulePluginProcessor.h').replace('"DSP/LayerEngine.h"','"DSP/LegacyZastavaCore.h"').replace('dsp::LayerEngine engine','dsp::LegacyZastavaCore engine').replace('const dsp::FxNodeProcessor& moduleNode() const noexcept { return engine.getMixNode(); }','const dsp::LegacyZastavaCore& moduleNode() const noexcept { return engine; }')
    save('Source/Standalone/ModulePluginProcessor.h',header)
    processor=load('Standalone/ModulePluginProcessor.cpp').replace('#include "DSP/ProcessorsM6.h"','#include "ZastavaS1Identity.h"\n#include <stdexcept>').replace('#include "DSP/ReverbProcessor.h"\n','')
    processor=processor.replace('    bypassParameter = dynamic_cast', '    if(juce::String(dsp::zastavaS1SourceIdentity())!=ZASTAVA_S1_SOURCE_ID)throw std::runtime_error("Mixed Zastava S1 translation units");\n    bypassParameter = dynamic_cast',1)
    # Both snapshot-publish sequences have identical text except the active flag.
    snapshot=re.compile(r'    dsp::LayerSnapshot snapshot;.*?    engine.update\(\);',re.S)
    processor,n=snapshot.subn('    engine.setTargets(currentSlot(isModuleOn()));',processor);assert n==2
    at=processor.index('void ModulePluginProcessor::processBlock');processor=processor[:at]+processor[at:].replace('engine.setTargets(currentSlot(isModuleOn()));','engine.setTargets(currentSlot(on));',1)
    start=processor.index('double ModulePluginProcessor::getTailLengthSeconds() const');_,end=block(processor,start)
    processor=processor[:start]+'''double ModulePluginProcessor::getTailLengthSeconds() const
{return static_cast<double>(fixedLatency)/currentSampleRate.load(std::memory_order_relaxed);}
'''+processor[end:]
    save('Source/Standalone/ModulePluginProcessor.cpp',processor)

    editor=load('Standalone/ModulePluginEditor.cpp');editor=re.sub(r'#include "UI/Hamlo(?!Zastava)[^"]+Panel.h"\n','',editor)
    start=editor.index('    switch (processor.getKind())');_,end=block(editor,start)
    selected=editor.index('case ModuleKind::Zastava:',start);body,_=block(editor,selected);body=body.replace('            break;\n','')
    editor=editor[:start]+'    '+body+editor[end:];save('Source/Standalone/ModulePluginEditor.cpp',editor)

    cmake=(ROOT/'CMakeLists.txt').read_text();start=cmake.index('add_library(HamloZastava_Shared STATIC');end=cmake.index(')',start)+1
    units=['Core/ModulePresets','DSP/HamloZastava','DSP/LegacyGateProcessor','DSP/LegacyZastavaCore','UI/BlackSteel','UI/HamloZastavaPanel','UI/PresetScreen','Standalone/ModuleVariant','Standalone/ModulePluginProcessor','Standalone/ModulePluginEditor']
    cmake=cmake[:start]+'add_library(HamloZastava_Shared STATIC\n'+''.join('    Source/'+unit+'.cpp\n' for unit in units)+')'+cmake[end:]
    save('CMakeLists.txt',cmake)
    print('Extracted ten gate-owned translation units; frozen M0 unchanged')


if __name__=='__main__':main()
