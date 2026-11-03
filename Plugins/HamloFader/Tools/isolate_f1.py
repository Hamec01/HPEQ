"""Apply the Fader-only build/host closure after successful original capture."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'out/audit/original'
def read(n):return (BASE/n).read_text(encoding='utf-8')
def write(n,s):(ROOT/n).write_text(s,encoding='utf-8',newline='\n')

before=json.loads((ROOT/'docs/reports/f1/before.json').read_text());assert before['failures']==0
assert (ROOT/'Source/Standalone/ModulePluginProcessor.cpp').read_text()==read('Source/Standalone/ModulePluginProcessor.cpp'),'Isolation already applied or source modified; refusing to overwrite'
# Preserve the complete Gain body and its member layout rather than rederive the arithmetic.
header=read('Source/DSP/Processors.h');body=header[header.index('class GainProcessor'):header.index('// Feed-forward compressor')]
write('Source/DSP/FaderGain.h','#pragma once\n#include <array>\n#include <atomic>\n#include <vector>\n#include "DSP/DspConfig.h"\n#include "DSP/FaderSettings.h"\nnamespace hpeq::dsp\n{\n'+body+'}\n')
body=read('Source/DSP/Processors.cpp');body=body[body.index('void GainProcessor::prepare'):body.index('void CompressorProcessor::prepare')]
write('Source/DSP/FaderGain.cpp','#include "DSP/FaderGain.h"\n#include <algorithm>\n#include <cmath>\nnamespace hpeq::dsp\n{\n'+body+'}\n')

s=read('Source/Core/ModulePresets.cpp');gain=s[s.index('const std::vector<ModulePreset> kGain'):s.index('// Revolution:')]
fallback=s[s.index('double presetValue'):]
write('Source/Core/ModulePresets.cpp','#include "Core/ModulePresets.h"\nnamespace hpeq::presets\n{\nnamespace\n{\n'+gain+'}\nconst std::vector<ModulePreset>& factoryModulePresets(FxType)\n{return kGain;}\n\n'+fallback)
write('Source/Core/ModulePresets.h',read('Source/Core/ModulePresets.h').replace('"Core/FxProjectState.h"','"DSP/FaderSettings.h"'))

s=read('Source/Standalone/ModuleVariant.cpp');helpers=s[s.index('using P = ParameterSpec;'):s.index('ModuleSpec makeComp()')]
fader=s[s.index('ModuleSpec makeFader()'):s.index('float transformed(')]
layout=s[s.index('juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout'):s.index('dsp::FxSlotParams makeSlot')]
write('Source/Standalone/ModuleVariant.cpp','#include "Standalone/ModuleVariant.h"\n#include <cmath>\nnamespace hpeq::standalone\n{\nnamespace\n{\n'+helpers+fader+'}\nconst ModuleSpec& moduleSpec(ModuleKind){static const auto fader=makeFader();return fader;}\n'+layout+'''dsp::FxSlotParams makeSlot(ModuleKind kind,const std::vector<std::atomic<float>*>& values,bool active) noexcept
{
    const auto& spec=moduleSpec(kind);dsp::FxSlotParams slot;slot.active=active;slot.type=spec.type;slot.mix=1;
    const auto count=juce::jmin(values.size(),spec.parameters.size());
    for(size_t i=0;i<count;++i){const auto& p=spec.parameters[i];const float value=values[i]->load(std::memory_order_relaxed);if(p.slotValueIndex<0)slot.mix=juce::jlimit(0.f,1.f,value);else slot.values[static_cast<size_t>(p.slotValueIndex)]=value;}
    return slot;
}
int maximumLatencySamples(ModuleKind,double) noexcept{return 0;}
}
''')
write('Source/Standalone/ModuleVariant.h',read('Source/Standalone/ModuleVariant.h').replace('"DSP/LayerSnapshot.h"','"DSP/FaderSettings.h"'))
s=read('Source/Standalone/ModulePluginProcessor.h').replace('"DSP/LayerEngine.h"','"DSP/LegacyFaderCore.h"').replace('dsp::FxNodeProcessor& moduleNode()','dsp::LegacyFaderCore& moduleNode()').replace('dsp::LayerEngine engine;','dsp::FaderEngine engine;');write('Source/Standalone/ModulePluginProcessor.h',s)
s=read('Source/Standalone/ModulePluginProcessor.cpp').replace('#include "DSP/ProcessorsM6.h"\n','').replace('#include "DSP/ReverbProcessor.h"\n','').replace('dsp::LayerSnapshot snapshot;','dsp::FaderSnapshot snapshot;')
start=s.index('    const auto slot = currentSlot(true);',s.index('double ModulePluginProcessor::getTailLengthSeconds'))
end=s.index('\n}',start)
s=s[:start]+'    return static_cast<double>(fixedLatency) / currentSampleRate.load(std::memory_order_relaxed);'+s[end:];write('Source/Standalone/ModulePluginProcessor.cpp',s)
s=read('Source/Standalone/ModulePluginEditor.cpp')
import re
s=re.sub(r'#include "UI/Hamlo(?!Fader)[^\n]+\n','',s)
start=s.index('    switch (processor.getKind())');end=s.index('    // Hamlo line',start)
case=s[s.index('            auto p = std::make_unique<HamloFaderPanel>();',start):s.index('            break;',s.index('case ModuleKind::Fader:',start))]
s=s[:start]+'    {\n'+case+'    }\n\n'+s[end:];write('Source/Standalone/ModulePluginEditor.cpp',s)

s=(ROOT/'CMakeLists.txt').read_text();start=s.index('add_library(HamloFader_Shared STATIC');end=s.index(')',start)
units=['Core/ModulePresets.cpp','DSP/FaderGain.cpp','DSP/LegacyFaderCore.cpp','UI/BlackSteel.cpp','UI/HamloFaderPanel.cpp','UI/PresetScreen.cpp','Standalone/ModuleVariant.cpp','Standalone/ModulePluginProcessor.cpp','Standalone/ModulePluginEditor.cpp']
s=s[:start]+'add_library(HamloFader_Shared STATIC\n'+''.join('    Source/'+n+'\n' for n in units)+s[end:];write('CMakeLists.txt',s)
print('Fader production closure:',len(units),'units; original Gain body preserved')
