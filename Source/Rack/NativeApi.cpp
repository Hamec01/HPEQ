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
    {
        p.push_back({b.parameter->paramID,b.parameter->getNormalisableRange(),
            b.parameter->convertFrom0to1(b.parameter->getDefaultValue()),b.parameter->isDiscrete()});
        auto& added=p.back();added.name=b.parameter->getName(128);added.unit=b.parameter->getLabel();
        if(const auto* choice=dynamic_cast<juce::AudioParameterChoice*>(b.parameter))added.choices=choice->choices;
        else if(dynamic_cast<juce::AudioParameterBool*>(b.parameter)!=nullptr)added.choices={"Off","On"};
    }
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
    error.clear();
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
    if(type==FxType::Reverb && (actual("routing")!=1||actual("keepDry")!=0||actual("engine")<1))
    {error="Native Verb requires a wet-only send";return false;}
    if(type==FxType::Delay && (actual("echoMode")!=1||actual("scoreRouting")!=1||actual("scoreMix")!=1))
    {error="Native Echo requires a Score wet-only send";return false;}
    if((type==FxType::StereoProcessor&&actual("v2Mix",1)!=1) || (type==FxType::GateExpander&&actual("v2_mix",1)!=1))
    {error="Native spatial/gate MIX is owned by the rack slot";return false;}
    if(type==FxType::Gain){values.faderRouting=static_cast<int>(actual("routing"));values.faderSwap=actual("swap")!=0;}
    // Match the public JUCE path before binding raw APVTS atoms: setValue()
    // denormalises once, then ParameterAdapter reads getValue() and denormalises
    // again. A physical float written verbatim can differ by several ULPs.
    for(size_t i=0;i<specs.size();++i)
    {
        const auto& range=specs[i].range;
        auto value=values.physical[i];
        for(int pass=0;pass<2;++pass)
            value=range.snapToLegalValue(range.convertFrom0to1(
                range.convertTo0to1(range.snapToLegalValue(value))));
        values.physical[i]=value;
    }
    return true;
}

bool loadProgram(FxType type,int index,juce::NamedValueSet& params,juce::MemoryBlock& structural,juce::String& error,float* rackMix)
{
    auto h=create(type);
    if(index<0||index>=h.processor->getNumPrograms()){error="Unknown native program";return false;}
    if(index==0)
    {
        params=defaultParameters(type);structural.reset();if(rackMix)*rackMix=1.f;
        if(type==FxType::MonstaFx){Scene blank;structural=makeRevolutionScene(params,blank,error);return structural.getSize()>0;}
        error.clear();return true;
    }
    h.processor->setCurrentProgram(index);params.clear();
    for(const auto& b:h.bindings)params.set(b.parameter->paramID,static_cast<double>(b.raw->load()));
    const char* mixId=type==FxType::Delay?"scoreMix":type==FxType::StereoProcessor?"v2Mix":type==FxType::GateExpander?"v2_mix":"mix";
    if(rackMix)*rackMix=params.contains(mixId)?static_cast<float>(params[mixId]):1.f;
    const auto set=[&](const char* id,double value){if(params.contains(id))params.set(id,value);};
    set("mix",1);set("bypass",0);set("v2Mix",1);set("v2_mix",1);
    if(type==FxType::Reverb){set("routing",1);set("keepDry",0);}
    if(type==FxType::Delay){set("scoreRouting",1);set("scoreMix",1);}
    Values checked;
    if(!describe(type,params,48000,checked,error))return false;
    structural.reset();
    if(type==FxType::MonstaFx)h.processor->getStateInformation(structural);
    return true;
}
const std::vector<Program>& programs(FxType type)
{
    static std::map<FxType,std::vector<Program>> cache;
    auto found=cache.find(type);if(found!=cache.end())return found->second;
    auto h=create(type);std::vector<Program> out;
    for(int i=0;i<h.processor->getNumPrograms();++i)
    {
        h.processor->setCurrentProgram(i);
        const auto raw=[&](const char* id,float fallback){for(const auto& b:h.bindings)if(b.parameter->paramID==id)return b.raw->load();return fallback;};
        if(i>0&&((type==FxType::Delay&&(raw("echoMode",0)!=1||raw("quickEnabled",0)!=0))
            ||(type==FxType::Reverb&&raw("engine",0)<1)||(type==FxType::Gain&&raw("engine",0)<1)
            ||(type==FxType::GateExpander&&raw("v2_engine",0)<1)||(type==FxType::StereoProcessor&&raw("v2Mode",0)<1)))continue;
        out.push_back({i,h.processor->getProgramName(i)});
    }
    return cache.emplace(type,std::move(out)).first->second;
}

juce::Optional<juce::AudioPlayHead::PositionInfo> Runtime::PlayHead::getPosition() const
{
    PositionInfo p;p.setBpm(bpm);p.setIsPlaying(playing);
    if(position)p.setPpqPosition(ppq);
    return p;
}
bool validStructuralState(FxType kind,const juce::MemoryBlock& state,juce::String& error)
{
    if(state.getSize()==0)return true;
    const auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));
    if(xml==nullptr||xml->getTagName()!=productName(kind)){error="Invalid native structural state";return false;}
    if(kind==FxType::MonstaFx)
    {
        const auto* scene=xml->getChildByName("REVOLUTION");
        if(scene==nullptr||!scene->hasAttribute("timeA")||!scene->hasAttribute("glitchA")||!scene->hasAttribute("motionA"))
        {error="Incomplete Revolution scene state";return false;}
    }
    return true;
}

