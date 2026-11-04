#include "Core/RevolutionRecipes.h"
#include "Standalone/ModulePluginProcessor.h"
#include "AllocationAudit.h"
#include "TestSignals.h"
#include <set>
#include <memory>
namespace hamlo::test
{
namespace
{
using P=hpeq::standalone::ModulePluginProcessor;
namespace rev=hpeq::revolution;
namespace simple=rev::simple;
struct Head final:juce::AudioPlayHead
{
    double q=0;
    juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(120);p.setIsPlaying(true);p.setPpqPosition(q);return p;}
};
auto make(int target,int source,int band=rev::Full,bool gate=true)
{
    auto p=std::make_unique<P>(hpeq::standalone::ModuleKind::Revolution,true);P::RevolutionState s;
    s.lanes[5].first.fill(15);s.lanes[5].band=band;s.lanes[3].first.fill(15);p->setRevolutionState(s);
    for(auto id:{"timeOn","glitchOn","motionOn","shapeBOn","throwBOn"})p->setParameterValue(id,0);
    p->setParameterValue("shapeAOn",gate?1:0);p->setParameterValue("shapeATarget",rev::ShapeVolume);
    p->setParameterValue("throwATarget",target);p->setParameterValue("throwASource",source);p->setParameterValue("throwATail",.55);
    return p;
}
juce::AudioBuffer<float> play(P& p,const juce::AudioBuffer<float>& x,int block=256,size_t* allocations=nullptr)
{
    auto y=x;Head h;p.setPlayHead(&h);p.prepareToPlay(48000,256);juce::MidiBuffer midi;
    size_t count=0;
    for(int at=0;at<y.getNumSamples();at+=block)
    {
        const int n=std::min(block,y.getNumSamples()-at);h.q=at/24000.;juce::AudioBuffer<float> b(y.getArrayOfWritePointers(),y.getNumChannels(),at,n);
        if(allocations)allocation::begin();p.processBlock(b,midi);if(allocations)count+=allocation::finish();
    }
    if(allocations)*allocations=count;p.setPlayHead(nullptr);return y;
}
double energy(const juce::AudioBuffer<float>& x,int from=24000)
{double e=0;for(int c=0;c<x.getNumChannels();++c)for(int i=from;i<x.getNumSamples();++i)e+=x.getSample(c,i)*x.getSample(c,i);return e;}
}
class SpatialTests final:public juce::UnitTest
{
public:
    SpatialTests():juce::UnitTest("Spatial recipes","HamloRevolution"){}
    void runTest()override
    {
        beginTest("INPUT sends survive a fully closed upstream gate; old CHAIN stays closed");
        auto x=drums(2);x.applyGain(.25f);
        // First run fed audio into the intentional 10ms SHAPE startup ramp, then
        // wrongly expected its delayed tail to be zero. Let controls settle on silence.
        for(int c=0;c<2;++c)x.clear(c,0,24000);
        for(int target:{rev::ThrowDelay,rev::ThrowReverb})
        {
            auto a=make(target,0),b=make(target,1);const auto chain=play(*a,x),input=play(*b,x);
            expect(energy(chain)<1e-18);expect(energy(input)>.01);
            logMessage("target "+juce::String(target)+" gated chain energy="+juce::String(energy(chain),10)+" input energy="+juce::String(energy(input),6));
        }
        beginTest("INPUT routing respects frequency band and does not filter the dry chain");
        for(int target:{rev::ThrowDelay,rev::ThrowReverb})
        {
            auto low=make(target,1,rev::High),high=make(target,1,rev::High);
            const auto l=play(*low,sine(2,80,-18)),h=play(*high,sine(2,6000,-18));
            const double rejection=10*std::log10(std::max(energy(l),1e-30)/std::max(energy(h),1e-30));
            logMessage("band rejection "+juce::String(rejection,2)+" dB");expect(rejection<=-30);
            auto dry=make(target,1,rev::High,false);dry->setParameterValue("throwADepth",0);
            expect(bitExact(play(*dry,x),x),"a zero send has exact original dry, not a crossover allpass");
        }
        beginTest("New routing deterministic for all blocks, both layouts and sources; zero C++ audio allocation");
        for(int channels:{1,2})for(int target:{rev::ThrowDelay,rev::ThrowReverb})for(int band:{rev::Full,rev::Tops})
        {
            juce::AudioBuffer<float> probe(channels,48000);for(int c=0;c<channels;++c)probe.copyFrom(c,0,x,c,0,48000);
            auto p=make(target,1,band);if(channels==1)p->setPlayConfigDetails(1,1,48000,256);const auto reference=play(*p,probe);
            for(int block:{1,64,127,512,4096})
            {
                auto q=make(target,1,band);if(channels==1)q->setPlayConfigDetails(1,1,48000,256);size_t count=0;const auto y=play(*q,probe,block,&count);
                expect(bitExact(reference,y),"block "+juce::String(block));expectEquals(static_cast<int>(count),0);
            }
        }
        beginTest("Target-aware recipes vary structure, capture attacks and remove stale scheduling");
        for(int lane=3;lane<7;++lane)for(int target=0;target<(lane<5?rev::kShapeTargets:rev::kThrowTargets);++target)
        {
            auto p=make(rev::ThrowDelay,0);p->setParameterValue(lane==3?"shapeATarget":lane==4?"shapeBTarget":lane==5?"throwATarget":"throwBTarget",target);
            const auto original=p->getRevolutionState();std::set<std::array<std::uint8_t,16>> distinct;
            for(int v=0;v<8;++v)
            {
                auto state=p->getRevolutionState();state.lanes[static_cast<size_t>(lane)].condition=rev::EightOfEight;state.lanes[static_cast<size_t>(lane)].probability.fill(7);
                p->setRevolutionState(state);p->generateLane(lane);const auto after=p->getRevolutionState();const auto& l=after.lanes[static_cast<size_t>(lane)];
                distinct.insert(l.first);expectEquals(l.condition,static_cast<int>(rev::Always));expectEquals(l.rate,0);expectEquals(l.length,16);
                expect(std::all_of(l.probability.begin(),l.probability.end(),[](auto a){return a==0;}));
                expect(after.pattern==original.pattern);expect(after.seed==original.seed);
                for(int other=0;other<7;++other)if(other!=lane)expect(after.lanes[static_cast<size_t>(other)]==original.lanes[static_cast<size_t>(other)]);
            }
            expect(distinct.size()>=4,"lane "+juce::String(lane)+" target "+juce::String(target)+" structures="+juce::String(distinct.size()));
        }
        beginTest("GEN preserves target/time/note/band and only touches selected lane parameters");
        {
            auto p=make(rev::ThrowDelay,0,rev::Mid);p->setParameterValue("throwATime",10);p->setParameterValue("throwANote",8);p->setParameterValue("shapeADepth",.234);
            p->generateLane(5);expectEquals(p->getRevolutionState().lanes[5].band,static_cast<int>(rev::Mid));
            expectWithinAbsoluteError(p->parameterValue("throwATime"),10.,1e-6);expectWithinAbsoluteError(p->parameterValue("throwANote"),8.,1e-6);expectWithinAbsoluteError(p->parameterValue("shapeADepth"),.234,1e-3);
            expectEquals(juce::roundToInt(p->parameterValue("throwASource")),1);
            juce::MemoryBlock state;p->getStateInformation(state);auto q=make(rev::ThrowReverb,0);q->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
            expect(q->currentSlot(true).values==p->currentSlot(true).values);
        }
        beginTest("Every spatial variation captures the strongest detected attack, including sustained sources");
        for(bool sustained:{false,true})for(int target:{rev::ThrowDelay,rev::ThrowReverb})for(int variant=0;variant<8;++variant)
        {
            simple::Settings settings;settings.sustained=sustained;simple::Hits hits;hits.snare[12]=1;hits.hats[2]=.3f;
            const auto r=simple::levelRecipe(5,target,variant,settings,hits);
            expectEquals(static_cast<int>(r.lanes[5].first[12]),15);expect(r.lanes[5].first[13]>0);
        }
        beginTest("Explicit generation adds delay/reverb; old program loading retains CHAIN");
        {
            auto p=make(rev::ThrowDelay,0);p->setCurrentProgram(25);expectEquals(juce::roundToInt(p->parameterValue("throwASource")),0);
            p->generateSimple(false);expectEquals(juce::roundToInt(p->parameterValue("throwASource")),1);expectEquals(juce::roundToInt(p->parameterValue("throwBSource")),1);
            expectEquals(juce::roundToInt(p->parameterValue("throwBTarget")),static_cast<int>(rev::ThrowReverb));
            p->setCurrentProgram(25);expectEquals(juce::roundToInt(p->parameterValue("throwASource")),0);
        }
    }
};static SpatialTests spatialTests;
}
