// HamloVerb_Snapshot <out.png> [program]: the page (the editor with the preset screen) at its design size.

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Standalone/ModulePluginEditor.h"
#include "UI/VerbV2Panel.h"
#include "Standalone/ModulePluginProcessor.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
        return 2;
    auto owned = std::make_unique<hpeq::standalone::ModulePluginProcessor>(static_cast<hpeq::standalone::ModuleKind>(HPEQ_MODULE_KIND));
    auto& p = *owned;
    p.setCurrentProgram(argc > 2 ? juce::String(argv[2]).getIntValue() : 1);
    if (argc > 6) p.setParameterValue("keepDry", juce::String(argv[6]).getIntValue());
    if (argc > 7) p.setParameterValue("wetDb", juce::String(argv[7]).getDoubleValue());
    std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
    e->setSize(p.getSpec().editorWidth, p.getSpec().editorHeight);
    if (auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(e.get()))
        if (auto* panel = dynamic_cast<hpeq::ui::VerbV2Panel*>(editor->getPanel()))
            if (argc > 3 && juce::String(argv[3]) != "main") { panel->showAdvanced(true); panel->showAdvancedSection(juce::String(argv[3]).getIntValue()); }
    if (argc > 5) e->setSize(juce::String(argv[4]).getIntValue(), juce::String(argv[5]).getIntValue());
    const auto image = e->createComponentSnapshot(e->getLocalBounds(), true, 1.0f);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat png;
    return stream.openedOk() && png.writeImageToStream(image, stream) ? 0 : 1;
}
