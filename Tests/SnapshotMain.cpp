// HamloZastava_Snapshot <out.png> [program]: the page (the editor with the preset screen) at its design size.

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
    if(argc>3)
    {
        p.setParameterValue("v2_engine",argc>4?juce::String(argv[4]).getIntValue():1);
        p.setEditorView(juce::String(argv[3]).getIntValue());
        p.setEditorTab(argc>5?juce::String(argv[5]).getIntValue():0);
        p.setParameterValue("v2_steps",32);
        if(argc>7)p.setParameterValue("v2_listen",juce::String(argv[7]).getIntValue());
        if(argc>8)p.setParameterValue("v2_external",juce::String(argv[8]).getIntValue());
        if(argc>9)p.setParameterValue("v2_profile",juce::String(argv[9]).getIntValue());
        if(argc>10)
        {
            p.setParameterValue("v2_enabled",juce::String(argv[10]).getIntValue());p.prepareToPlay(48000,256);
            juce::AudioBuffer<float> data(2,4096);juce::MidiBuffer midi;
            for(int block=0;block<4;++block){for(int c=0;c<2;++c)for(int n=0;n<4096;++n)data.setSample(c,n,static_cast<float>(.1*std::sin((block*4096+n)*.13)));p.processBlock(data,midi);}
        }
    }
    std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
    const double scale=argc>6?juce::String(argv[6]).getDoubleValue():1;
    e->setSize(juce::roundToInt(p.getSpec().editorWidth*scale), juce::roundToInt(p.getSpec().editorHeight*scale));
    const auto image = e->createComponentSnapshot(e->getLocalBounds(), true, 1.0f);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat png;
    return stream.openedOk() && png.writeImageToStream(image, stream) ? 0 : 1;
}
