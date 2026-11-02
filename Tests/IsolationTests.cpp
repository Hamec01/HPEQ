#include "Standalone/ModulePluginProcessor.h"
#include "AllocationAudit.h"
#include "TestSignals.h"
#include <memory>

namespace
{
using hpeq::standalone::ModulePluginProcessor;
constexpr auto kind=hpeq::standalone::ModuleKind::Zastava;
juce::AudioBuffer<float> render(int program,const juce::AudioBuffer<float>& input,int block)
{
    auto p=std::make_unique<ModulePluginProcessor>(kind);p->setCurrentProgram(program);p->prepareToPlay(48000,256);
    auto result=input;juce::MidiBuffer midi;
    for(int at=0;at<result.getNumSamples();at+=block)
    {juce::AudioBuffer<float> part(result.getArrayOfWritePointers(),2,at,juce::jmin(block,result.getNumSamples()-at));p->processBlock(part,midi);}
    return result;
}
class IsolationTests final:public juce::UnitTest
{
public:
    IsolationTests():juce::UnitTest("Zastava isolation","HamloZastava"){}
    void runTest() override
    {
        beginTest("Every original static program is bit-identical across blocks after prepare256");
        auto p=std::make_unique<ModulePluginProcessor>(kind);const auto input=hamlo::test::drums(.35);
        for(int program=0;program<p->getNumPrograms();++program)
        {
            const auto expected=render(program,input,256);
            for(int block:{1,64,127,512,4096})expect(hamlo::test::bitExact(expected,render(program,input,block)),p->getProgramName(program)+" block"+juce::String(block));
        }
        beginTest("First processing and branch/filter/bypass/lookahead automation allocate zero prepared memory");
        for(int program=0;program<p->getNumPrograms();++program)
        {
            p->setCurrentProgram(program);p->prepareToPlay(48000,256);juce::MidiBuffer midi;juce::AudioBuffer<float> data(2,4096);
            size_t count=0;bool finite=true;const int blocks[]{1,64,127,512,4096};
            for(int event=0;event<60;++event)
            {
                if(event>0)
                {
                    p->setParameterValue("style",event%3);p->setParameterValue("lookahead",event%3);p->setParameterValue("bypass",event%4==0?1:0);p->setParameterValue("mix",event%3*.5);
                    p->setParameterValue("keyHpHz",event%2?300:20);p->setParameterValue("keyLpHz",event%2?3000:20000);
                    p->setParameterValue("keyListen",event%2);p->setParameterValue("flip",event%2);p->setParameterValue("outputDb",event%2?-12:0);
                    p->setParameterValue("thresholdDb",event%2?-90:-20);
                }
                const int samples=blocks[event%5];
                for(int i=0;i<samples;++i){data.setSample(0,i,static_cast<float>(.1*std::sin(.07*i)));data.setSample(1,i,static_cast<float>(.13*std::sin(.09*i)));}
                juce::AudioBuffer<float> view(data.getArrayOfWritePointers(),2,0,samples);
                hamlo::test::allocation::begin();p->processBlock(view,midi);count+=hamlo::test::allocation::finish();
                for(int ch=0;ch<2;++ch)for(int i=0;i<samples;++i)finite=finite&&std::isfinite(data.getSample(ch,i));
            }
            expectEquals(static_cast<int>(count),0,p->getProgramName(program));expect(finite);
            p->setParameterValue("bypass",0);p->setParameterValue("keyListen",0);p->setParameterValue("lookahead",1);p->setParameterValue("style",0);p->processBlock(data,midi);
            const auto meters=p->moduleNode().getGateMeters(0);expect(std::isfinite(meters.attenuationDb)&&std::isfinite(meters.keyDb),"Original gate panel meters remain available");
        }
    }
};
IsolationTests tests;
}
