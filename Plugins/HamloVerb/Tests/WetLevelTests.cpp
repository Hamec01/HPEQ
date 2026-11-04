#include "VerbLabHelpers.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>

namespace hamlo::test
{
using namespace hamlo::verblab;
class WetLevelTests final : public juce::UnitTest
{
public:
    WetLevelTests() : juce::UnitTest("Wet return level", "HamloVerb") {}
    void runTest() override
    {
        beginTest("Measured return on six neutral characters, unchanged laboratory");
        auto x=excitation(8);x.clear(4*48000,4*48000);
        for(int character=1;character<=6;++character)
        {
            auto p=processor(neutral(character));settle(*p);const auto y=render(*p,x);
            logMessage("NEUTRAL "+juce::String(character)+" wet_dry_db="+juce::String(10*std::log10(energy(y)/energy(x)),6));
            expect(finite(y));
        }
        beginTest("Screenshot preset on owner keys2, Send with/without Reverse, full render");
        const juce::File file("C:/Users/Ham_h/AppData/Local/Temp/claude/C--DRUMENGINE/ec2c5161-f2c8-4110-8e0e-02c63cfee6b7/scratchpad/mat/wav/keys2.wav");
        if(file.existsAsFile())
        {
            juce::AudioFormatManager formats;formats.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
            expect(reader!=nullptr);
            if(reader)
            {
                const int count=std::min(static_cast<int>(reader->lengthInSamples),static_cast<int>(8*reader->sampleRate));
                juce::AudioBuffer<float> raw(2,count+8);raw.clear();reader->read(&raw,0,count,0,true,true);
                if(reader->numChannels==1)raw.copyFrom(1,0,raw,0,0,count);
                const int length=static_cast<int>(count*48000./reader->sampleRate);
                juce::AudioBuffer<float> material(2,length+8*48000);material.clear();
                for(int c=0;c<2;++c){juce::LagrangeInterpolator resample;resample.process(reader->sampleRate/48000.,raw.getReadPointer(c),material.getWritePointer(c),length);}
                const double rms=std::sqrt(energy(material)/(2.*length));
                material.applyGain(static_cast<float>(std::min(std::pow(10.,-18./20)/rms,std::pow(10.,-3./20)/material.getMagnitude(0,length))));
                int program=-1;auto p=processor({});
                for(int i=0;i<p->getNumPrograms();++i)if(p->getProgramName(i)=="TRAP Memphis Reverse Horizon")program=i;
                expect(program>=0);
                logMessage("MATERIAL sha256="+juce::SHA256(file).toHexString()+" program="+juce::String(program));
                for(int shape:{0,3})
                {
                    p=processor({});p->setCurrentProgram(program);p->setParameterValue("routing",1);p->setParameterValue("timeShape",shape);
                    p->prepareToPlay(48000,256);settle(*p);const auto y=render(*p,material);
                    logMessage("SCREENSHOT shape="+juce::String(shape)+" wet_dry_db="+juce::String(10*std::log10(energy(y)/energy(material)),6));
                    expect(finite(y));
                }
            }
        }
        else logMessage("OWNER_MATERIAL=n/a (private source unavailable)");
        auto probe=processor({});
        if(!probe->parameter("wetDb")){logMessage("WET_LEVEL=n/a (baseline)");return;}
        beginTest("Send return trim measures selected gain, all six characters and Reverse");
        const auto source=excitation(.6);
        for(int character=1;character<=6;++character)for(int shape:{0,3})
        {
            auto s=neutral(character);set(s,"timeShape",shape);set(s,"shapeTimeMs",50);
            auto p=processor(s);settle(*p);const auto unity=render(*p,source);
            for(double gain:{-24.,6.,12.,24.})
            {
                set(s,"wetDb",gain);auto q=processor(s);settle(*q);const auto y=render(*q,source);
                expect(finite(y));expectWithinAbsoluteError(10*std::log10(energy(y)/energy(unity)),gain,.01);
            }
        }
        beginTest("Dry unchanged; boosted Insert equals dry+Mix*Send; Legacy ignores trim");
        for(int character=1;character<=6;++character)
        {
            auto s=neutral(character,2,500);set(s,"routing",0);set(s,"mix",.7);set(s,"keepDry",1);set(s,"wetDb",24);
            auto p=processor(s);settle(*p);expect(exact(render(*p,excitation(.08)),excitation(.08)));
            s=neutral(character);set(s,"wetDb",12);auto send=processor(s);settle(*send);const auto wet=render(*send,source);
            set(s,"routing",0);set(s,"keepDry",1);set(s,"mix",.6);auto insert=processor(s);settle(*insert);const auto y=render(*insert,source);
            bool same=true;for(int c=0;c<2;++c)for(int i=0;i<source.getNumSamples();++i)
                same=same&&std::abs(y.getSample(c,i)-(source.getSample(c,i)+.6f*wet.getSample(c,i)))<2e-6f;
            expect(same);set(s,"mix",0);p=processor(s);settle(*p);expect(exact(render(*p,source),source));
        }
        {
            auto s=neutral(0);auto a=processor(s);set(s,"wetDb",24);auto b=processor(s);settle(*a);settle(*b);expect(exact(render(*a,source),render(*b,source)));
        }
        beginTest("Trim state migration, recall, factory reset and modified marker");
        {
            auto p=processor({{"wetDb",12}});juce::MemoryBlock state;p->getStateInformation(state);
            auto xml=Processor::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));expect(xml!=nullptr);if(!xml)return;
            if(auto* child=xml->getChildByAttribute("id","wetDb"))xml->removeChildElement(child,true);
            juce::MemoryBlock old;Processor::copyXmlToBinary(*xml,old);p->setStateInformation(old.getData(),static_cast<int>(old.getSize()));expectWithinAbsoluteError(p->parameterValue("wetDb"),0.,0.);
            p->setStateInformation(state.getData(),static_cast<int>(state.getSize()));expectWithinAbsoluteError(p->parameterValue("wetDb"),12.,1e-5);
            p->setCurrentProgram(105);expectWithinAbsoluteError(p->parameterValue("wetDb"),0.,0.);
        }
        beginTest("Trim sample automation and zero latency, mono/stereo, all block sizes");
        for(int channels:{1,2})
        {
            const auto input=excitation(.8,48000,channels);const auto s=neutral(6);
            const std::vector<Event> events{{4093,"wetDb",24},{9997,"wetDb",-24},{15001,"wetDb",0},{21011,"wetDb",12}};
            auto p=processor(s,48000,channels);settle(*p);const auto expected=render(*p,input,256,events);
            for(int block:{1,64,127,512,4096})
            {auto q=processor(s,48000,channels);settle(*q);expect(exact(render(*q,input,block,events),expected));expectEquals(q->getLatencySamples(),0);}
        }
    }
};static WetLevelTests wetLevelTests;
}
