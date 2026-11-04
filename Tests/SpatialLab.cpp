#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
using P = hpeq::standalone::ModulePluginProcessor;
namespace rev = hpeq::revolution;
namespace simple = rev::simple;
struct Head final : juce::AudioPlayHead
{
    double q = 0;
    juce::Optional<PositionInfo> getPosition() const override
    { PositionInfo p; p.setBpm(120); p.setIsPlaying(true); p.setPpqPosition(q); return p; }
};
static auto make() { return std::make_unique<P>(hpeq::standalone::ModuleKind::Revolution, true); }
static juce::AudioBuffer<float> render(P& p, const juce::AudioBuffer<float>& x)
{
    auto y = x; Head head; p.setPlayHead(&head); p.prepareToPlay(48000,256); juce::MidiBuffer midi;
    for(int at=0;at<y.getNumSamples();at+=256)
    { const int n=std::min(256,y.getNumSamples()-at);head.q=at/24000.;juce::AudioBuffer<float> b(y.getArrayOfWritePointers(),2,at,n);p.processBlock(b,midi); }
    p.setPlayHead(nullptr); return y;
}
static juce::String hash(const juce::AudioBuffer<float>& y)
{
    juce::MemoryBlock b; for(int c=0;c<y.getNumChannels();++c)b.append(y.getReadPointer(c),sizeof(float)*static_cast<size_t>(y.getNumSamples()));
    return juce::SHA256(b).toHexString();
}
static void wav(const juce::File& f,const juce::AudioBuffer<float>& x)
{
    juce::WavAudioFormat format;auto stream=f.createOutputStream();
    std::unique_ptr<juce::AudioFormatWriter> w(format.createWriterFor(stream.get(),48000,2,32,{},0));
    if(!w)throw std::runtime_error("WAV writer");stream.release();if(!w->writeFromAudioSampleBuffer(x,0,x.getNumSamples()))throw std::runtime_error("WAV write");
}
int main(int argc,char**argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if(argc!=4)return 2;const juce::File folder(argv[1]), baseline(argv[3]); const bool before=juce::String(argv[2])=="before";
    if(folder.exists())return 3;if(folder.createDirectory().failed())return 4;
    try
    {
        juce::Array<juce::var> programs,probes;
        auto source=hamlo::test::drums(3);for(int c=0;c<2;++c)source.clear(c,96000,48000);
        for(int i=0;i<225;++i)
        {
            auto p=make();p->setCurrentProgram(i);auto loaded=render(*p,source);
            const auto file=baseline.getChildFile("state_"+juce::String(i)+".bin");juce::MemoryBlock state;
            if(before){p->getStateInformation(state);if(!folder.getChildFile(file.getFileName()).replaceWithData(state.getData(),state.getSize()))throw std::runtime_error("state write");}
            else if(!file.loadFileAsData(state))throw std::runtime_error("state read");
            auto restored=make();if(!before){restored->setParameterValue("throwASource",1);restored->setParameterValue("throwBSource",1);}
            restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));auto replay=render(*restored,source);
            auto row=juce::var(new juce::DynamicObject);auto* o=row.getDynamicObject();o->setProperty("program",i);o->setProperty("load",hash(loaded));o->setProperty("restore",hash(replay));o->setProperty("latency",p->getLatencySamples());
            if(!before)o->setProperty("missing_sources_reset",restored->parameterValue("throwASource")==0&&restored->parameterValue("throwBSource")==0);
            programs.add(row);if(i%25==0)std::cout<<"program "<<i<<std::endl;
        }
        auto x=hamlo::test::drums(8);x.applyGain(.4f);for(int c=0;c<2;++c)x.clear(c,192000,192000);wav(folder.getChildFile("dry.wav"),x);
        for(int genre=1;genre<=4;++genre)for(int target : {rev::ThrowDelay,rev::ThrowReverb})for(int stale=0;stale<2;++stale)
        {
            auto p=make();P::RevolutionState s; s.lanes[5].condition=stale?rev::EightOfEight:rev::Always;
            p->setRevolutionState(s);for(auto id:{"timeOn","glitchOn","motionOn","shapeAOn","shapeBOn","throwBOn"})p->setParameterValue(id,0);
            p->setParameterValue("simpleGenre",genre);p->setParameterValue("throwATarget",target);p->generateLane(5);
            juce::MemoryBlock state;p->getStateInformation(state);auto off=make();off->setStateInformation(state.getData(),static_cast<int>(state.getSize()));off->setParameterValue("throwAOn",0);
            const auto y=render(*p,x),d=render(*off,x);double wet=0,dry=0,peak=0,tail=0;bool finite=true;
            for(int c=0;c<2;++c)for(int i=0;i<x.getNumSamples();++i){const double a=y.getSample(c,i),v=a-d.getSample(c,i);wet+=v*v;dry+=x.getSample(c,i)*x.getSample(c,i);peak=std::max(peak,std::abs(a));finite=finite&&std::isfinite(a);if(i>=192000+p->getLatencySamples())tail+=v*v;}
            const juce::String key=juce::String(genre)+"_"+juce::String(target)+"_"+juce::String(stale);auto row=juce::var(new juce::DynamicObject);auto* o=row.getDynamicObject();o->setProperty("key",key);o->setProperty("return_dry_db",10*std::log10(std::max(wet,1e-30)/std::max(dry,1e-30)));o->setProperty("tail_energy",tail);o->setProperty("peak",peak);o->setProperty("finite",finite);o->setProperty("source",p->parameter("throwASource")?juce::var(p->parameterValue("throwASource")):juce::var("n/a"));o->setProperty("condition",p->getRevolutionState().lanes[5].condition);probes.add(row);wav(folder.getChildFile("throw_"+key+".wav"),y);std::cout<<juce::JSON::toString(row,true)<<std::endl;
        }
        auto report=juce::var(new juce::DynamicObject);report.getDynamicObject()->setProperty("programs",programs);report.getDynamicObject()->setProperty("probes",probes);
        if(!folder.getChildFile("results.json").replaceWithText(juce::JSON::toString(report)))return 5;
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}
