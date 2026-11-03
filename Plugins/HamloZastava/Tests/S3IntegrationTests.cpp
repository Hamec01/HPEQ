#include "Standalone/ModulePluginProcessor.h"
#include "AllocationAudit.h"
#include "TestSignals.h"

namespace
{
using P=hpeq::standalone::ModulePluginProcessor;
constexpr auto kind=hpeq::standalone::ModuleKind::Zastava;
class IntegrationTests final:public juce::UnitTest
{
public:
    IntegrationTests():juce::UnitTest("S3 integration","HamloZastava"){}
    void runTest() override
    {
        beginTest("External key may share the main input memory");
        // First native suite used disjoint bus memory. These four alias cases
        // expose key reads after the parallel Legacy path has overwritten main.
        for(int channels:{1,2})for(int keys:{1,2})
        {
            auto processor=std::make_unique<P>(kind);auto layout=processor->getBusesLayout();
            layout.inputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));layout.outputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(channels));layout.inputBuses.set(1,juce::AudioChannelSet::canonicalChannelSet(keys));
            expect(processor->setBusesLayout(layout));processor->setParameterValue("v2_enabled",1);processor->setParameterValue("v2_external",1);processor->setParameterValue("v2_detectorMs",0);processor->setParameterValue("v2_lookahead",0);processor->setParameterValue("v2_threshold",-30);processor->setParameterValue("thresholdDb",0);
            processor->prepareToPlay(48000,256);auto source=hamlo::test::drums(.15);juce::AudioBuffer<float> actual(channels,source.getNumSamples()),expected(channels,source.getNumSamples());
            hamlo::zastava::Gate gate;gate.prepare(48000,256);hamlo::zastava::Settings settings;settings.enabled=true;settings.external=true;settings.detectorMs=0;settings.lookaheadMs=0;settings.thresholdDb=-30;gate.set(settings);
            juce::MidiBuffer midi;size_t allocations=0;
            for(int at=0;at<source.getNumSamples();at+=127)
            {
                const int count=std::min(127,source.getNumSamples()-at);std::array<std::array<float,127>,2> main{},ref{},key{};
                for(int c=0;c<channels;++c)for(int n=0;n<count;++n)main[static_cast<size_t>(c)][static_cast<size_t>(n)]=ref[static_cast<size_t>(c)][static_cast<size_t>(n)]=source.getSample(c,at+n);
                for(int c=0;c<keys;++c)key[static_cast<size_t>(c)]=main[static_cast<size_t>(std::min(c,channels-1))];
                float* pointers[]{main[0].data(),channels==2?main[1].data():main[0].data(),main[0].data(),main[1].data()};
                if(channels==1){pointers[1]=main[0].data();pointers[2]=main[0].data();}
                else {pointers[2]=main[0].data();pointers[3]=main[1].data();}
                float* refPointers[]{ref[0].data(),ref[1].data()};const float* keyPointers[]{key[0].data(),key[1].data()};
                gate.process(refPointers,channels,count,keyPointers,keys);juce::AudioBuffer<float> buffer(pointers,channels+keys,count);
                hamlo::test::allocation::begin();processor->processBlock(buffer,midi);allocations+=hamlo::test::allocation::finish();
                for(int c=0;c<channels;++c){actual.copyFrom(c,at,main[static_cast<size_t>(c)].data(),count);expected.copyFrom(c,at,ref[static_cast<size_t>(c)].data(),count);}
            }
            expect(hamlo::test::bitExact(actual,expected),"main"+juce::String(channels)+" key"+juce::String(keys));expectEquals(static_cast<int>(allocations),0);
        }
        beginTest("Host direct bypass preserves fixed PDC and allocates nothing");
        for(int mode=0;mode<=4;++mode)
        {
            auto p=std::make_unique<P>(kind);p->setCurrentProgram(0);
            p->setParameterValue("v2_engine",mode);p->setParameterValue("v2_enabled",1);p->setParameterValue("v2_threshold",0);
            p->setParameterValue("thresholdDb",0);p->prepareToPlay(48000,256);
            auto original=hamlo::test::drums(.2);auto got=original;juce::MidiBuffer midi;size_t allocations=0;
            for(int at=0;at<got.getNumSamples();at+=127)
            {juce::AudioBuffer<float> view(got.getArrayOfWritePointers(),2,at,std::min(127,got.getNumSamples()-at));hamlo::test::allocation::begin();p->processBlockBypassed(view,midi);allocations+=hamlo::test::allocation::finish();}
            expectEquals(static_cast<int>(allocations),0);
            bool exact=true;for(int c=0;c<2;++c)for(int n=2000;n<got.getNumSamples();++n)exact=exact&&got.getSample(c,n)==original.getSample(c,n-240);
            expect(exact,"mode"+juce::String(mode));expectEquals(p->getLatencySamples(),240);
        }
        beginTest("Every appended parameter retains its quantized state");
        auto p=std::make_unique<P>(kind);
        for(int i=16;i<p->getParameters().size();++i)
        {auto* q=dynamic_cast<juce::RangedAudioParameter*>(p->getParameters()[i]);const auto& r=q->getNormalisableRange();p->setParameterValue(q->paramID,r.start+.371f*(r.end-r.start));}
        p->setParameterValue("v2_engine",4);juce::MemoryBlock b;p->getStateInformation(b);auto q=std::make_unique<P>(kind);q->setStateInformation(b.getData(),static_cast<int>(b.getSize()));
        for(int i=16;i<p->getParameters().size();++i)expectEquals(q->getParameters()[i]->getValue(),p->getParameters()[i]->getValue(),p->getParameters()[i]->getName(100));
        beginTest("Full modern serialized bytes remain stable");
        juce::MemoryBlock restored;q->getStateInformation(restored);expect(restored==b);
        if(restored!=b)
        {
            const auto folder=juce::File::getCurrentWorkingDirectory().getChildFile("out/s3/diagnostic_state");folder.createDirectory();
            juce::AudioProcessor::getXmlFromBinary(b.getData(),static_cast<int>(b.getSize()))->writeTo(folder.getChildFile("before.xml"));
            juce::AudioProcessor::getXmlFromBinary(restored.getData(),static_cast<int>(restored.getSize()))->writeTo(folder.getChildFile("after.xml"));
        }
        beginTest("New nonfinite, out-of-range and wrong-root states are contained");
        auto xml=juce::AudioProcessor::getXmlFromBinary(b.getData(),static_cast<int>(b.getSize()));
        for(auto* child:xml->getChildIterator())
        {const auto id=child->getStringAttribute("id");if(id=="v2_tail")child->setAttribute("value","nan");if(id=="v2_mix")child->setAttribute("value",9);}
        juce::MemoryBlock bad;juce::AudioProcessor::copyXmlToBinary(*xml,bad);q->setStateInformation(bad.getData(),static_cast<int>(bad.getSize()));
        expectEquals(q->parameterValue("v2_mix"),1.);expectEquals(q->parameterValue("v2_tail"),100.);
        juce::MemoryBlock valid;q->getStateInformation(valid);juce::XmlElement wrong("OtherProduct");juce::AudioProcessor::copyXmlToBinary(wrong,bad);q->setStateInformation(bad.getData(),static_cast<int>(bad.getSize()));juce::MemoryBlock after;q->getStateInformation(after);expect(after==valid);
    }
};
IntegrationTests tests;
}
