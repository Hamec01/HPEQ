#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"
#include "AllocationAudit.h"
#include "UI/ScorePanel.h"

namespace
{
using hpeq::standalone::ModulePluginProcessor;
std::unique_ptr<ModulePluginProcessor> make()
{
    auto p=std::make_unique<ModulePluginProcessor>(hpeq::standalone::ModuleKind::Echo);
    p->setParameterValue("echoMode",1);p->setParameterValue("scoreSpaceMix",0);p->setParameterValue("scoreFeedback",0);
    p->setParameterValue("scoreMix",1);p->setParameterValue("tap1Gain",0);p->setParameterValue("tap1Note",60);p->setParameterValue("tap1Time",10);
    for(int t=2;t<=8;++t)p->setParameterValue("tap"+juce::String(t)+"On",0);
    return p;
}
juce::AudioBuffer<float> run(ModulePluginProcessor& p,const juce::AudioBuffer<float>& input,int block)
{
    p.prepareToPlay(48000,256);auto out=input;juce::MidiBuffer midi;
    for(int start=0;start<out.getNumSamples();start+=block)
    {juce::AudioBuffer<float> part(out.getArrayOfWritePointers(),2,start,juce::jmin(block,out.getNumSamples()-start));p.processBlock(part,midi);}
    return out;
}
class ScoreIntegrationTests final:public juce::UnitTest
{
public:
    ScoreIntegrationTests():juce::UnitTest("Score integration","HamloEcho"){}
    void runTest() override
    {
        beginTest("Neutral Default starts dry and resets a playing Send/locked/frozen instance");
        {
            auto p=std::make_unique<ModulePluginProcessor>(hpeq::standalone::ModuleKind::Echo);
            expectWithinAbsoluteError(p->parameterValue("echoMode"),1.,1e-6);
            for(const auto* id:{"mix","scoreMix","scoreFeedback","feedback","scoreSpaceMix","scoreGlitchAmount","scoreGlitch","scoreFeedbackPitch","scoreDrive","modDepth","scoreSwing"})expectWithinAbsoluteError(p->parameterValue(id),0.,1e-6,id);
            for(int t=1;t<=8;++t){expectWithinAbsoluteError(p->parameterValue("tap"+juce::String(t)+"Note"),60.,1e-6);expectWithinAbsoluteError(p->parameterValue("tap"+juce::String(t)+"On"),t==1?1.:0.,1e-6);}
            const auto input=hamlo::test::drums(.3);expect(hamlo::test::bitExact(run(*p,input,127),input));
            p->setCurrentProgram(17);(void)run(*p,hamlo::test::drums(.6),127);
            p->setParameterValue("scoreFreeze",1);p->setParameterValue("scoreRouting",1);p->setParameterValue("scoreMixLock",1);p->setParameterValue("bypass",1);
            juce::AudioBuffer<float> held(2,256);held.clear();juce::MidiBuffer midi;p->processBlock(held,midi);
            p->setCurrentProgram(0);auto actual=input;
            for(int start=0;start<actual.getNumSamples();start+=127){juce::AudioBuffer<float> view(actual.getArrayOfWritePointers(),2,start,juce::jmin(127,actual.getNumSamples()-start));p->processBlock(view,midi);}
            expect(hamlo::test::bitExact(actual,input),"Default must clear retiring tails and restore dry Insert even with Mix Lock");
            for(const auto* id:{"scoreRouting","scoreMixLock","scoreFreeze","bypass"})expectWithinAbsoluteError(p->parameterValue(id),0.,1e-6,id);
            expect(!p->presetModified());
            p->setParameterValue("scoreMix",1);juce::AudioBuffer<float> impulse(2,512);impulse.clear();impulse.setSample(0,100,.25f);impulse.setSample(1,100,.25f);
            const auto echo=run(*p,impulse,64);expectWithinAbsoluteError(echo.getSample(0,148),.25f,1e-6f,"Raising Mix must expose the prepared 1ms unpitched reflection");
        }
        beginTest("Original parameter indices, append-only additions and note conventions");
        {
            auto p=make();expectEquals(p->getParameters().size(),28+hpeq::standalone::scoreparam::Count);
            auto* bypass=dynamic_cast<juce::RangedAudioParameter*>(p->getParameters()[27]);expect(bypass!=nullptr&&bypass->paramID=="bypass");
            expectEquals(hpeq::standalone::noteName(60,true),juce::String("C5"));expectEquals(hpeq::standalone::noteName(60,false),juce::String("C4"));
            expectEquals(hpeq::standalone::parseNote("C-5",true),60);expectEquals(hpeq::standalone::parseNote("Db4",false),61);
            expectEquals(hpeq::standalone::parseNote("C-1",false),0);expectEquals(hpeq::standalone::parseNote("hello",true),-1);
        }
        juce::AudioBuffer<float> impulse(2,9600);impulse.clear();impulse.setSample(0,1000,.25f);impulse.setSample(1,1000,.25f);
        beginTest("Unity forward exact requested time, zero host latency and neutral complementary bands");
        for(bool bands:{false,true})
        {
            auto p=make();p->setParameterValue("scoreBands",bands?1:0);
            for(int b=1;b<=3;++b)p->setParameterValue("band"+juce::String(b)+"Feedback",0);
            auto out=run(*p,impulse,127);expectEquals(p->getLatencySamples(),0);
            for(int ch=0;ch<2;++ch)
            {
                expectWithinAbsoluteError(out.getSample(ch,1480),.25f,1e-6f);
                double error=0;for(int i=0;i<out.getNumSamples();++i)error=std::max(error,std::abs(static_cast<double>(out.getSample(ch,i)-(i==1480?.25f:0))));
                expect(error<=1e-6,"Maximum impulse/reference error "+juce::String(error));
            }
        }
        beginTest("Insert/Send preserve stored Mix, Mix zero is dry, Width zero is mono");
        {
            auto p=make();p->setParameterValue("scoreMix",0);expect(hamlo::test::bitExact(run(*p,impulse,64),impulse));
            p->setParameterValue("scoreMix",.25);p->setParameterValue("scoreRouting",1);auto out=run(*p,impulse,64);
            expectWithinAbsoluteError(p->parameterValue("scoreMix"),.25,1e-6);expectWithinAbsoluteError(out.getSample(0,1000),0.f,1e-6f);expectWithinAbsoluteError(out.getSample(0,1480),.25f,1e-6f);
            p->setParameterValue("scoreRouting",0);out=run(*p,impulse,64);expectWithinAbsoluteError(out.getSample(0,1000),.1875f,1e-6f);expectWithinAbsoluteError(out.getSample(0,1480),.0625f,1e-6f);
            p->setParameterValue("scoreWidth",0);p->setParameterValue("scoreMix",1);auto asymmetric=impulse;asymmetric.clear(1,0,asymmetric.getNumSamples());out=run(*p,asymmetric,127);
            expect(std::memcmp(out.getReadPointer(0),out.getReadPointer(1),out.getNumSamples()*sizeof(float))==0);
        }
        beginTest("Complete project state overrides Mix Lock and preserves program/modified metadata");
        {
            auto p=make();p->setCurrentProgram(2);p->setParameterValue("echoMode",1);p->setParameterValue("scoreMix",.23);p->setParameterValue("scoreMixLock",1);
            p->setParameterValue("tap3Note",67);p->setParameterValue("scoreRouting",1);expect(p->presetModified());
            juce::MemoryBlock state;p->getStateInformation(state);auto q=make();q->setParameterValue("scoreMix",.9);q->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
            expectEquals(q->getCurrentProgram(),2);expect(q->presetModified());
            for(const auto& spec:hpeq::standalone::scoreParameterSpecs())expectWithinAbsoluteError(q->parameterValue(spec.id),p->parameterValue(spec.id),1e-4,spec.id);
            q->setCurrentProgram(3);expectWithinAbsoluteError(q->parameterValue("scoreMix"),.23,1e-5);expectWithinAbsoluteError(q->parameterValue("scoreRouting"),1.,1e-5);expect(!q->presetModified());
        }
        beginTest("Score fixed notes and Reverse are block invariant after prepare(256)");
        {
            const auto input=hamlo::test::drums(.8);
            for(bool reverse:{false,true})
            {
                auto p=make();p->setParameterValue("tap1Note",48);p->setParameterValue("tap1Time",125);p->setParameterValue("tap1Shape",reverse?1:0);
                const auto expected=run(*p,input,256);
                for(int block:{1,64,127,512,4096})expect(hamlo::test::bitExact(run(*p,input,block),expected),"Block "+juce::String(block));
            }
        }
        beginTest("No C++ allocation in prepared audio, including mode/note/space transitions");
        {
            auto p=make();p->prepareToPlay(48000,256);juce::AudioBuffer<float> buffer(2,256);buffer.clear();juce::MidiBuffer midi;
            size_t allocated=0;
            for(int block=0;block<80;++block)
            {
                if(block%10==0){p->setParameterValue("echoMode",block%20==0?1:0);p->setParameterValue("tap1Note",block%20==0?48:60);}
                if(block==35)p->setParameterValue("scoreSpaceType",2);
                hamlo::test::allocation::begin();p->processBlock(buffer,midi);allocated+=hamlo::test::allocation::finish();
            }
            expectEquals(static_cast<int>(allocated),0);
        }
        beginTest("All-effects score and rapid structural/tempo/bypass automation are block invariant");
        {
            struct Playhead:juce::AudioPlayHead
            {double bpm=120;int64_t time=0;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(bpm);p.setTimeInSamples(time);p.setIsPlaying(true);return p;}};
            const auto signal=hamlo::test::drums(1.2);
            const auto render=[&](int block)
            {
                auto p=make();p->setParameterValue("tap1Time",125);p->setParameterValue("tap1Note",48);p->setParameterValue("scoreFeedback",.55);
                p->setParameterValue("scoreBands",1);p->setParameterValue("band1Time",.5);p->setParameterValue("band3Time",1.5);
                for(int b=1;b<=3;++b)p->setParameterValue("band"+juce::String(b)+"Feedback",.4);
                p->setParameterValue("scoreSpaceMix",.4);p->setParameterValue("scoreGlitch",3);p->setParameterValue("scoreGlitchAmount",.3);
                Playhead ph;p->setPlayHead(&ph);p->prepareToPlay(48000,256);auto out=signal;juce::MidiBuffer midi;
                const int positions[]{0,10001,10401,10901,21007,31009,41011,out.getNumSamples()};
                for(int e=0;e<7;++e)
                {
                    if(e==1)p->setParameterValue("tap1Note",55);
                    if(e==2)p->setParameterValue("scoreSpaceType",2);
                    if(e==3){p->setParameterValue("tap1Shape",1);ph.bpm=174;}
                    if(e==4){p->setParameterValue("scoreFeedbackPitch",-12);p->setParameterValue("scoreFreeze",1);}
                    if(e==5){p->setParameterValue("bypass",1);p->setParameterValue("scoreFreeze",0);}
                    if(e==6){p->setParameterValue("bypass",0);p->setParameterValue("echoMode",0);}
                    for(int start=positions[e];start<positions[e+1];start+=block)
                    {ph.time=start;juce::AudioBuffer<float> view(out.getArrayOfWritePointers(),2,start,juce::jmin(block,positions[e+1]-start));p->processBlock(view,midi);}
                }
                p->setPlayHead(nullptr);return out;
            };
            const auto expected=render(256);for(int block:{1,64,127,512,4096})expect(hamlo::test::bitExact(expected,render(block)),"Automation block "+juce::String(block));
        }
        beginTest("Host infinite Freeze tail and actual UI note editing functions");
        {
            auto p=make();p->setParameterValue("scoreFreeze",1);expect(std::isinf(p->getTailLengthSeconds()));
            hpeq::ui::ScorePanel panel(*p);juce::Slider* note=nullptr;
            std::function<void(juce::Component&)> find=[&](juce::Component& c){if(c.getComponentID()=="tap1Note")note=dynamic_cast<juce::Slider*>(&c);for(auto* child:c.getChildren())find(*child);};find(panel);
            expect(note!=nullptr);if(note){expectEquals(note->getTextFromValue(60),juce::String("C5"));expectWithinAbsoluteError(note->getValueFromText("G4"),55.,1e-9);}
        }
        beginTest("After Echo is outside feedback, and individual Space Send can remain dry");
        {
            auto p=make();p->setParameterValue("scoreSpaceDecay",2);p->setParameterValue("scoreSpaceDamping",6000); // Explicit reference settings, independent of the neutral init recipe.
            p->setParameterValue("scoreFeedback",.45);auto input=hamlo::test::drums(1.5);const auto echoes=run(*p,input,127);
            p->setParameterValue("scoreSpaceMix",.8);p->setParameterValue("scoreSpaceRoute",0);p->setParameterValue("tap1SpaceSend",0);
            p->setParameterValue("scoreFeedback",0);auto drySpace=run(*p,input,127);p->setParameterValue("scoreSpaceMix",0);auto dryReference=run(*p,input,127);expect(hamlo::test::bitExact(drySpace,dryReference));
            p->setParameterValue("tap1SpaceSend",1);p->setParameterValue("scoreFeedback",.45);p->setParameterValue("scoreSpaceMix",.8);const auto actual=run(*p,input,127);
            hamlo::echo::ScoreSettings s;s.spaceMix=.8;hamlo::echo::EchoSpace space;space.prepare(48000);space.configure(s);double error=0;
            for(int i=0;i<echoes.getNumSamples();++i){const auto tail=space.process({echoes.getSample(0,i),echoes.getSample(1,i)});for(int ch=0;ch<2;++ch)error=std::max(error,std::abs(actual.getSample(ch,i)-(echoes.getSample(ch,i)+.8*tail[static_cast<size_t>(ch)])));}
            expect(error<1e-6,"After Echo routing error "+juce::String(error));
        }
        beginTest("Reusing a prepared path with additional captured groups matches a fresh epoch");
        for(auto action:{hamlo::echo::Action::Reverse,hamlo::echo::Action::Stutter,hamlo::echo::Action::TapeStop})
        {
            auto reused=std::make_unique<hamlo::echo::ScoreEngine>(),fresh=std::make_unique<hamlo::echo::ScoreEngine>();
            hamlo::echo::ScoreSettings s;s.mix=1;s.feedback=0;s.spaceMix=0;s.taps[0].enabled=true;s.taps[0].note=60;s.taps[0].gainDb=-48;
            reused->prepare(48000,256);reused->start(s);juce::AudioBuffer<float> prime(2,14417);for(int ch=0;ch<2;++ch)for(int i=0;i<prime.getNumSamples();++i)prime.setSample(ch,i,ch==0?.37f:.29f);reused->process(prime.getArrayOfWritePointers(),2,prime.getNumSamples());
            s.taps[1].enabled=true;s.taps[1].note=48;s.taps[1].timeMs=150;s.taps[1].gainDb=0;s.taps[1].fragmentMs=100;s.taps[1].action=action;
            reused->start(s);fresh->prepare(48000,256);fresh->start(s);auto a=hamlo::test::drums(.8),b=a;
            reused->process(a.getArrayOfWritePointers(),2,a.getNumSamples());fresh->process(b.getArrayOfWritePointers(),2,b.getNumSamples());
            expect(hamlo::test::bitExact(a,b),"A previously inactive history must use the new epoch's source clock");
        }
        beginTest("Non-divisible stutter fragments have exactly the selected 2..8 subdivisions");
        for(int repeats=2;repeats<=8;++repeats)
        {
            auto p=make();p->setParameterValue("tap1Shape",2);p->setParameterValue("tap1Fragment",100.083333333);p->setParameterValue("tap1Repeats",repeats);
            juce::AudioBuffer<float> input(2,24000);for(int ch=0;ch<2;++ch)for(int i=0;i<input.getNumSamples();++i)input.setSample(ch,i,.1f+i*.00001f);
            const auto actual=run(*p,input,127);const int length=4804,delay=480;double error=0;
            for(int n=0;n<input.getNumSamples();++n)
            {double expected=0;const int elapsed=n-delay;if(elapsed>=length){const int age=elapsed%length,repeat=age*repeats/length,start=(repeat*length+repeats-1)/repeats;
                    const int index=n-age-delay-length+age-start;const double edge=std::clamp(std::min(age,length-1-age)/32.,0.,1.);if(index>=0)expected=edge*input.getSample(0,index);}
                error=std::max(error,std::abs(actual.getSample(0,n)-expected));}
            expect(error<=1e-6,"Stutter subdivision reference error "+juce::String(error,9));
        }
        beginTest("Old state loaded on a playing Score instance resets to fresh Legacy sound");
        {
            const auto file=juce::File(__FILE__).getParentDirectory().getParentDirectory().getChildFile("out/m0/fixtures/program_0_48000_ch2.state");
            juce::MemoryBlock old;
            if(file.loadFileAsData(old))
            {
                auto changed=make();const auto signal=hamlo::test::drums(.5);(void)run(*changed,signal,127);
                changed->setStateInformation(old.getData(),static_cast<int>(old.getSize()));
                auto fresh=std::make_unique<ModulePluginProcessor>(hpeq::standalone::ModuleKind::Echo);fresh->setStateInformation(old.getData(),static_cast<int>(old.getSize()));fresh->prepareToPlay(48000,256);
                auto a=signal,b=signal;juce::MidiBuffer midi;
                for(int start=0;start<a.getNumSamples();start+=127)
                {
                    const int count=juce::jmin(127,a.getNumSamples()-start);
                    juce::AudioBuffer<float> av(a.getArrayOfWritePointers(),2,start,count),bv(b.getArrayOfWritePointers(),2,start,count);
                    changed->processBlock(av,midi);fresh->processBlock(bv,midi);
                }
                expect(hamlo::test::bitExact(a,b));expectWithinAbsoluteError(changed->parameterValue("echoMode"),0.,1e-6);
            }
            else logMessage("SKIP: local M0 state archive unavailable");
        }
    }
};
ScoreIntegrationTests scoreIntegrationTests;
}
