#include "VerbLabHelpers.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_cryptography/juce_cryptography.h>

namespace hamlo::test
{
using namespace hamlo::verblab;
class WetLevelWorkflowTests final : public juce::UnitTest
{
public:
    WetLevelWorkflowTests():juce::UnitTest("Wet level workflow", "HamloVerb"){}
    void runTest() override
    {
        const auto input=excitation(.8);
        beginTest("Append-only parameter, modified marker and factory reset");
        auto p=processor({});auto& params=p->getParameters();
        auto* last=dynamic_cast<juce::AudioProcessorParameterWithID*>(params.getLast());
        expect(last && last->paramID=="wetDb");
        p->setCurrentProgram(104);expect(!p->isProgramModified());p->setParameterValue("wetDb",12);expect(p->isProgramModified());
        p->setCurrentProgram(104);expect(!p->isProgramModified());expectWithinAbsoluteError(p->parameterValue("wetDb"),0.,0.);
        beginTest("Ordinary Insert keeps its original dry coefficient; Output trims total; bypass stays dry");
        for(int character=1;character<=6;++character)
        {
            auto s=neutral(character);set(s,"wetDb",12);p=processor(s);settle(*p);const auto wet=render(*p,input);
            set(s,"routing",0);set(s,"mix",.4);set(s,"outputDb",-6);auto q=processor(s);settle(*q);const auto y=render(*q,input);
            bool same=true;const float gain=static_cast<float>(std::pow(10.,-6./20));
            for(int c=0;c<2;++c)for(int i=0;i<y.getNumSamples();++i)
                same=same&&std::abs(y.getSample(c,i)-gain*(.6f*input.getSample(c,i)+.4f*wet.getSample(c,i)))<2e-6;
            expect(same);set(s,"bypass",1);q=processor(s);settle(*q);expect(exact(render(*q,input),input));
        }
        beginTest("Freeze tank and its timing unchanged; wet meter includes return trim");
        for(int character=1;character<=6;++character)
        {
            auto s=neutral(character);auto a=processor(s);set(s,"wetDb",12);auto b=processor(s);settle(*a);settle(*b);
            const std::vector<Event> events{{17003,"freeze",1}};
            const auto zero=render(*a,input,256,events),boost=render(*b,input,256,events);
            expectWithinAbsoluteError(10*std::log10(energy(boost)/energy(zero)),12.,.001);
            expectWithinAbsoluteError(static_cast<double>(b->getOutputDb()-a->getOutputDb()),12.,.001);
            expect(std::isinf(a->getTailLengthSeconds())&&std::isinf(b->getTailLengthSeconds()));
        }
        beginTest("Optional private owner-material audition, fixed common gain");
        const auto destination=juce::SystemStats::getEnvironmentVariable("HAMLO_WET_DEMO_DIR",{});
        if(destination.isEmpty()){logMessage("PRIVATE_AUDITION=not requested");return;}
        const juce::File folder(destination);
        expect(!folder.exists());if(folder.exists())return;expect(folder.createDirectory().wasOk());
        const juce::File file("C:/Users/Ham_h/AppData/Local/Temp/claude/C--DRUMENGINE/ec2c5161-f2c8-4110-8e0e-02c63cfee6b7/scratchpad/mat/wav/keys2.wav");
        juce::AudioFormatManager formats;formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));expect(reader!=nullptr);if(!reader)return;
        const int count=std::min(static_cast<int>(reader->lengthInSamples),static_cast<int>(8*reader->sampleRate));
        juce::AudioBuffer<float> raw(2,count+8);raw.clear();reader->read(&raw,0,count,0,true,true);
        if(reader->numChannels==1)raw.copyFrom(1,0,raw,0,0,count);
        const int length=static_cast<int>(count*48000./reader->sampleRate);
        juce::AudioBuffer<float> material(2,length+8*48000);material.clear();
        for(int c=0;c<2;++c){juce::LagrangeInterpolator resample;resample.process(reader->sampleRate/48000.,raw.getReadPointer(c),material.getWritePointer(c),length);}
        material.applyGain(static_cast<float>(std::min(std::pow(10.,-18./20)/std::sqrt(energy(material)/(2.*length)),std::pow(10.,-3./20)/material.getMagnitude(0,length))));
        for(int route:{0,1})
        {
            std::array<juce::AudioBuffer<float>,2> outputs;
            for(int i=0;i<2;++i)
            {
                p=processor({});p->setCurrentProgram(104);p->setParameterValue("routing",route);p->setParameterValue("keepDry",1);p->setParameterValue("wetDb",12*i);
                p->prepareToPlay(48000,256);settle(*p);outputs[static_cast<size_t>(i)]=render(*p,material);
            }
            const int n=material.getNumSamples(),gap=19200;juce::AudioBuffer<float> pair(2,2*n+gap);pair.clear();
            for(int c=0;c<2;++c){pair.copyFrom(c,0,outputs[0],c,0,n);pair.copyFrom(c,n+gap,outputs[1],c,0,n);}
            const float common=std::min(1.0f,.5f/pair.getMagnitude(0,pair.getNumSamples()));pair.applyGain(common);
            const auto name=route==0?"Insert_KEEP_DRY_0dB_then_12dB.wav":"Send_WET_0dB_then_12dB.wav";
            auto stream=folder.getChildFile(name).createOutputStream();juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.get(),48000,2,24,{},0));
            expect(writer!=nullptr);if(writer){stream.release();expect(writer->writeFromAudioSampleBuffer(pair,0,pair.getNumSamples()));}
            logMessage("AUDITION "+juce::String(name)+" common_gain="+juce::String(common,9)+" source_sha="+juce::SHA256(file).toHexString());
        }
    }
};static WetLevelWorkflowTests wetLevelWorkflowTests;
}
