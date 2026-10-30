// HamloEcho_Snapshot <out.png> [program]: the page (the editor with the preset screen) at its design size.

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
        return 2;
    hpeq::standalone::ModulePluginProcessor p(static_cast<hpeq::standalone::ModuleKind>(HPEQ_MODULE_KIND));
    p.setCurrentProgram(argc > 2 ? juce::String(argv[2]).getIntValue() : 1);
    const bool score=argc>3&&juce::String(argv[3]).getIntValue()!=0;if(score)p.setParameterValue("echoMode",1);
    std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
    const int width=argc>5?juce::String(argv[5]).getIntValue():p.getSpec().editorWidth;
    e->setSize(width,juce::roundToInt(width*(score?980.f:760.f)/1180.f));
    if(auto* editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(e.get()))if(score&&argc>4)editor->selectScorePage(juce::String(argv[4]).getIntValue());
    const auto image = e->createComponentSnapshot(e->getLocalBounds(), true, 1.0f);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat png;
    return stream.openedOk() && png.writeImageToStream(image, stream) ? 0 : 1;
}
