#include "Standalone/QuickControls.h"
#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"
#include "AllocationAudit.h"
#include <cstring>
#include <memory>

namespace
{
using namespace hpeq::standalone;
std::unique_ptr<ModulePluginProcessor> instance(int style=0)
{
    auto p=std::make_unique<ModulePluginProcessor>(ModuleKind::Echo);
    p->setQuickEnabled(true);p->setParameterValue("quickStyle",style);p->setParameterValue("quickSync",0);
    p->setParameterValue("quickTime",.4);p->setParameterValue("quickMotion",.65);p->setParameterValue("quickSpace",.35);
    return p;
}
void process(ModulePluginProcessor& p,juce::AudioBuffer<float>& out,int start,int end,int block)
{
    juce::MidiBuffer midi;
    for(int pos=start;pos<end;pos+=block)
    {juce::AudioBuffer<float> part(out.getArrayOfWritePointers(),2,pos,juce::jmin(block,end-pos));p.processBlock(part,midi);}
}
class QuickTests final:public juce::UnitTest
{
public:
    QuickTests():juce::UnitTest("Quick Note Score","HamloEcho"){}
    void runTest() override
    {
        beginTest("Append-only contract, deterministic bounded recipes and retained shared controls");
        expectEquals(scoreparam::BaseCount,143);expectEquals(static_cast<int>(scoreparam::Count),149);
        double previous=0;
        for(int i=0;i<=10;++i)
        {
            const int d=quickDivision(static_cast<float>(i)/10);const double beats=hamlo::echo::divisionBeats(d);
            expect(beats>previous,"TIME divisions must increase monotonically");previous=beats;
            expectEquals(quickDivision(quickTimeForDivision(d)),d,"Typed timing must select the same division");
        }
        for(int style=0;style<8;++style)for(float macro:{0.f,.5f,1.f})for(bool sync:{false,true})
        {
            const auto a=quickRecipe(style,macro,macro,macro,sync),b=quickRecipe(style,macro,macro,macro,sync);
            bool valid=true,identical=true;
            for(int i=0;i<scoreparam::BaseCount;++i)
            {
                const auto& spec=scoreParameterSpecs()[static_cast<size_t>(i)];const float v=a.actual(i);
                valid=valid&&std::isfinite(v)&&v>=spec.minimum&&v<=spec.maximum;
                identical=identical&&a.normalised[static_cast<size_t>(i)]==b.normalised[static_cast<size_t>(i)];
            }
            expect(valid,quickName(style));expect(identical);
            for(int i:{scoreparam::Mix,scoreparam::Output,scoreparam::Routing,scoreparam::MixLock,scoreparam::Freeze,scoreparam::Octaves})expect(!a.applies[static_cast<size_t>(i)]);
        }
        beginTest("Quick disabled by Default and old projects; zero Mix remains exact dry");
        {
            auto p=instance();const auto input=hamlo::test::drums(.6);auto out=input;p->prepareToPlay(48000,256);process(*p,out,0,out.getNumSamples(),127);
            expect(hamlo::test::bitExact(out,input));p->setCurrentProgram(0);expectEquals(static_cast<int>(p->parameterValue("quickEnabled")),0);
            for(int program=1;program<208;++program){p->setCurrentProgram(program);expectEquals(static_cast<int>(p->parameterValue("quickEnabled")),0);}
            const auto old=juce::File(__FILE__).getParentDirectory().getParentDirectory().getChildFile("out/m0/fixtures/program_0_48000_ch2.state");juce::MemoryBlock state;
            if(old.existsAsFile())
            {expect(old.loadFileAsData(state));p->setQuickEnabled(true);p->setStateInformation(state.getData(),static_cast<int>(state.getSize()));expectEquals(static_cast<int>(p->parameterValue("quickEnabled")),0);}
            else logMessage("SKIP: machine-local M0 archive absent; generated pre-Quick Score migration still runs below");
        }
        beginTest("Pre-Quick Score states preserve clean/modified labels and reset new macros");
        {
            auto p=instance();p->setCurrentProgram(20);auto state=p->valueTreeState().copyState();
            uint64_t fingerprint=1469598103934665603ull;
            const auto append=[&](const juce::String& id){const float v=p->valueTreeState().getRawParameterValue(id)->load();uint32_t bits=0;std::memcpy(&bits,&v,sizeof(bits));fingerprint=(fingerprint^bits)*1099511628211ull;};
            for(const auto& spec:p->getSpec().parameters)append(spec.id);
            for(int i=0;i<scoreparam::BaseCount;++i)append(scoreParameterSpecs()[static_cast<size_t>(i)].id);
            for(int i=scoreparam::BaseCount;i<scoreparam::Count;++i)state.removeChild(state.getChildWithProperty("id",scoreParameterSpecs()[static_cast<size_t>(i)].id),nullptr);
            state.setProperty("recipeHash",juce::String::toHexString(static_cast<juce::int64>(fingerprint)),nullptr);state.setProperty("selectedProgram",20,nullptr);
            for(bool modified:{false,true})
            {
                if(modified)state.getChildWithProperty("id","tap1Note").setProperty("value",61,nullptr);
                const auto xml=state.createXml();juce::MemoryBlock bytes;juce::AudioProcessor::copyXmlToBinary(*xml,bytes);
                p->setQuickEnabled(true);p->setStateInformation(bytes.getData(),static_cast<int>(bytes.getSize()));
                expect(p->presetModified()==modified);expectEquals(static_cast<int>(p->parameterValue("quickEnabled")),0);
            }
        }
        beginTest("Quick temporary recipe, exact transfer and uninterrupted audio for all eight moves");
        const auto input=hamlo::test::drums(1.2);
        for(int style=0;style<8;++style)
        {
            auto p=instance(style),q=instance(style);p->setParameterValue("tap3Note",51);p->setParameterValue("scoreMix",.7);q->setParameterValue("scoreMix",.7);
            const auto recipe=quickRecipe(style,static_cast<float>(p->parameterValue("quickTime")),static_cast<float>(p->parameterValue("quickMotion")),static_cast<float>(p->parameterValue("quickSpace")),false);
            p->setQuickEnabled(false);expectWithinAbsoluteError(p->parameterValue("tap3Note"),51.,0.);p->setQuickEnabled(true);
            p->prepareToPlay(48000,256);q->prepareToPlay(48000,256);auto a=input,b=input;
            const int split=10001;process(*p,a,0,split,127);process(*q,b,0,split,127);
            p->expandQuickToScore();expectEquals(static_cast<int>(p->parameterValue("quickEnabled")),0);
            for(int i=0;i<scoreparam::BaseCount;++i)if(recipe.applies[static_cast<size_t>(i)])
                expectEquals(p->valueTreeState().getRawParameterValue(scoreParameterSpecs()[static_cast<size_t>(i)].id)->load(),recipe.actual(i),scoreParameterSpecs()[static_cast<size_t>(i)].id);
            process(*p,a,split,a.getNumSamples(),127);process(*q,b,split,b.getNumSamples(),127);
            expect(hamlo::test::bitExact(a,b),juce::String(quickName(style))+" transfer must not restart, fade or alter the running sound");
            bool finite=true;double difference=0;
            for(int ch=0;ch<2;++ch)for(int i=0;i<a.getNumSamples();++i){finite=finite&&std::isfinite(a.getSample(ch,i));difference+=std::abs(a.getSample(ch,i)-input.getSample(ch,i));}
            expect(finite&&difference>1,quickName(style));
            juce::MemoryBlock state;q->getStateInformation(state);auto restored=instance();restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
            for(const auto& spec:scoreParameterSpecs())expectWithinAbsoluteError(restored->parameterValue(spec.id),q->parameterValue(spec.id),1e-4,spec.id);
        }
        beginTest("Maximum controls remain finite and bounded on a dense input, all eight moves");
        for(int style=0;style<8;++style)
        {
            auto p=instance(style);p->setParameterValue("scoreMix",1);p->setParameterValue("quickTime",0);p->setParameterValue("quickMotion",1);p->setParameterValue("quickSpace",1);
            p->prepareToPlay(48000,256);auto out=hamlo::test::drums(3);process(*p,out,0,out.getNumSamples(),127);
            bool finite=true;for(int ch=0;ch<2;++ch)for(int i=0;i<out.getNumSamples();++i)finite=finite&&std::isfinite(out.getSample(ch,i));
            expect(finite&&out.getMagnitude(0,out.getNumSamples())<16,quickName(style));
        }
        beginTest("Macro, move and tempo events are block invariant and prepared processing allocates zero");
        struct Playhead:juce::AudioPlayHead
        {double bpm=120;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(bpm);p.setIsPlaying(true);return p;}};
        const auto render=[&](int block,bool allocations)
        {
            auto p=instance();p->setParameterValue("scoreMix",.65);p->setParameterValue("quickSync",1);Playhead ph;p->setPlayHead(&ph);p->prepareToPlay(48000,256);
            auto out=input;juce::MidiBuffer midi;size_t count=0;const int events[]{0,7011,14001,22003,30001,38009,45007,50001,out.getNumSamples()};
            for(int e=0;e<8;++e)
            {
                p->setParameterValue("quickStyle",e);p->setParameterValue("quickMotion",e/7.);p->setParameterValue("quickSpace",e/9.);p->setParameterValue("quickTime",.1+.025*e);ph.bpm=e%2?174:90;
                for(int pos=events[e];pos<events[e+1];pos+=block)
                {
                    juce::AudioBuffer<float> part(out.getArrayOfWritePointers(),2,pos,juce::jmin(block,events[e+1]-pos));
                    if(allocations)hamlo::test::allocation::begin();p->processBlock(part,midi);if(allocations)count+=hamlo::test::allocation::finish();
                }
            }
            if(allocations)expectEquals(static_cast<int>(count),0);p->setPlayHead(nullptr);return out;
        };
        const auto reference=render(256,true);
        for(int block:{1,64,127,512,4096})expect(hamlo::test::bitExact(render(block,false),reference),"Quick block "+juce::String(block));
    }
};
QuickTests quickTests;
}
