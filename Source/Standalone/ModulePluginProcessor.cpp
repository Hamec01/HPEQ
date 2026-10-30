#include "Standalone/ModulePluginProcessor.h"
#include "Core/ModulePresets.h"

#include "Standalone/ModulePluginEditor.h"
#include <cstring>
#include <limits>

namespace hpeq::standalone
{
namespace
{
constexpr double kBypassRampSeconds = 0.02;
}

ModulePluginProcessor::ModulePluginProcessor(ModuleKind moduleKind)
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      kind(moduleKind),
      parameters(*this, nullptr, juce::Identifier(moduleSpec(kind).productName), createParameterLayout(kind))
{
    rawValues.reserve(getSpec().parameters.size());
    for (const auto& spec : getSpec().parameters)
    {
        auto* value = parameters.getRawParameterValue(spec.id);
        jassert(value != nullptr);
        rawValues.push_back(value);
    }
    bypassParameter = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("bypass"));
    jassert(bypassParameter != nullptr);
    for(const auto& spec:scoreParameterSpecs())scoreValues.push_back(parameters.getRawParameterValue(spec.id));
    lastLegacy=currentSlot(true);activeLegacy=lastLegacy;lastScore=readScoreSettings(scoreValues,120);
    loadedHash.store(parameterHash(),std::memory_order_relaxed);
}

void ModulePluginProcessor::prepareToPlay(double sampleRate, int expectedBlockSize)
{
    maximumBlockSize = juce::jmax(1, expectedBlockSize);
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    engine.prepare(sampleRate, maximumBlockSize);
    for(auto& score:scoreEngines)score.prepare(sampleRate,maximumBlockSize);
    for(auto& work:pathWork)work.setSize(dsp::kMaxChannels,maximumBlockSize,false,false,true);

    fixedLatency = maximumLatencySamples(kind, sampleRate);
    wetAlign.prepare(fixedLatency);
    dryAlign.prepare(fixedLatency);
    dry.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    bypassRampSteps = dsp::stepsFor(kBypassRampSeconds, sampleRate);
    wetAmount.reset(isModuleOn() ? 1.0 : 0.0);
    setLatencySamples(fixedLatency);

    engine.setTargets(currentSlot(isModuleOn()));
    if(scoreValues[scoreparam::Mode]->load(std::memory_order_relaxed)>.5f)(void)currentTransport();
    lastLegacy=activeLegacy=currentSlot(isModuleOn());lastScore=readScoreSettings(scoreValues,getHostBpm());
    lastMode=scoreValues[scoreparam::Mode]->load(std::memory_order_relaxed)>.5f?1:0;
    activePath=lastMode?0:-1;retiringPath=-2;forcedRetire=0;retireAge=retireLimit=0;
    hasRetiringTail.store(false,std::memory_order_relaxed);
    if(activePath>=0){scoreConfigs[static_cast<size_t>(activePath)]=lastScore;scoreEngines[static_cast<size_t>(activePath)].start(lastScore);}
    incomingWeight.reset(1);retiringWeight.reset(0);scoreDry.reset(lastMode?(lastScore.routing==1?0:1-lastScore.mix):1);
    restorePending.store(false,std::memory_order_release);
}

bool ModulePluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

dsp::FxSlotParams ModulePluginProcessor::currentSlot(bool active) const noexcept
{
    return makeSlot(kind, rawValues, active);
}

dsp::Transport ModulePluginProcessor::currentTransport() noexcept
{
    dsp::Transport transport;
    transport.bpm = hostBpm.load(std::memory_order_relaxed);
    if (auto* hostPlayHead = getPlayHead())
        if (const auto position = hostPlayHead->getPosition())
        {
            if (const auto bpm = position->getBpm(); bpm.hasValue() && *bpm > 0.0)
            {
                transport.bpm = *bpm;
                hostBpm.store(*bpm, std::memory_order_relaxed);
            }
            transport.playing = position->getIsPlaying();
            if (const auto ppq = position->getPpqPosition(); ppq.hasValue())
            {
                transport.hasPosition = true;
                transport.ppqAtBlockStart = *ppq;
            }
        }
    return transport;
}

void ModulePluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int inputs = getTotalNumInputChannels();
    const int outputs = getTotalNumOutputChannels();
    const int samples = buffer.getNumSamples();
    for (int channel = inputs; channel < outputs; ++channel)
        buffer.clear(channel, 0, samples);
    if (samples <= 0)
        return;

    const int channels = juce::jmin(inputs, outputs, buffer.getNumChannels(), dsp::kMaxChannels);
    const bool on = isModuleOn();
    wetAmount.setTarget(on ? 1.0 : 0.0, bypassRampSteps);

    const auto transport=currentTransport();
    const auto revision=bulkRevision.load(std::memory_order_acquire);
    if((revision&1)==0)
    {
        const auto legacy=currentSlot(on);const auto score=readScoreSettings(scoreValues,transport.bpm);
        const int mode=scoreValues[scoreparam::Mode]->load(std::memory_order_relaxed)>.5f?1:0;
        if(revision==bulkRevision.load(std::memory_order_acquire)){lastLegacy=legacy;lastScore=score;lastMode=mode;}
    }
    if(restorePending.exchange(false,std::memory_order_acq_rel))
    {
        engine.reset();engine.setTargets(lastLegacy);activeLegacy=lastLegacy;
        activePath=lastMode?0:-1;retiringPath=-2;forcedRetire=0;
        hasRetiringTail.store(false,std::memory_order_relaxed);
        if(activePath>=0){scoreConfigs[0]=lastScore;scoreEngines[0].start(lastScore);}
        incomingWeight.reset(1);retiringWeight.reset(0);scoreDry.reset(lastMode?(lastScore.routing==1?0:1-lastScore.mix):1);
        wetAmount.reset(on?1:0);
    }
    if(lastMode!=0||activePath!=-1||retiringPath!=-2||incomingWeight.isRamping())
    {
        processScorePaths(buffer,channels,samples,transport,lastMode,lastScore,lastLegacy);
        return;
    }

    activeLegacy=lastLegacy;
    engine.setTargets(lastLegacy);
    const int extraWetDelay = juce::jmax(0, fixedLatency - engine.getLatencySamples());

    for (int start = 0; start < samples; start += maximumBlockSize)
    {
        const int count = juce::jmin(maximumBlockSize, samples - start);
        float* wet[dsp::kMaxChannels] {};
        float* dryPointers[dsp::kMaxChannels] {};
        for (int channel = 0; channel < channels; ++channel)
        {
            wet[channel] = buffer.getWritePointer(channel, start);
            dryPointers[channel] = dry.getWritePointer(channel);
            juce::FloatVectorOperations::copy(dryPointers[channel], wet[channel], count);
        }

        auto chunkTransport = transport;
        if (chunkTransport.hasPosition && start > 0)
            chunkTransport.ppqAtBlockStart += static_cast<double>(start) * chunkTransport.bpm
                                               / (60.0 * currentSampleRate.load(std::memory_order_relaxed));
        engine.process(wet, channels, count, chunkTransport);
        // Always clock the aligner, including at zero delay, so a later automated latency change
        // can draw from continuous history instead of newly-cleared memory.
        wetAlign.process(wet, wet, channels, count, extraWetDelay);
        dryAlign.process(dryPointers, dryPointers, channels, count, fixedLatency);

        for (int i = 0; i < count; ++i)
        {
            const float amount = static_cast<float>(wetAmount.next());
            for (int channel = 0; channel < channels; ++channel)
                wet[channel][i] = dryPointers[channel][i] + amount * (wet[channel][i] - dryPointers[channel][i]);
        }
    }
}

void ModulePluginProcessor::choosePath(int mode,const hamlo::echo::ScoreSettings& requested,const dsp::FxSlotParams& legacy) noexcept
{
    const bool change=mode==0?activePath!=-1:(activePath<0||hamlo::echo::ScoreEngine::structuralChange(scoreConfigs[static_cast<size_t>(activePath)],requested));
    if(change)
    {
        if(retiringPath!=-2)
        {
            if(forcedRetire==0){forcedRetire=dsp::stepsFor(.05,currentSampleRate.load(std::memory_order_relaxed));retiringWeight.setTarget(0,forcedRetire);}
            return;
        }
        retiringPath=activePath;retireAge=0;retiringWeight.reset(1);
        hasRetiringTail.store(true,std::memory_order_relaxed);
        double tail=0;
        if(retiringPath<0)
        {
            retiringLegacy=activeLegacy;retiringLegacy.active=false;retiringLegacy.values[dsp::slot::kDlyFreeze]=0;
            tail=dsp::DelayProcessor::tailSeconds(retiringLegacy,getHostBpm());
        }
        else {scoreEngines[static_cast<size_t>(retiringPath)].releaseFreeze();tail=scoreEngines[static_cast<size_t>(retiringPath)].tailSeconds();}
        retireLimit=static_cast<int64_t>(std::ceil(std::clamp(tail,.05,120.)*currentSampleRate.load(std::memory_order_relaxed)));
        if(mode==0){activePath=-1;engine.reset();activeLegacy=legacy;engine.setTargets(legacy);}
        else
        {
            activePath=retiringPath>=0?1-retiringPath:0;
            scoreConfigs[static_cast<size_t>(activePath)]=requested;scoreEngines[static_cast<size_t>(activePath)].start(requested);
        }
        incomingWeight.reset(0);incomingWeight.setTarget(1,bypassRampSteps);
    }
    else if(activePath>=0)
    {scoreConfigs[static_cast<size_t>(activePath)]=requested;scoreEngines[static_cast<size_t>(activePath)].configure(requested);}
    else {activeLegacy=legacy;engine.setTargets(legacy);}
    const double target=activePath<0?1:(requested.routing==1?0:1-requested.mix);
    scoreDry.setTarget(target,bypassRampSteps);
}

