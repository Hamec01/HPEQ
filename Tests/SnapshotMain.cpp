// HamloComp_Snapshot <out.png> [program]: the page (the editor with the preset screen) at its design size, with sample
// readings; `program` = the host program to show (default: the first genre preset).

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Plugin/HamloCompEditor.h"
#include "Plugin/HamloCompProcessor.h"
#include "Plugin/Parameters.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
        return 2;
    hamlo::comp::HamloCompProcessor p;
    p.setCurrentProgram(argc > 2 ? juce::String(argv[2]).getIntValue() : p.getNumPrograms() - 200);
    std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
    auto* editor = dynamic_cast<hamlo::comp::HamloCompEditor*>(e.get());
    if (editor == nullptr)
        return 1;
    editor->setSize(hamlo::comp::kEditorWidth, hamlo::comp::kEditorHeight);
    editor->getPanel().settleMeters(-12.0f, 6.0f);
    const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1.0f);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat png;
    return stream.openedOk() && png.writeImageToStream(image, stream) ? 0 : 1;
}
