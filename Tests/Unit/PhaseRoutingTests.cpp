#include "DSP/ProjectEngine.h"
#include "Harness/RtAllocationProbe.h"
#include "Tools/CalibrationSearch.h"
#include <cmath>
#include <memory>

namespace hpeq::test {
namespace {
FxProjectState neutralProject(LayerRouting routing)
{
    FxProjectState p;p.routing=routing;
    for(auto& layer:p.layers)layer.enabled=false;
    for(int j=0;j<2;++j) {
        auto& layer=p.layers[static_cast<size_t>(j)];layer.enabled=true;layer.mix=.5f;
        FxNodeState n;n.id=p.nextNodeId++;n.lowHz=j?900:250;n.highHz=j?3600:700;
        n.chain[0].type=FxType::Gain;n.chain[0].parameters.set("gainDb",0.0);
        layer.fxNodes.push_back(n);
    }
    sanitize(p);return p;
}
juce::AudioBuffer<float> stimulus()
{
    juce::AudioBuffer<float> x(2,36000);juce::Random r(78191);
    for(int i=0;i<x.getNumSamples();++i) {
        const auto l=static_cast<float>(.1*std::sin(i*.017)+.015*r.nextFloat());
        const auto s=static_cast<float>(.04*std::sin(i*.097));x.setSample(0,i,l+s);x.setSample(1,i,l-s);
    }
    return x;
}
juce::AudioBuffer<float> dynamicRender(int block,LayerRouting routing,std::uint64_t& allocations)
{
    auto p=neutralProject(routing);auto e=std::make_unique<dsp::ProjectEngine>();e->prepare(48000,256);e->publish(p);
    auto y=stimulus();const int edits[]{2048,8192,12288,20480,24576,36000};int edit=0;
    for(int start=0;start<y.getNumSamples();) {
        if(start==edits[edit]) {
            if(edit==0){p.layers[0].fxNodes[0].lowHz=400;p.layers[0].fxNodes[0].highHz=1400;}
            if(edit==1)p.layers[1].enabled=false;
            if(edit==2)p.layers[1].enabled=true;
            if(edit==3)p.layers[0].channel=ChannelMode::Mid;
            if(edit==4){p.layers[0].mix=.75f;p.layers[1].mix=.25f;}
            e->publish(p);++edit;
        }
        const int n=std::min({block,y.getNumSamples()-start,edits[edit]-start});
        float* io[]{y.getWritePointer(0,start),y.getWritePointer(1,start)};
        {ScopedAllocationProbe probe;e->process(io,2,n);allocations+=probe.counts().allocations;}
        start+=n;
    }
    return y;
}
}
class PhaseRoutingTests final:public juce::UnitTest {
public:
    PhaseRoutingTests():UnitTest("Phase routing repair","HPEQ"){}
    void runTest() override {
        beginTest("Calibration: a valid GR wins over a closer invalid measurement in an asymmetric target band");
        expect(preferCompressorMeasurement(11.9,5.9,8,6,12));
        expect(!preferCompressorMeasurement(5.9,11.9,8,6,12));
        expect(preferCompressorMeasurement(8.5,11.9,8,6,12));
        beginTest("Coefficient replay and phase histories are block independent through retunes, disable/re-enable and M/S changes");
        for(auto route:{LayerRouting::Parallel,LayerRouting::Serial}) {
            std::uint64_t allocations=0;const auto reference=dynamicRender(1,route,allocations);
            expectEquals(allocations,std::uint64_t(0));
            for(int block:{64,127,512,4096}) {
                allocations=0;const auto actual=dynamicRender(block,route,allocations);double error=0,peak=0;bool finite=true;int worstSample=0;
                for(int ch=0;ch<2;++ch)for(int i=0;i<actual.getNumSamples();++i) {
                    const auto v=actual.getSample(ch,i);finite=finite&&std::isfinite(v);
                    const auto difference=std::abs(static_cast<double>(v-reference.getSample(ch,i)));
                    if(difference>error){error=difference;worstSample=i;}
                    peak=std::max(peak,std::abs(static_cast<double>(v)));
                }
                logMessage(toString(route)+" block "+juce::String(block)+" worst sample "+juce::String(worstSample)+" error "+juce::String(error,9));
                expect(error<=1.e-6,"block "+juce::String(block)+" max difference "+juce::String(error,9));
                expect(finite);
                expect(peak<.7,"Unexpected transition burst");expectEquals(allocations,std::uint64_t(0));
            }
        }
        beginTest("Wet-only neutral regions subtract their phase-aligned dry reference");
        for(auto route:{LayerRouting::Parallel,LayerRouting::Serial}) {
            auto p=neutralProject(route);p.wetOnly=true;auto e=std::make_unique<dsp::ProjectEngine>();e->prepare(48000,256);e->publish(p);
            auto y=stimulus();double peak=0;
            for(int i=0;i<y.getNumSamples();i+=127){float* io[]{y.getWritePointer(0,i),y.getWritePointer(1,i)};e->process(io,2,std::min(127,y.getNumSamples()-i));}
            for(int ch=0;ch<2;++ch)for(int i=12000;i<y.getNumSamples();++i)peak=std::max(peak,std::abs(static_cast<double>(y.getSample(ch,i))));
            expect(peak<=1.e-6,"Neutral wet-only peak "+juce::String(peak,9));
        }
        beginTest("Disabling a region clears its phase program while a neutral EQ keeps the layer running");
        for(auto route:{LayerRouting::Parallel,LayerRouting::Serial})for(int block:{1,64,127,512,4096})
        {
            auto p=neutralProject(route);p.layers[1].enabled=false;
            EqNodeState eq;eq.id=p.nextNodeId++;eq.gainDb=0;p.layers[0].eqNodes.push_back(eq);sanitize(p);
            auto e=std::make_unique<dsp::ProjectEngine>();e->prepare(48000,256);e->publish(p);
            const auto input=stimulus();auto y=input;
            for(int i=0;i<y.getNumSamples();)
            {
                if(i==8192){p.layers[0].fxNodes[0].enabled=false;e->publish(p);}
                const int n=std::min({block,y.getNumSamples()-i,i<8192?8192-i:y.getNumSamples()-i});
                float* io[]{y.getWritePointer(0,i),y.getWritePointer(1,i)};e->process(io,2,n);i+=n;
            }
            double error=0;
            for(int ch=0;ch<2;++ch)for(int i=18000;i<y.getNumSamples();++i)
                error=std::max(error,std::abs(static_cast<double>(y.getSample(ch,i)-input.getSample(ch,i))));
            expect(error<=1.e-6,toString(route)+" block "+juce::String(block)+" retired error "+juce::String(error,9));
        }
    }
} phaseRoutingTests;
}
