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

Handle create(FxType type,bool realtime)
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
        case FxType::MonstaFx:return createRevolution(realtime);
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
    // Original plug-in controls stay functional, including MIX, BYPASS and
    // insert/send routing. Only an explicitly wet-only native output is added
    // to a dry reference; a complete native insert is crossfaded by rack MIX.
    values.additiveSend=actual("bypass")==0 &&
        ((type==FxType::Reverb&&actual("engine")>=1&&actual("routing")==1)
        ||(type==FxType::Delay&&actual("echoMode")==1&&actual("scoreRouting")==1));
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

bool loadProgram(FxType type,int index,juce::NamedValueSet& params,juce::MemoryBlock& structural,juce::String& error,float* rackMix,bool completePlugin)
{
    auto h=create(type);
    if(index<0||index>=h.processor->getNumPrograms()){error="Unknown native program";return false;}
    if(completePlugin)
    {
        h.processor->setCurrentProgram(index);params.clear();
        for(const auto& b:h.bindings)params.set(b.parameter->paramID,static_cast<double>(b.raw->load()));
        if(rackMix)*rackMix=1.f;
        h.processor->getStateInformation(structural);Values checked;
        return describe(type,params,48000,checked,error);
    }
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
    :handle(create(kind,true)),type(kind),maxBlock(std::max(1,maximumBlockSize)),rate(sampleRate)
{
    rightChannel=right&&kind==FxType::Gain;
    const auto indexOf=[&](const char* id) {for(size_t i=0;i<handle.bindings.size();++i)if(handle.bindings[i].parameter->paramID==id)return static_cast<int>(i);return -1;};
    if(kind==FxType::Reverb||kind==FxType::Delay)
    {
        returnEngineIndex=indexOf(kind==FxType::Reverb?"engine":"echoMode");
        returnRoutingIndex=indexOf(kind==FxType::Reverb?"routing":"scoreRouting");
        returnBypassIndex=indexOf("bypass");
    }
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
    if(maximumBlockSize>0){handle.processor->prepareToPlay(rate,maxBlock);prepared=true;}
    reportedLatency=prepared?handle.processor->getLatencySamples():values.latency;
    if(reportedLatency!=values.latency){error="Native fixed-latency contract mismatch";return;}
    reportedTail.store(handle.processor->getTailLengthSeconds(),std::memory_order_relaxed);
    ready=true;
}

Runtime::~Runtime(){handle.processor->setPlayHead(nullptr);handle.processor->releaseResources();}
void Runtime::reprepare(double sampleRate,int maximumBlockSize,int channels)
{
    jassert(inUse()==0);
    // The native processor owns editor/workbench state too. Keep that object
    // across host prepare calls, exactly as a standalone plug-in host does.
    if(prepared)handle.processor->releaseResources();rate=sampleRate;maxBlock=std::max(1,maximumBlockSize);
    handle.processor->setPlayConfigDetails(channels,channels,rate,maxBlock);
    handle.processor->prepareToPlay(rate,maxBlock);
    prepared=true;
    reportedLatency=handle.processor->getLatencySamples();
    if(reportedLatency!=fixedLatency(type,rate))throw std::runtime_error("Native reprepare latency mismatch");
    reportedTail.store(handle.processor->getTailLengthSeconds(),std::memory_order_relaxed);
}
double Runtime::tailSeconds() const noexcept{return reportedTail.load(std::memory_order_relaxed);}

bool Runtime::process(float* const* channels,int count,int samples,const Values& values,
                      double bpm,bool playing,bool hasPosition,double ppq) noexcept
{
    jassert(ready && prepared && values.count==static_cast<int>(handle.bindings.size()));
    double inputEnergy=0,outputEnergy=0;float reduction=0;
    for(int ch=0;ch<count;++ch)for(int i=0;i<samples;++i)inputEnergy+=static_cast<double>(channels[ch][i])*channels[ch][i];
    for(int i=0;i<values.count;++i)
    {
        const auto source=static_cast<size_t>(sourceIndex[static_cast<size_t>(i)]);
        float value=values.physical[source];
        auto serial=editorSerial[source].load(std::memory_order_acquire);
        if(serial!=0)
        {
            const auto edited=editorValues[source].load(std::memory_order_relaxed);
            if(value==edited)editorSerial[source].compare_exchange_strong(serial,0,std::memory_order_acq_rel);
            value=edited;
        }
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
    return returnTarget();
}
bool Runtime::returnTarget() const noexcept
{
    const auto raw=[&](int index){return index>=0?handle.bindings[static_cast<size_t>(index)].raw->load(std::memory_order_relaxed):0.f;};
    return returnEngineIndex>=0&&raw(returnEngineIndex)>=1&&raw(returnRoutingIndex)==1&&raw(returnBypassIndex)<.5f;
}

juce::MemoryBlock Runtime::state()
{
    jassert(inUse()==0);
    juce::MemoryBlock block;if(inUse()==0)handle.processor->getStateInformation(block);return block;
}
void Runtime::requestEditorValue(int index,float physical) noexcept
{
    if(index<0||index>=static_cast<int>(handle.bindings.size()))return;
    const auto& range=handle.bindings[static_cast<size_t>(index)].parameter->getNormalisableRange();
    for(int pass=0;pass<2;++pass)physical=range.snapToLegalValue(range.convertFrom0to1(range.convertTo0to1(range.snapToLegalValue(physical))));
    editorValues[static_cast<size_t>(index)].store(physical,std::memory_order_relaxed);
    editorSerial[static_cast<size_t>(index)].store(nextEditorSerial.fetch_add(1,std::memory_order_relaxed),std::memory_order_release);
}
void Runtime::applyPendingEditorValues(juce::NamedValueSet& values) const
{
    for(size_t i=0;i<handle.bindings.size();++i)if(editorSerial[i].load(std::memory_order_acquire)!=0)
        values.set(handle.bindings[i].parameter->paramID,static_cast<double>(editorValues[i].load(std::memory_order_relaxed)));
}
juce::MemoryBlock Runtime::captureEditorState()
{
    juce::MemoryBlock bytes;handle.processor->getStateInformation(bytes);return bytes;
}
void Runtime::synchronizeEditor(const Values& values,bool preservePending)
{
    editorSynchronizing.store(true,std::memory_order_release);
    for(int i=0;i<values.count;++i)
    {
        auto& b=handle.bindings[static_cast<size_t>(i)];
        const auto value=values.physical[static_cast<size_t>(i)];
        auto serial=editorSerial[static_cast<size_t>(i)].load(std::memory_order_acquire);
        if(preservePending&&serial!=0)continue;
        if(serial!=0&&editorValues[static_cast<size_t>(i)].load(std::memory_order_relaxed)!=value)
            editorSerial[static_cast<size_t>(i)].compare_exchange_strong(serial,0,std::memory_order_acq_rel);
        if(b.raw->load(std::memory_order_relaxed)!=value)
            b.parameter->setValueNotifyingHost(b.parameter->convertTo0to1(value));
    }
    editorSynchronizing.store(false,std::memory_order_release);
}
} // namespace hpeq::native
