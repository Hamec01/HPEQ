#include "Standalone/ModulePluginProcessor.h"
#include "AllocationAudit.h"
#include "TestSignals.h"
#include <memory>

namespace
{
using hpeq::standalone::ModulePluginProcessor;
constexpr auto kind=hpeq::standalone::ModuleKind::Shirota;
juce::AudioBuffer<float> render(int program,const juce::AudioBuffer<float>& input,int block)
{
    auto p=std::make_unique<ModulePluginProcessor>(kind);p->setCurrentProgram(program);p->prepareToPlay(48000,256);
    auto out=input;juce::MidiBuffer midi;
    for(int pos=0;pos<out.getNumSamples();pos+=block)
    {juce::AudioBuffer<float> view(out.getArrayOfWritePointers(),2,pos,juce::jmin(block,out.getNumSamples()-pos));p->processBlock(view,midi);}
    return out;
}
class IsolationTests final:public juce::UnitTest
{
public:
    IsolationTests():juce::UnitTest("Shirota isolation","HamloShirota"){}
    void runTest() override
    {
        beginTest("Every original static program is bit-identical across host blocks after prepare256");
        auto p=std::make_unique<ModulePluginProcessor>(kind);const auto input=hamlo::test::drums(.35);
        for(int program=0;program<p->getNumPrograms();++program)
        {
            const auto expected=render(program,input,256);
            for(int block:{1,64,127,512,4096})expect(hamlo::test::bitExact(expected,render(program,input,block)),p->getProgramName(program)+" block"+juce::String(block));
        }
        beginTest("Prepared audio allocates zero, including first call and rapid insert/alignment changes");
        for(int program=0;program<p->getNumPrograms();++program)
        {
            p->setCurrentProgram(program);p->prepareToPlay(48000,256);juce::MidiBuffer midi;
            juce::AudioBuffer<float> data(2,4096);size_t count=0;bool finite=true;
            const int blocks[]{1,64,127,512,4096};
            for(int event=0;event<60;++event)
            {
                if(event>0)
                {
                    p->setParameterValue("style",event%3);p->setParameterValue("safe",event%2);
                    p->setParameterValue("bypass",event%4==0?1:0);p->setParameterValue("mix",event%3*.5);
                    p->setParameterValue("monoBassHz",event%2?500:10);p->setParameterValue("stereoize",event%2);
                    p->setParameterValue("space",event%2);p->setParameterValue("width",event%2?2:0);
                }
                const int samples=blocks[event%5];
                for(int i=0;i<samples;++i){data.setSample(0,i,static_cast<float>(.1*std::sin(.07*i)));data.setSample(1,i,static_cast<float>(.13*std::sin(.09*i)));}
                juce::AudioBuffer<float> view(data.getArrayOfWritePointers(),2,0,samples);
                hamlo::test::allocation::begin();p->processBlock(view,midi);count+=hamlo::test::allocation::finish();
                for(int ch=0;ch<2;++ch)for(int i=0;i<samples;++i)finite=finite&&std::isfinite(data.getSample(ch,i));
            }
            expectEquals(static_cast<int>(count),0,p->getProgramName(program));expect(finite);
            p->setParameterValue("bypass",0);p->setParameterValue("safe",1);p->setParameterValue("stereoize",.5);p->processBlock(data,midi);
            const auto meters=p->moduleNode().getStereoMeters(0);std::array<float,512> left{},right{};
            const bool scope=p->moduleNode().getStereoScope(0,left,right);
            expect(scope&&std::isfinite(meters.correlation)&&std::isfinite(meters.guardDb),"Legacy panel telemetry remains available");
        }
    }
};
IsolationTests isolationTests;
}
