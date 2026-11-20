#include "Rack/NativeApi.h"
#include "Core/FxProjectState.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>
#include <stdexcept>

namespace hpeq::native
{
void Handle::bind(juce::AudioProcessorValueTreeState& state)
{
    for (auto* p : processor->getParameters())
        if (auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(p))
        {
            auto* raw=state.getRawParameterValue(ranged->paramID);
            if (raw==nullptr) throw std::runtime_error("Native APVTS binding missing");
            bindings.push_back({ranged,raw});
        }
    if (bindings.empty() || bindings.size()>kMaxParameters) throw std::runtime_error("Native parameter capacity exceeded");
}

Handle create(FxType type)
{
    switch(type)
    {
        case FxType::Compressor:return createComp();
        case FxType::Reverb:return createVerb();
        case FxType::Delay:return createEcho();
        case FxType::Saturator:return createMag();
        case FxType::TransientShaper:return createUdarnik();
        case FxType::GateExpander:return createZastava();
        case FxType::StereoProcessor:return createShirota();
        case FxType::Limiter:return createRedWall();
        case FxType::Gain:return createFader();
        case FxType::MonstaFx:return createRevolution();
    }
    throw std::runtime_error("Unsupported native module");
}

const std::vector<Parameter>& layout(FxType type)
{
    // This cache is only entered by message-thread description/preparation.
    static std::mutex lock;
    static std::map<FxType,std::vector<Parameter>> layouts;
    const std::lock_guard<std::mutex> guard(lock);
    const auto found=layouts.find(type);
    if(found!=layouts.end())return found->second;
    auto h=create(type);
    std::vector<Parameter> p;
    for (const auto& b:h.bindings)
        p.push_back({b.parameter->paramID,b.parameter->getNormalisableRange(),
            b.parameter->convertFrom0to1(b.parameter->getDefaultValue()),b.parameter->isDiscrete()});
    return layouts.emplace(type,std::move(p)).first->second;
}

juce::String productName(FxType type)
{
    switch(type)
    {
        case FxType::Compressor:return "HamloComp";
        case FxType::Reverb:return "HamloVerb";
        case FxType::Delay:return "HamloEcho";
        case FxType::Saturator:return "HamloMag";
        case FxType::TransientShaper:return "HamloUdarnik";
        case FxType::GateExpander:return "HamloZastava";
        case FxType::StereoProcessor:return "HamloShirota";
        case FxType::Limiter:return "HamloRedWall";
        case FxType::Gain:return "HamloFader";
        case FxType::MonstaFx:return "HamloRevolution";
    }
    return {};
}

int fixedLatency(FxType type,double rate)
{
    static std::mutex lock;
    static std::map<std::pair<FxType,double>,int> cache;
    const std::lock_guard<std::mutex> guard(lock);
    const auto key=std::make_pair(type,rate);
    if(const auto found=cache.find(key);found!=cache.end())return found->second;
    int value=0;
    switch(type)
    {
        case FxType::Compressor:value=latencyComp(rate);break;
        case FxType::Reverb:value=latencyVerb(rate);break;
        case FxType::Delay:value=latencyEcho(rate);break;
        case FxType::Saturator:value=latencyMag(rate);break;
        case FxType::TransientShaper:value=latencyUdarnik(rate);break;
        case FxType::GateExpander:value=latencyZastava(rate);break;
        case FxType::StereoProcessor:value=latencyShirota(rate);break;
        case FxType::Limiter:value=latencyRedWall(rate);break;
        case FxType::Gain:value=latencyFader(rate);break;
        case FxType::MonstaFx:value=latencyRevolution(rate);break;
    }
    cache.emplace(key,value);return value;
}

juce::NamedValueSet defaultParameters(FxType type)
{
    juce::NamedValueSet p;
    for(const auto& spec:layout(type))p.set(spec.id,static_cast<double>(spec.initial));
    const auto set=[&](const char* key,double v){if(p.contains(key))p.set(key,v);};
    set("bypass",0); set("mix",1);
    switch(type)
    {
        case FxType::Reverb:set("engine",1);set("routing",1);set("keepDry",0);break;
        case FxType::Delay:set("echoMode",1);set("quickEnabled",0);set("scoreRouting",1);set("scoreMix",1);break;
        case FxType::GateExpander:set("v2_engine",1);set("v2_enabled",1);set("v2_mix",1);break;
        case FxType::StereoProcessor:set("v2Mode",1);set("v2Mix",1);break;
        case FxType::Gain:set("engine",1);set("enabled",1);break;
        case FxType::MonstaFx:
            for(const auto* k:{"timeOn","glitchOn","motionOn","shapeAOn","shapeBOn","throwAOn","throwBOn"})set(k,0);
            break;
        default:break;
    }
    return p;
}

bool describe(FxType type,const juce::NamedValueSet& params,double rate,Values& values,juce::String& error)
{
    const auto& specs=layout(type);
    values.count=static_cast<int>(specs.size());values.latency=fixedLatency(type,rate);
    for(size_t i=0;i<specs.size();++i)values.physical[i]=specs[i].initial;
    for(const auto& entry:params)
    {
        const auto found=std::find_if(specs.begin(),specs.end(),[&](const Parameter& p){return p.id==entry.name.toString();});
        if(found==specs.end()){error="Unknown native parameter: "+entry.name.toString();return false;}
        const auto n=static_cast<size_t>(std::distance(specs.begin(),found));
        const double value=static_cast<double>(entry.value);
        if(!(entry.value.isInt()||entry.value.isInt64()||entry.value.isDouble()||entry.value.isBool()) || !std::isfinite(value)
            || value<found->range.start || value>found->range.end
            || ((found->discrete||found->range.interval>0) && std::abs(value-found->range.snapToLegalValue(static_cast<float>(value)))>1.e-4))
        {error="Invalid native value: "+entry.name.toString();return false;}
        values.physical[n]=static_cast<float>(value);
    }
    const auto actual=[&](const char* name,float fallback=0.f)
    {
        for(size_t i=0;i<specs.size();++i)if(specs[i].id==name)return values.physical[i];
        return fallback;
    };
    if(actual("bypass")!=0 || actual("mix",1)!=1)
    {error="Native bypass/MIX is owned by the rack slot";return false;}
    if(type==FxType::Reverb && (actual("routing")!=1||actual("keepDry")!=0))
    {error="Native Verb requires a wet-only send";return false;}
    if(type==FxType::Delay && (actual("echoMode")!=1||actual("scoreRouting")!=1||actual("scoreMix")!=1))
    {error="Native Echo requires a Score wet-only send";return false;}
    if((type==FxType::StereoProcessor&&actual("v2Mix",1)!=1) || (type==FxType::GateExpander&&actual("v2_mix",1)!=1))
    {error="Native spatial/gate MIX is owned by the rack slot";return false;}
    return true;
}

juce::Optional<juce::AudioPlayHead::PositionInfo> Runtime::PlayHead::getPosition() const
{
    PositionInfo p;p.setBpm(bpm);p.setIsPlaying(playing);
    if(position)p.setPpqPosition(ppq);
    return p;
}

Runtime::Runtime(FxType kind,const juce::MemoryBlock& structuralState,const juce::NamedValueSet& params,
                 double sampleRate,int maximumBlockSize,juce::String& error)
    :handle(create(kind)),type(kind),maxBlock(std::max(1,maximumBlockSize)),rate(sampleRate)
{
    Values values;
    if(!describe(type,params,rate,values,error))return;
    if(structuralState.getSize()>0)
    {
        const auto xml=juce::AudioProcessor::getXmlFromBinary(structuralState.getData(),static_cast<int>(structuralState.getSize()));
        if(xml==nullptr||xml->getTagName()!=handle.processor->getName())
        {error="Invalid native structural state";return;}
        for(const auto& p:layout(type))if(!params.contains(p.id))
        {error="Native structural state requires complete physical parameters";return;}
        handle.processor->setStateInformation(structuralState.getData(),static_cast<int>(structuralState.getSize()));
    }
    for(int i=0;i<values.count;++i)
    {
        auto* p=handle.bindings[static_cast<size_t>(i)].parameter;
        p->setValueNotifyingHost(p->convertTo0to1(values.physical[static_cast<size_t>(i)]));
    }
    handle.processor->setPlayHead(&playHead);
    handle.processor->setPlayConfigDetails(2,2,rate,maxBlock);
    handle.processor->prepareToPlay(rate,maxBlock);
    reportedLatency=handle.processor->getLatencySamples();
    if(reportedLatency!=values.latency){error="Native fixed-latency contract mismatch";return;}
    ready=true;
}

Runtime::~Runtime(){handle.processor->setPlayHead(nullptr);handle.processor->releaseResources();}
double Runtime::tailSeconds() const noexcept{return ready?handle.processor->getTailLengthSeconds():0;}

void Runtime::process(float* const* channels,int count,int samples,const Values& values,
                      double bpm,bool playing,bool hasPosition,double ppq) noexcept
{
    jassert(ready && values.count==static_cast<int>(handle.bindings.size()));
    for(int i=0;i<values.count;++i)handle.bindings[static_cast<size_t>(i)].raw->store(values.physical[static_cast<size_t>(i)],std::memory_order_relaxed);
    for(int offset=0;offset<samples;offset+=maxBlock)
    {
        float* ptr[2]{channels[0]+offset,count>1?channels[1]+offset:nullptr};
        const int n=std::min(maxBlock,samples-offset);
        playHead.bpm=bpm;playHead.playing=playing;playHead.position=hasPosition;
        playHead.ppq=ppq+offset*bpm/(60.0*rate);
        juce::AudioBuffer<float> block(ptr,count,n);
        midi.clear();handle.processor->processBlock(block,midi);
    }
}

juce::MemoryBlock Runtime::state()
{
    jassert(inUse()==0);
    juce::MemoryBlock block;if(inUse()==0)handle.processor->getStateInformation(block);return block;
}
} // namespace hpeq::native
