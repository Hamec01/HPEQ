#include "Standalone/ModulePluginProcessor.h"
#include "Standalone/V2Parameters.h"
#include "Standalone/GenrePresets.h"
#include "AllocationAudit.h"
#include "TestSignals.h"
#include "ShirotaHrtf.h"
#include <memory>
namespace
{
using hpeq::standalone::ModulePluginProcessor;
std::unique_ptr<ModulePluginProcessor> instance(int mode=1)
{auto p=std::make_unique<ModulePluginProcessor>(hpeq::standalone::ModuleKind::Shirota);p->setParameterValue("v2Mode",mode);return p;}
juce::AudioBuffer<float> render(ModulePluginProcessor& p,const juce::AudioBuffer<float>& input)
{p.prepareToPlay(48000,256);auto out=input;juce::MidiBuffer midi;for(int pos=0;pos<out.getNumSamples();pos+=127){juce::AudioBuffer<float> part(out.getArrayOfWritePointers(),2,pos,juce::jmin(127,out.getNumSamples()-pos));p.processBlock(part,midi);}return out;}
class IntegrationTests final:public juce::UnitTest
{
public:
    IntegrationTests():juce::UnitTest("Production spatial integration","HamloShirota"){}
    void runTest() override
    {
        beginTest("New parameters append after the original bypass; neutral and bypass use unchanged PDC");
        auto p=instance();expectEquals(dynamic_cast<juce::AudioProcessorParameterWithID*>(p->getParameters()[11])->paramID,juce::String("bypass"));
        expectEquals(p->getParameters().size(),12+static_cast<int>(hpeq::standalone::v2Parameters().size()));
        const auto input=hamlo::test::drums(.4);
        for(int mode:{1,2})for(bool bypass:{false,true})
        {
            auto q=instance(mode);q->setParameterValue("bypass",bypass?1:0);const auto y=render(*q,input);bool dry=true;const int d=q->getLatencySamples();
            for(int ch=0;ch<2;++ch)for(int i=0;i<y.getNumSamples();++i)dry=dry&&y.getSample(ch,i)==(i<d?0:input.getSample(ch,i-d));expect(dry);expectEquals(d,120);
        }
        beginTest("Generated Side is governed by Width0 and output remains mono with position/space off");
        for(int mode:{1,2}){auto empty=instance(mode);empty->prepareToPlay(48000,256);juce::AudioBuffer<float> noChannels(0,256),noFrames(2,0);juce::MidiBuffer midi;hamlo::test::allocation::begin();empty->processBlock(noChannels,midi);empty->processBlock(noFrames,midi);const auto allocations=hamlo::test::allocation::finish();expectEquals(static_cast<int>(allocations),0,"Empty I/O callbacks are safe and allocate nothing");}
        juce::AudioBuffer<float> tones(2,19200);for(int i=0;i<19200;++i){tones.setSample(0,i,static_cast<float>(.1*std::sin(997*juce::MathConstants<double>::twoPi*i/48000)));tones.setSample(1,i,static_cast<float>(.1*std::sin(1319*juce::MathConstants<double>::twoPi*i/48000)));}
        for(double width:{2.,4.}){auto w=instance();w->setParameterValue("v2Width",width);const auto y=render(*w,tones);double ym=0,ys=0,xm=0,xs=0;for(int i=9600;i<19200;++i){const double a=y.getSample(0,i),b=y.getSample(1,i),c=tones.getSample(0,i-120),d=tones.getSample(1,i-120);ym+=(a+b)*(a+b);ys+=(a-b)*(a-b);xm+=(c+d)*(c+d);xs+=(c-d)*(c-d);}expectWithinAbsoluteError(10*std::log10(ys/ym*xm/xs),20*std::log10(width),.02);}
        juce::AudioBuffer<float> bassTone(2,19200);for(int i=0;i<19200;++i){const float v=static_cast<float>(.1*std::sin(60*juce::MathConstants<double>::twoPi*i/48000));bassTone.setSample(0,i,v);bassTone.setSample(1,i,v);}
        for(int generator=0;generator<3;++generator){double energies[3]{};for(int setting=0;setting<3;++setting){auto b=instance();b->setParameterValue("v2Generate",.75);b->setParameterValue("v2Generator",generator);if(setting==1)b->setParameterValue("v2MonoBass",150);if(setting==2)b->setParameterValue("v2LowWidth",0);const auto y=render(*b,bassTone);for(int i=9600;i<19200;++i)energies[setting]+=std::pow(y.getSample(0,i)-y.getSample(1,i),2);}expect(10*std::log10(energies[1]/energies[0])<=-30);expect(10*std::log10(energies[2]/energies[0])<=-12);}
        for(int mode:{1,2}){auto bypassMix=instance(mode);bypassMix->setParameterValue("v2PositionOn",1);bypassMix->setParameterValue("v2Space",1);bypassMix->setParameterValue("v2Generate",.75);bypassMix->setParameterValue("v2Mix",0);const auto y=render(*bypassMix,input);bool exact=true;for(int ch=0;ch<2;++ch)for(int i=0;i<y.getNumSamples();++i)exact=exact&&y.getSample(ch,i)==(i<120?0:input.getSample(ch,i-120));expect(exact,"Mix0 is aligned dry with the full effect enabled");}
        auto mono=input;mono.copyFrom(1,0,mono,0,0,mono.getNumSamples());
        for(int generator=0;generator<3;++generator){auto q=instance();q->setParameterValue("v2Generate",.75);q->setParameterValue("v2Generator",generator);q->setParameterValue("v2Width",0);const auto y=render(*q,mono);expect(std::memcmp(y.getReadPointer(0),y.getReadPointer(1),static_cast<size_t>(y.getNumSamples())*4)==0);}
        beginTest("Modern state restores every control/node/program/view and modified flag; old states clear modern edits");
        p=instance();p->setCurrentProgram(3);p->setParameterValue("v2Mode",2);p->setParameterValue("v2PositionOn",1);p->setParameterValue("v2Page",3);p->setParameterValue("v2EditNode",16);
        for(int i=0;i<16;++i){const auto key="v2Node"+juce::String(i);p->setParameterValue(key+"Angle",-170+i*20);p->setParameterValue(key+"Hold",123+i);p->setParameterValue(key+"Travel",456+i);p->setParameterValue(key+"HoldBeat",.125+i*.1);p->setParameterValue(key+"TravelBeat",.25+i*.1);p->setParameterValue(key+"Arc",i%3);p->setParameterValue(key+"Curve",i%2);}
        juce::MemoryBlock state;p->getStateInformation(state);auto q=instance();q->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        for(const auto& spec:hpeq::standalone::v2Parameters())expectWithinAbsoluteError(q->parameterValue(spec.id),p->parameterValue(spec.id),1e-4,spec.id);
        expectEquals(q->getCurrentProgram(),3);expect(q->presetModified());
        auto old=instance(0);old->setCurrentProgram(2);juce::MemoryBlock oldState;old->getStateInformation(oldState);q->setStateInformation(oldState.getData(),static_cast<int>(oldState.getSize()));
        for(const auto& spec:hpeq::standalone::v2Parameters())expectWithinAbsoluteError(q->parameter(spec.id)->getValue(),q->parameter(spec.id)->getDefaultValue(),1e-6f,spec.id);
        juce::MemoryBlock restored;q->getStateInformation(restored);expect(restored==oldState,"Compact old state remains byte-identical");
        beginTest("UI-only page/node changes do not mark a preset modified; program load resets modern settings");
        q->setCurrentProgram(0);q->setParameterValue("v2Page",1);q->setParameterValue("v2EditNode",16);expect(!q->presetModified());q->setParameterValue("v2Space",.5);expect(q->presetModified());q->setCurrentProgram(0);expect(!q->presetModified());expectEquals(q->parameterValue("v2Mode"),0.0);
        beginTest("Mono input can feed stereo without collapsing generated spatial Side");
        q=instance();auto layout=q->getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::mono());layout.outputBuses.set(0,juce::AudioChannelSet::stereo());expect(q->setBusesLayout(layout));q->setParameterValue("v2Generate",.75);q->setParameterValue("v2Generator",2);const auto y=render(*q,mono);double side=0;for(int i=2000;i<y.getNumSamples();++i)side+=std::pow(y.getSample(0,i)-y.getSample(1,i),2);expect(side>1e-6);
        beginTest("Travel Rate changes speed without rewinding path phase");
        hamlo::shirota::v2::MotionClock clock;clock.prepare(48000);hamlo::shirota::v2::MotionPath path;path.count=1;path.nodes[0]={0,0,1000,hamlo::shirota::v2::MotionArc::Counterclockwise,hamlo::shirota::v2::MotionCurve::Linear};clock.setPath(path);
        for(int i=0;i<24000;++i)clock.next();clock.setRate(2);expectWithinAbsoluteError(clock.next().azimuth,180.,1e-7);for(int i=0;i<11999;++i)clock.next();expectWithinAbsoluteError(std::remainder(clock.next().azimuth,360.),0.,1e-7);
        beginTest("Stable genre ID restores selection despite a stale numeric index, then Default clears the modern state");
        {
            struct StateCodec:juce::AudioProcessor
            {using juce::AudioProcessor::getXmlFromBinary;using juce::AudioProcessor::copyXmlToBinary;};
            auto saved=instance();saved->setCurrentProgram(7);juce::MemoryBlock state;saved->getStateInformation(state);
            auto xml=StateCodec::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));
            expect(xml!=nullptr);
            if(xml)
            {
                expectEquals(xml->getStringAttribute("v2PresetID"),juce::String(hpeq::standalone::genrePresets().front().id));
                xml->setAttribute("v2Program",206);StateCodec::copyXmlToBinary(*xml,state);
                auto restored=instance(2);restored->setCurrentProgram(206);
                restored->setParameterValue("v2Width",saved->parameterValue("v2Width"));
                restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
                expectEquals(restored->getCurrentProgram(),7);expect(!restored->presetModified());
                expectWithinAbsoluteError(restored->parameterValue("v2Width"),saved->parameterValue("v2Width"),1e-7,"Saved value equal to the pre-load value must not leave the UI at its default");
                restored->setCurrentProgram(0);juce::MemoryBlock actual,expected;restored->getStateInformation(actual);
                auto clean=instance(0);clean->getStateInformation(expected);expect(actual==expected,"Default returns to the original compact state after a genre project");
            }
        }
        beginTest("Actual integrated binaural impulse equals the independent embedded filter for all profiles and both sides");
        juce::AudioBuffer<float> impulse(2,4096);impulse.clear();impulse.setSample(0,512,.5f);impulse.setSample(1,512,.5f);
        std::vector<float> bank(3*72*2*512);std::memcpy(bank.data(),ShirotaHrtf::production_bin,bank.size()*4);
        for(int profile=0;profile<3;++profile)for(int direction:{90,270})
        {
            auto convolver=instance(2);convolver->setParameterValue("v2PositionOn",1);convolver->setParameterValue("v2Source",1);convolver->setParameterValue("v2Position",direction==270?-90:90);convolver->setParameterValue("v2Profile",profile);const auto actual=render(*convolver,impulse);double error=0,reference=0;
            for(int ear=0;ear<2;++ear)for(int i=0;i<4096;++i){const int tap=i-512-64;const double expected=tap>=0&&tap<512?.5*bank[static_cast<size_t>(((profile*72+direction/5)*2+ear)*512+tap)]:0;error+=std::pow(actual.getSample(ear,i)-expected,2);reference+=expected*expected;}
            expect(error/std::max(reference,1e-30)<1e-9,"Integrated FIR error <=-90dB");
        }
    }
};
IntegrationTests tests;
}