void ModulePluginProcessor::processScorePaths(juce::AudioBuffer<float>& buffer,int channels,int samples,const dsp::Transport& transport,
                                             int mode,const hamlo::echo::ScoreSettings& requested,const dsp::FxSlotParams& legacy) noexcept
{
    int start=0;double wetEnergy=0;
    while(start<samples)
    {
        choosePath(mode,requested,legacy);
        int count=juce::jmin(maximumBlockSize,samples-start);
        if(forcedRetire>0)count=juce::jmin(count,forcedRetire);
        if(retiringPath!=-2&&retireAge<retireLimit)
            count=juce::jmin(count,static_cast<int>(std::min<int64_t>(retireLimit-retireAge,maximumBlockSize)));
        for(int ch=0;ch<channels;++ch)
        {
            dry.copyFrom(ch,0,buffer,ch,start,count);
            for(auto& work:pathWork)work.copyFrom(ch,0,buffer,ch,start,count);
        }
        auto chunk=transport;if(chunk.hasPosition)chunk.ppqAtBlockStart+=start*chunk.bpm/(60*currentSampleRate.load(std::memory_order_relaxed));
        const auto run=[&](int path,int work,bool accepting)
        {
            auto& b=pathWork[static_cast<size_t>(work)];
            if(path<0)
            {
                engine.setTargets(accepting?legacy:retiringLegacy);
                engine.process(b.getArrayOfWritePointers(),channels,count,chunk);
                for(int ch=0;ch<channels;++ch)for(int i=0;i<count;++i)b.getWritePointer(ch)[i]-=dry.getReadPointer(ch)[i];
            }
            else scoreEngines[static_cast<size_t>(path)].process(b.getArrayOfWritePointers(),channels,count,accepting&&isModuleOn(),true);
        };
        run(activePath,0,true);
        if(retiringPath!=-2)run(retiringPath,1,false);else pathWork[1].clear();
        for(int i=0;i<count;++i)
        {
            const double incoming=incomingWeight.next(),outgoing=retiringPath!=-2?retiringWeight.next():0,d=scoreDry.next();
            const float bypass=static_cast<float>(wetAmount.next());
            for(int ch=0;ch<channels;++ch)
            {
                const float x=dry.getReadPointer(ch)[i];
                const double wet=incoming*pathWork[0].getReadPointer(ch)[i]+outgoing*pathWork[1].getReadPointer(ch)[i];wetEnergy+=wet*wet;
                const float y=static_cast<float>(d*x+incoming*pathWork[0].getReadPointer(ch)[i]+outgoing*pathWork[1].getReadPointer(ch)[i]);
                buffer.getWritePointer(ch,start)[i]=bypass==0?x:x+bypass*(y-x);
            }
            if(retiringPath!=-2)
            {
                ++retireAge;
                if(forcedRetire>0)--forcedRetire;
                if(forcedRetire==0&&retiringWeight.value()==0)retiringPath=-2;
                else if(retireAge>=retireLimit-dsp::stepsFor(.05,currentSampleRate.load(std::memory_order_relaxed))&&retiringWeight.targetValue()!=0)
                    retiringWeight.setTarget(0,dsp::stepsFor(.05,currentSampleRate.load(std::memory_order_relaxed)));
                if(retireAge>=retireLimit)retiringPath=-2;
            }
        }
        start+=count;
    }
    scoreMeter.store(static_cast<float>(10*std::log10(wetEnergy/(samples*channels)+1e-12)),std::memory_order_relaxed);
    hasRetiringTail.store(retiringPath!=-2,std::memory_order_relaxed);
}

double ModulePluginProcessor::getTailLengthSeconds() const
{
    if(scoreValues[scoreparam::Mode]->load(std::memory_order_relaxed)>.5f)
    {
        const auto settings=readScoreSettings(scoreValues,getHostBpm());
        if(settings.freeze)return std::numeric_limits<double>::infinity();
        return std::max(hasRetiringTail.load(std::memory_order_relaxed)?120.:0.,hamlo::echo::ScoreEngine::estimateTail(settings));
    }
    const auto slot = currentSlot(true);
    const double bpm = hostBpm.load(std::memory_order_relaxed);
    double tail = 0.0;
    tail = dsp::DelayProcessor::tailSeconds(slot, bpm);
    return std::max(hasRetiringTail.load(std::memory_order_relaxed)?120.:0.,tail + static_cast<double>(fixedLatency) / currentSampleRate.load(std::memory_order_relaxed));
}

