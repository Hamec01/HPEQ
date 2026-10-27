// HamloUdarnik_Snapshot <out.png> [mode 0..2]: the page at its design size, with sample readings.

#include <juce_gui_basics/juce_gui_basics.h>

#include "Plugin/Parameters.h"
#include "UI/HamloUdarnikPanel.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
        return 2;
    hamlo::ui::HamloUdarnikPanel panel;
    panel.setSize(hamlo::udarnik::kEditorWidth, hamlo::udarnik::kEditorHeight);
    hamlo::ui::HamloUdarnikPanel::Values v;
    v.knobs = { 6.0, -3.0, 0.6, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, argc > 2 ? juce::String(argv[2]).getDoubleValue() : 1.0 };
    panel.setValues(v);
    panel.settleMeters({ 3.5f, -2.0f, 6.0f, 12, 0.9f, -3.0f });
    const auto image = panel.createComponentSnapshot(panel.getLocalBounds(), true, 1.0f);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat png;
    return stream.openedOk() && png.writeImageToStream(image, stream) ? 0 : 1;
}
