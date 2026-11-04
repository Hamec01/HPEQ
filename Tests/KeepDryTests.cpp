#include "VerbLabHelpers.h"
#include <juce_cryptography/juce_cryptography.h>

namespace hamlo::test
{
using namespace hamlo::verblab;
class KeepDryTests final : public juce::UnitTest
{
public:
    KeepDryTests() : juce::UnitTest("Keep dry", "HamloVerb") {}
    void runTest() override
    {
        auto probe=processor({});const bool available=probe->parameter("keepDry")!=nullptr;
        beginTest("Frozen old-program PCM: all208 programs, default mix law");
        const auto source=excitation(.5);
        for(int program=0;program<208;++program)
        {
            auto p=processor({});p->setCurrentProgram(program);p->prepareToPlay(48000,256);settle(*p);
            const auto y=render(*p,source);juce::MemoryBlock pcm;
            for(int c=0;c<2;++c)pcm.append(y.getReadPointer(c),static_cast<size_t>(y.getNumSamples())*sizeof(float));
            logMessage("PROGRAM "+juce::String(program)+" "+juce::SHA256(pcm).toHexString());
            expect(finite(y));
        }
        if(!available){logMessage("KEEP_DRY=n/a (baseline)");return;}
        beginTest("KEEP DRY preserves source before predelay for every Mix and modern character");
        const auto input=excitation(.08);
        for(int engine=1;engine<=6;++engine)for(double mix:{0.,.25,.5,1.})
        {
            auto s=neutral(engine,2,500);set(s,"routing",0);set(s,"mix",mix);set(s,"keepDry",1);
            auto p=processor(s);settle(*p);expect(exact(render(*p,input),input));
        }
        beginTest("KEEP DRY equals dry+Mix*Send at unity Output, including the full tail");
        {
            auto s=neutral(1);set(s,"earlyDb",0);set(s,"mix",.5);auto send=processor(s);settle(*send);
            const auto wet=render(*send,source);set(s,"routing",0);set(s,"keepDry",1);auto add=processor(s);settle(*add);
            const auto mixed=render(*add,source);bool same=true;
            for(int c=0;c<2;++c)for(int i=0;i<source.getNumSamples();++i)
                same=same&&std::abs(mixed.getSample(c,i)-(source.getSample(c,i)+.5f*wet.getSample(c,i)))<2e-6f;
            expect(same);
        }
        beginTest("Send ignores KEEP DRY; old states reset it; new states retain it; presets keep workflow choice");
        {
            auto s=neutral(2);auto a=processor(s);set(s,"keepDry",1);auto b=processor(s);settle(*a);settle(*b);
            expect(exact(render(*a,source),render(*b,source)));
            juce::MemoryBlock state;b->getStateInformation(state);
            auto xml=Processor::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));expect(xml!=nullptr);if(!xml)return;
            if(auto* child=xml->getChildByAttribute("id","keepDry"))xml->removeChildElement(child,true);
            juce::MemoryBlock old;Processor::copyXmlToBinary(*xml,old);
            b->setStateInformation(old.getData(),static_cast<int>(old.getSize()));expectEquals(juce::roundToInt(b->parameterValue("keepDry")),0);
            b->setStateInformation(state.getData(),static_cast<int>(state.getSize()));expectEquals(juce::roundToInt(b->parameterValue("keepDry")),1);
            b->setCurrentProgram(10);expectEquals(juce::roundToInt(b->parameterValue("keepDry")),1);
        }
        beginTest("KEEP DRY automation is sample-exact across blocks, mono/stereo; no new latency");
        for(int channels:{1,2})
        {
            const auto x=excitation(.6,48000,channels);
            auto s=neutral(3);set(s,"routing",0);set(s,"mix",.6);
            const std::vector<Event> events{{4093,"keepDry",1},{9997,"keepDry",0},{15001,"keepDry",1},{21011,"mix",.2}};
            auto p=processor(s,48000,channels);settle(*p);const auto expected=render(*p,x,256,events);
            for(int block:{1,64,127,512,4096})
            {auto q=processor(s,48000,channels);settle(*q);expect(exact(render(*q,x,block,events),expected));expectEquals(q->getLatencySamples(),0);}
        }
    }
};static KeepDryTests keepDryTests;
}