juce::AudioProcessorEditor* ModulePluginProcessor::createEditor()
{
    return new ModulePluginEditor(*this);
}

void ModulePluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    const juce::ScopedLock lock(stateWriter);
    auto state=parameters.copyState();state.setProperty("selectedProgram",getCurrentProgram(),nullptr);
    state.setProperty("recipeHash",juce::String::toHexString(static_cast<juce::int64>(loadedHash.load(std::memory_order_relaxed))),nullptr);
    state.setProperty("scoreSchema",1,nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}

void ModulePluginProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType().toString()))
        {
            const juce::ScopedLock lock(stateWriter);bulkRevision.fetch_add(1,std::memory_order_acq_rel);
            auto state=juce::ValueTree::fromXml(*xml);
            for(const auto& spec:scoreParameterSpecs())
                if(!state.getChildWithProperty("id",spec.id).isValid())
                {juce::ValueTree child("PARAM");child.setProperty("id",spec.id,nullptr);child.setProperty("value",spec.defaultValue,nullptr);state.addChild(child,-1,nullptr);}
            parameters.replaceState(state);
            currentProgram.store(juce::jlimit(0,getNumPrograms()-1,static_cast<int>(state.getProperty("selectedProgram",0))),std::memory_order_relaxed);
            const auto baseline=state.getProperty("recipeHash").toString();loadedHash.store(baseline.isNotEmpty()?static_cast<uint64_t>(baseline.getHexValue64()):parameterHash(),std::memory_order_relaxed);
            restorePending.store(true,std::memory_order_release);bulkRevision.fetch_add(1,std::memory_order_release);
        }
}

uint64_t ModulePluginProcessor::parameterHash() const noexcept
{
    uint64_t hash=1469598103934665603ull;
    const auto add=[&](float value){uint32_t bits=0;std::memcpy(&bits,&value,sizeof(bits));hash=(hash^bits)*1099511628211ull;};
    for(auto* p:rawValues)add(p->load(std::memory_order_relaxed));
    for(auto* p:scoreValues)add(p->load(std::memory_order_relaxed));
    return hash;
}
bool ModulePluginProcessor::presetModified() const noexcept{return loadedHash.load(std::memory_order_relaxed)!=parameterHash();}

juce::RangedAudioParameter* ModulePluginProcessor::parameter(const juce::String& id) const noexcept
{
    return parameters.getParameter(id);
}

double ModulePluginProcessor::parameterValue(const juce::String& id) const noexcept
{
    if (const auto* p = parameter(id))
        return p->convertFrom0to1(p->getValue());
    return 0.0;
}

void ModulePluginProcessor::setParameterValue(const juce::String& id, double actualValue)
{
    if (auto* p = parameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(actualValue)));
}

int ModulePluginProcessor::getNumPrograms()
{
    return 1 + static_cast<int>(presets::factoryModulePresets(getSpec().type).size());
}

const juce::String ModulePluginProcessor::getProgramName(int index)
{
    const auto& list = presets::factoryModulePresets(getSpec().type);
    if (index <= 0 || index > static_cast<int>(list.size()))
        return "Default";
    return list[static_cast<size_t>(index - 1)].name;
}

void ModulePluginProcessor::setCurrentProgram(int index)
{
    const juce::ScopedLock lock(stateWriter);bulkRevision.fetch_add(1,std::memory_order_acq_rel);
    const bool locked=parameterValue("scoreMixLock")>.5;const double oldMix=parameterValue("mix"),scoreMix=parameterValue("scoreMix"),routing=parameterValue("scoreRouting");
    const auto& list = presets::factoryModulePresets(getSpec().type);
    const int selected=juce::jlimit(0,static_cast<int>(list.size()),index);currentProgram.store(selected,std::memory_order_relaxed);
    for (const auto& spec : getSpec().parameters)
    {
        double value = spec.defaultValue;
        if (selected > 0)
            value = presets::presetValue(list[static_cast<size_t>(selected - 1)], spec.id, value);
        setParameterValue(spec.id, value);
    }
    for(const auto& spec:scoreParameterSpecs())
        setParameterValue(spec.id,selected>0?presets::presetValue(list[static_cast<size_t>(selected-1)],spec.id,spec.defaultValue):spec.defaultValue);
    setParameterValue("scoreMixLock",locked?1:0);setParameterValue("scoreRouting",routing);
    if(locked){setParameterValue("mix",oldMix);setParameterValue("scoreMix",scoreMix);}
    loadedHash.store(parameterHash(),std::memory_order_relaxed);bulkRevision.fetch_add(1,std::memory_order_release);
}

} // namespace hpeq::standalone
