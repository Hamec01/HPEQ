// HamloComp_Snapshot <out.png> [model 0..4] [source 0..3]: the page at its design size, with sample readings.

#include <juce_gui_basics/juce_gui_basics.h>

#include "Plugin/Parameters.h"
#include "UI/HamloCompPanel.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
        return 2;
    hamlo::ui::HamloCompPanel panel;
    panel.setSize(hamlo::comp::kEditorWidth, hamlo::comp::kEditorHeight);
    hamlo::ui::HamloCompPanel::Values v;
    v.knobs = { -24.0, 4.0, 6.0, 3.0, 120.0, 2.0, 1.0 };
    v.switches = { 0, 1, 1, argc > 2 ? juce::String(argv[2]).getIntValue() : 1, argc > 3 ? juce::String(argv[3]).getIntValue() : 1 };
    panel.setValues(v);
    panel.settleMeters(-12.0f, 6.0f);
    const auto image = panel.createComponentSnapshot(panel.getLocalBounds(), true, 1.0f);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat png;
    return stream.openedOk() && png.writeImageToStream(image, stream) ? 0 : 1;
}
