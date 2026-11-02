// HamloRevolution_Snapshot <out.png> [program] [scale] [generate]: the page (the editor with the preset screen) at its opening size x scale
// (R3: the page opens at 75 %; scale 1.3333 = the design size 1180 x 1280).

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
    auto owned = std::make_unique<hpeq::standalone::ModulePluginProcessor>(static_cast<hpeq::standalone::ModuleKind>(HPEQ_MODULE_KIND)); // large: heap
    auto& p = *owned;
    p.setCurrentProgram(argc > 2 ? juce::String(argv[2]).getIntValue() : 1);
#if HPEQ_MODULE_KIND == 9
    // R4: "generate" = the SIMPLE page with a generated pattern
    if (argc > 4 && juce::String(argv[4]).contains("generate"))
        p.generateSimple(false);
    if (argc > 4 && juce::String(argv[4]).contains("advanced")) // R6: the ADVANCED page
        p.setPage(hpeq::standalone::ModulePluginProcessor::Page::Advanced);
#endif
    std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
    e->setSize(p.getSpec().editorWidth, p.getSpec().editorHeight);
    const float scale = argc > 3 ? static_cast<float>(juce::String(argv[3]).getDoubleValue()) : 1.0f;
    const auto image = e->createComponentSnapshot(e->getLocalBounds(), true, scale);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat png;
    return stream.openedOk() && png.writeImageToStream(image, stream) ? 0 : 1;
}
