// HamloFader_Snapshot <out.png> [program]: the page (the editor with the preset screen) at its design size.

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"
#include "UI/FaderV2Panel.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
        return 2;
    hpeq::standalone::ModulePluginProcessor p(static_cast<hpeq::standalone::ModuleKind>(HPEQ_MODULE_KIND));
    if(argc>2&&juce::String(argv[2]).getIntValue()>=0)p.setCurrentProgram(juce::String(argv[2]).getIntValue());
    if(argc>3){p.setParameterValue("engine",juce::String(argv[3]).getIntValue()==2?0:1);p.setEditorPage(juce::String(argv[3]).getIntValue());}
    if(argc>4)p.setParameterValue("mode",juce::String(argv[4]).getIntValue());
    if(argc>7)
    {
        p.prepareToPlay(48000,256);
        const int mode=static_cast<int>(p.parameterValue("mode"));
        if(mode==1)p.requestAction(hpeq::standalone::FaderAction::StartLearn);
        if(mode==3&&p.getCurrentProgram()<8){p.setParameterValue("depthDb",18);for(int i=0;i<16;++i)p.setParameterValue("level"+juce::String(i).paddedLeft('0',2),i%4==0?.1:1.);}
        juce::AudioBuffer<float> block(2,256);juce::MidiBuffer midi;
        for(int n=0;n<240;++n){for(int c=0;c<2;++c)for(int i=0;i<256;++i)block.setSample(c,i,static_cast<float>(.1*std::sin(6.283185307179586*997*(n*256+i)/48000.)));p.processBlock(block,midi);}
        if(mode==1){p.requestAction(hpeq::standalone::FaderAction::StopLearn);block.clear();p.processBlock(block,midi);}
    }
    std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
    e->setSize(p.getSpec().editorWidth, p.getSpec().editorHeight);
    if(argc>6)e->setSize(juce::String(argv[5]).getIntValue(),juce::String(argv[6]).getIntValue());
    if(argc>8&&juce::String(argv[8])=="channels")
        if(auto* editor=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(e.get()))
            if(auto* panel=dynamic_cast<hpeq::ui::FaderV2Panel*>(editor->getPanel()))
                if(auto* section=dynamic_cast<juce::ComboBox*>(panel->control("advancedSection")))section->setSelectedId(1,juce::sendNotificationSync);
    const auto image = e->createComponentSnapshot(e->getLocalBounds(), true, 1.0f);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat png;
    return stream.openedOk() && png.writeImageToStream(image, stream) ? 0 : 1;
}
