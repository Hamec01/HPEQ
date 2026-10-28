// HamloRedWall_Snapshot <out.png>: the page at its design size, with sample readings.

#include <juce_gui_basics/juce_gui_basics.h>

#include "Plugin/Parameters.h"
#include "UI/HamloRedWallPanel.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
        return 2;
    hamlo::ui::HamloRedWallPanel panel;
    panel.setSize(hamlo::redwall::kEditorWidth, hamlo::redwall::kEditorHeight);
    hamlo::ui::HamloRedWallPanel::Values v;
    v.knobs[0] = -6.0;
    panel.setValues(v);
    panel.settleMeters(-1.0f, 4.0f);
    const auto image = panel.createComponentSnapshot(panel.getLocalBounds(), true, 1.0f);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat png;
    return stream.openedOk() && png.writeImageToStream(image, stream) ? 0 : 1;
}
