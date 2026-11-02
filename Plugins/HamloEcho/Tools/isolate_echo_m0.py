"""Extract only Echo from the pinned M0 commit; never read or modify HPEQ/Verb.

The delay algorithms are copied as text, preserving their evaluation order. The
separate compatibility lab, not this generator, establishes sound equivalence.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REV = "ede0db8"


def original(path):
    return subprocess.check_output(["git", "show", f"{REV}:{path}"], cwd=ROOT).decode("utf-8").replace("\r\n", "\n")


def save(path, text):
    (ROOT / path).write_text(text, encoding="utf-8", newline="\n")


h = original("Source/DSP/ProcessorsM6.h")
h = h[h.index("class ClassicDelay"):h.index("// Monsta FX")]
save("Source/DSP/LegacyDelay.h", '#pragma once\n\n#include "DSP/HamloEcho.h"\n\nnamespace hpeq::dsp\n{\n' + h + '}\n')
c = original("Source/DSP/ProcessorsM6.cpp")
helper = c[c.index("float readLinear("):c.index("} // namespace")]
body = c[c.index("namespace\n{\nconstexpr std::array<double, 12> kDivisionBeats"):c.index("// ------------------------------------------------------------------------------------- Monsta")]
save("Source/DSP/LegacyDelay.cpp", '#include "DSP/LegacyDelay.h"\n#include <algorithm>\n#include <cmath>\n\nnamespace hpeq::dsp\n{\nnamespace\n{\nconstexpr double kPi = 3.14159265358979323846;\n' + helper + '}\n' + body + '}\n')

c = original("Source/Standalone/ModuleVariant.cpp")
prefix = c[:c.index("ModuleSpec makeComp()")].replace('#include "DSP/FxNodeProcessor.h"\n', '')
echo = c[c.index("ModuleSpec makeEcho()"):c.index("ModuleSpec makeMag()")]
layout = c[c.index("juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout"):c.index("int maximumLatencySamples")]
layout = layout.replace('transformed(value, p.transform)', 'value')
layout = layout.replace('    if (kind == ModuleKind::Comp)\n        slot.values[static_cast<size_t>(dsp::slot::kKneeDb)] = juce::jmin(24.0f, slot.values[static_cast<size_t>(dsp::slot::kKneeDb)]);\n', '')
save("Source/Standalone/ModuleVariant.cpp", prefix + echo + '} // namespace\n\nconst ModuleSpec& moduleSpec(ModuleKind)\n{\n    static const ModuleSpec echo = makeEcho();\n    return echo;\n}\n\n' + layout + 'int maximumLatencySamples(ModuleKind, double) noexcept { return 0; }\n\n} // namespace hpeq::standalone\n')

c = original("Source/Core/ModulePresets.cpp")
delay = c[c.index("const std::vector<ModulePreset> kDelay"):c.index("const std::vector<ModulePreset> kTransient")]
value = c[c.index("double presetValue("):]
save("Source/Core/ModulePresets.cpp", '#include "Core/ModulePresets.h"\n\nnamespace hpeq::presets\n{\nnamespace\n{\n' + delay + '}\n\nconst std::vector<ModulePreset>& factoryModulePresets(FxType) { return kDelay; }\n\n' + value)

for path in ["Source/DSP/HamloEcho.h", "Source/UI/HamloEchoPanel.h", "Source/Standalone/ModuleVariant.h"]:
    save(path, original(path).replace('"DSP/LayerSnapshot.h"', '"DSP/EchoSettings.h"'))
path = "Source/Core/ModulePresets.h"
save(path, original(path).replace('"Core/FxProjectState.h"', '"DSP/EchoSettings.h"'))

path = "Source/Standalone/ModulePluginEditor.cpp"
c = original(path)
for name in ["Comp", "Mag", "RedWall", "Shirota", "Udarnik", "Verb", "Zastava"]:
    c = c.replace(f'#include "UI/Hamlo{name}Panel.h"\n', '')
start = c.index('    switch (processor.getKind())')
echo = c[c.index('        case ModuleKind::Echo:', start):c.index('        case ModuleKind::Mag:', start)]
echo = echo[echo.index('        {') + len('        {\n'):echo.rindex('            break;')]
c = c[:start] + '    {\n' + echo + '    }\n' + c[c.index('    // Hamlo line', start):]
save(path, c)

path = "Source/Standalone/ModulePluginProcessor.h"
c = original(path).replace('"DSP/LayerEngine.h"', '"DSP/LegacyEchoCore.h"')
c = c.replace('const dsp::FxNodeProcessor& moduleNode() const noexcept { return engine.getMixNode(); }', 'const dsp::LegacyEchoCore& moduleNode() const noexcept { return engine; }')
c = c.replace('dsp::LayerEngine engine;', 'dsp::LegacyEchoCore engine;')
save(path, c)

path = "Source/Standalone/ModulePluginProcessor.cpp"
c = original(path).replace('#include "DSP/ProcessorsM6.h"\n', '').replace('#include "DSP/ReverbProcessor.h"\n', '')
for active in ['isModuleOn()', 'on']:
    snapshot = f'''    dsp::LayerSnapshot snapshot;
    snapshot.mix.active = true;
    snapshot.mix.channel = ChannelMode::Stereo;
    snapshot.mix.numSlots = 1;
    snapshot.mix.slots[0] = currentSlot({active});
    engine.publish(snapshot);
    engine.update();'''
    if snapshot not in c:
        raise RuntimeError('Expected M0 snapshot block is missing')
    c = c.replace(snapshot, f'    engine.setTargets(currentSlot({active}));')
c = c.replace('    if (kind == ModuleKind::Verb)\n        tail = dsp::ReverbProcessor::tailSeconds(slot, bpm);\n    else if (kind == ModuleKind::Echo)\n        tail = dsp::DelayProcessor::tailSeconds(slot, bpm);', '    tail = dsp::DelayProcessor::tailSeconds(slot, bpm);')
save(path, c)

path = "CMakeLists.txt"
c = (ROOT / path).read_text(encoding="utf-8")
start, end = c.index('add_library(HamloEcho_Shared STATIC'), c.index('target_include_directories(HamloEcho_Shared')
c = c[:start] + '''add_library(HamloEcho_Shared STATIC
    Source/Core/ModulePresets.cpp
    Source/DSP/HamloEcho.cpp
    Source/DSP/LegacyDelay.cpp
    Source/DSP/LegacyEchoCore.cpp
    Source/UI/BlackSteel.cpp
    Source/UI/HamloEchoPanel.cpp
    Source/Standalone/ModuleVariant.cpp
    Source/Standalone/ModulePluginProcessor.cpp
    Source/Standalone/ModulePluginEditor.cpp
    Source/UI/PresetScreen.cpp)
''' + c[end:]
c = c.replace("# --- Shared code: HPEQ's module-plug-in stack (Source/ is HPEQ's, verbatim; docs/SYNC_WITH_HPEQ.md) -----------", '# --- Echo-only shared code; M0 provenance and bit-exact verification: docs/SYNC_WITH_HPEQ.md -----------')
save(path, c)
print('Extracted Echo-only build from M0', REV)