Runtime::Runtime(FxType kind,const juce::MemoryBlock& structuralState,const juce::NamedValueSet& params,
                 double sampleRate,int maximumBlockSize,juce::String& error,int channels,bool right)
    :handle(create(kind)),type(kind),maxBlock(std::max(1,maximumBlockSize)),rate(sampleRate)
{
    rightChannel=right&&kind==FxType::Gain;
    const auto indexOf=[&](const char* id) {for(size_t i=0;i<handle.bindings.size();++i)if(handle.bindings[i].parameter->paramID==id)return static_cast<int>(i);return -1;};
    for(int i=0;i<static_cast<int>(handle.bindings.size());++i)sourceIndex[static_cast<size_t>(i)]=i;
    if(rightChannel)
    {
        for(const auto& pair:{std::pair<const char*,const char*>{"leftDb","rightDb"},{"leftDelayMs","rightDelayMs"}})
        {const int to=indexOf(pair.first),from=indexOf(pair.second);if(to>=0&&from>=0)sourceIndex[static_cast<size_t>(to)]=from;}
        phaseIndex=indexOf("phase");rightPhaseIndex=indexOf("rightPhase");polarityIndex=indexOf("polarity");
    }
    Values values;
    if(!describe(type,params,rate,values,error))return;
    if(structuralState.getSize()>0)
    {
        if(!validStructuralState(type,structuralState,error))return;
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
        const auto& spec=layout(type)[static_cast<size_t>(i)];
        const auto* supplied=params.getVarPointer(spec.id);
        const float physical=supplied!=nullptr?static_cast<float>(*supplied):spec.initial;
        p->setValueNotifyingHost(p->convertTo0to1(physical));
    }
    handle.processor->setPlayHead(&playHead);
    handle.processor->setPlayConfigDetails(channels,channels,rate,maxBlock);
    handle.processor->prepareToPlay(rate,maxBlock);
    reportedLatency=handle.processor->getLatencySamples();
    if(reportedLatency!=values.latency){error="Native fixed-latency contract mismatch";return;}
    reportedTail.store(handle.processor->getTailLengthSeconds(),std::memory_order_relaxed);
    ready=true;
}

Runtime::~Runtime(){handle.processor->setPlayHead(nullptr);handle.processor->releaseResources();}
double Runtime::tailSeconds() const noexcept{return reportedTail.load(std::memory_order_relaxed);}

void Runtime::process(float* const* channels,int count,int samples,const Values& values,
                      double bpm,bool playing,bool hasPosition,double ppq) noexcept
{
    jassert(ready && values.count==static_cast<int>(handle.bindings.size()));
    double inputEnergy=0,outputEnergy=0;float reduction=0;
    for(int ch=0;ch<count;++ch)for(int i=0;i<samples;++i)inputEnergy+=static_cast<double>(channels[ch][i])*channels[ch][i];
    for(int i=0;i<values.count;++i)
    {
        float value=values.physical[static_cast<size_t>(sourceIndex[static_cast<size_t>(i)])];
        if(rightChannel&&i==phaseIndex&&rightPhaseIndex>=0)
        {
            const double phase=static_cast<double>(value)+values.physical[static_cast<size_t>(rightPhaseIndex)];
            value=static_cast<float>(phase-std::floor(phase));
        }
        if(rightChannel&&i==polarityIndex)value=(static_cast<int>(value)&2)?1.f:0.f;
        handle.bindings[static_cast<size_t>(i)].raw->store(value,std::memory_order_relaxed);
    }
    for(int offset=0;offset<samples;offset+=maxBlock)
    {
        float* ptr[2]{channels[0]+offset,count>1?channels[1]+offset:nullptr};
        const int n=std::min(maxBlock,samples-offset);
        playHead.bpm=bpm;playHead.playing=playing;playHead.position=hasPosition;
        playHead.ppq=ppq+offset*bpm/(60.0*rate);
        juce::AudioBuffer<float> block(ptr,count,n);
        midi.clear();handle.processor->processBlock(block,midi);
        if(type==FxType::Compressor)reduction=std::max(reduction,reductionComp(*handle.processor));
        if(type==FxType::Limiter)reduction=std::max(reduction,reductionRedWall(*handle.processor));
    }
    for(int ch=0;ch<count;++ch)for(int i=0;i<samples;++i)outputEnergy+=static_cast<double>(channels[ch][i])*channels[ch][i];
    const double divisor=std::max(1,count*samples);
    inputMeter.store(static_cast<float>(10*std::log10(std::max(1.e-12,inputEnergy/divisor))),std::memory_order_relaxed);
    outputMeter.store(static_cast<float>(10*std::log10(std::max(1.e-12,outputEnergy/divisor))),std::memory_order_relaxed);
    reductionMeter.store(reduction,std::memory_order_relaxed);
    if(type==FxType::Reverb||type==FxType::Delay)
        reportedTail.store(handle.processor->getTailLengthSeconds(),std::memory_order_relaxed);
}

juce::MemoryBlock Runtime::state()
{
    jassert(inUse()==0);
    juce::MemoryBlock block;if(inUse()==0)handle.processor->getStateInformation(block);return block;
}
} // namespace hpeq::native
