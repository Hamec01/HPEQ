#include "Standalone/ModulePluginEditor.h"
#include "UI/FaderV2Panel.h"
#include <cmath>

namespace hamlo::test
{
using P=hpeq::standalone::ModulePluginProcessor;
using Panel=hpeq::ui::FaderV2Panel;
class F4UITests final:public juce::UnitTest
{
    static void audio(P& p,int blocks)
    {
        juce::AudioBuffer<float> x(2,256);juce::MidiBuffer midi;
        for(int n=0;n<blocks;++n){for(int c=0;c<2;++c)for(int i=0;i<256;++i)x.setSample(c,i,static_cast<float>(.1*std::sin(6.283185307179586*997*(i+256*n)/48000)));p.processBlock(x,midi);}
    }
public:
    F4UITests():juce::UnitTest("F4 interface","HamloFader"){}
    void runTest()override
    {
        beginTest("Editor open is read-only; all three pages select their actual engine and recall");
        auto p=std::make_unique<P>(hpeq::standalone::ModuleKind::Fader);
        juce::MemoryBlock before,after;p->getStateInformation(before);
        auto editor=std::unique_ptr<juce::AudioProcessorEditor>(p->createEditor());
        p->getStateInformation(after);expect(before==after);
        auto* panel=dynamic_cast<Panel*>(dynamic_cast<hpeq::standalone::ModulePluginEditor*>(editor.get())->getPanel());expect(panel!=nullptr);if(!panel)return;
        for(int page:{2,0,1}){panel->selectPage(page);expectEquals(p->editorPage(),page);expectWithinAbsoluteError(p->parameterValue("engine"),page==2?0.:1.,0.);}
        p->getStateInformation(before);editor.reset();editor.reset(p->createEditor());p->getStateInformation(after);expect(before==after);
        panel=dynamic_cast<Panel*>(dynamic_cast<hpeq::standalone::ModulePluginEditor*>(editor.get())->getPanel());
        beginTest("Real slider and combo callbacks edit host parameters; external changes refresh");
        panel->selectPage(0);
        auto* gain=dynamic_cast<juce::Slider*>(panel->control("gainDb"));expect(gain!=nullptr);if(gain){gain->setValue(-3.25,juce::sendNotificationSync);expectWithinAbsoluteError(p->parameterValue("gainDb"),-3.25,1e-4);p->setParameterValue("gainDb",-7);panel->refresh();expectWithinAbsoluteError(gain->getValue(),-7.,1e-4);}
        auto* routing=dynamic_cast<juce::ComboBox*>(panel->control("routing"));expect(routing!=nullptr);if(routing){routing->setSelectedId(2,juce::sendNotificationSync);expectWithinAbsoluteError(p->parameterValue("routing"),1.,0.);}
        for(int mode=0;mode<4;++mode)
        {
            const char* names[]{"TRIM","MATCH","RIDE","MOTION"};auto* button=dynamic_cast<juce::TextButton*>(panel->control(names[mode]));expect(button!=nullptr);if(button)button->onClick();expectWithinAbsoluteError(p->parameterValue("mode"),static_cast<double>(mode),0.);
        }
        beginTest("Motion graph edits are saved/automatable; point count clamps selection");
        panel->selectPage(1);p->setParameterValue("mode",3);panel->refresh();
        auto* points=dynamic_cast<juce::Slider*>(panel->control("points"));expect(points!=nullptr);if(points)points->setValue(32,juce::sendNotificationSync);
        panel->editPoint(31,.375);expectWithinAbsoluteError(p->parameterValue("level31"),.375,1e-5);expect(p->isProgramModified());
        auto* selected=dynamic_cast<juce::ComboBox*>(panel->control("selectedPoint"));expect(selected&&selected->getSelectedId()==32);
        auto* exact=dynamic_cast<juce::Slider*>(panel->control("pointValue"));expect(exact!=nullptr);if(exact)exact->setValue(.625,juce::sendNotificationSync);expectWithinAbsoluteError(p->parameterValue("level31"),.625,1e-5);
        p->getStateInformation(before);auto q=std::make_unique<P>(hpeq::standalone::ModuleKind::Fader);q->setStateInformation(before.getData(),static_cast<int>(before.getSize()));expectEquals(q->editorPage(),1);expectWithinAbsoluteError(q->parameterValue("level31"),.625,1e-5);
        if(points)points->setValue(2,juce::sendNotificationSync);panel->refresh();expect(selected&&selected->getSelectedId()<=2);
        beginTest("Match never auto-applies; queued requests wait, proposal is deliberate, stale Apply disabled");
        p->setCurrentProgram(0);panel->selectPage(0);p->setParameterValue("mode",1);p->prepareToPlay(48000,256);panel->refresh();
        auto* apply=dynamic_cast<juce::TextButton*>(panel->control("applyMatch"));expect(apply&&!apply->isEnabled());
        audio(*p,4);panel->refresh();auto* start=dynamic_cast<juce::TextButton*>(panel->control("startLearn"));expect(start&&start->isEnabled());if(start)start->onClick();panel->refresh();expect(apply&&!apply->isEnabled());
        audio(*p,120);panel->refresh();auto* stop=dynamic_cast<juce::TextButton*>(panel->control("stopLearn"));expect(stop&&stop->isEnabled());if(stop)stop->onClick();
        expectWithinAbsoluteError(p->parameterValue("matchedDb"),0.,0.);audio(*p,1);panel->refresh();expect(apply&&apply->isEnabled());if(apply)apply->onClick();expect(std::abs(p->parameterValue("matchedDb"))>.1);
        p->setCurrentProgram(0);panel->selectPage(0);p->setParameterValue("mode",1);panel->refresh();expect(apply&&!apply->isEnabled());expect(!panel->apply());
        beginTest("Full/busy command queue is reported; resizing preserves preset geometry");
        auto busy=std::make_unique<P>(hpeq::standalone::ModuleKind::Fader);Panel busyPanel(*busy);for(int i=0;i<64;++i)expect(busy->requestAction(hpeq::standalone::FaderAction::Reset));expect(!busyPanel.action(hpeq::standalone::FaderAction::StartLearn));
        auto* typed=dynamic_cast<hpeq::standalone::ModulePluginEditor*>(editor.get());
        for(int width:{590,1180,2360}){editor->setSize(width,width*700/1180);panel->resized();const auto bounds=typed->getPresetScreen().getBounds();expectEquals(bounds.getX(),static_cast<int>(56.*width/1180));expectEquals(bounds.getY(),static_cast<int>(20.*width/1180));expectEquals(bounds.getWidth(),static_cast<int>(336.*width/1180));}
    }
};static F4UITests f4UITests;
}
