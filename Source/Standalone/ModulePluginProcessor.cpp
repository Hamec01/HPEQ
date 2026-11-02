#include "Standalone/ModulePluginProcessor.h"
#include "Core/ModulePresets.h"

#include "Standalone/ModulePluginEditor.h"
#include "Standalone/V2Parameters.h"
#include "S4Identity.h"

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
    if(juce::String(dsp::s4SourceIdentity())!=SHIROTA_S4_SOURCE_ID)throw std::runtime_error("Mixed S4 production translation units");
    const auto& additions=v2Parameters();if(additions.size()!=modernRaw.size())throw std::runtime_error("S4 control layout mismatch");
    for(size_t i=0;i<modernRaw.size();++i)modernRaw[i]=parameters.getRawParameterValue(additions[i].id);
    captureProgramAnchor();
}

void ModulePluginProcessor::prepareToPlay(double sampleRate, int expectedBlockSize)
{
    maximumBlockSize = juce::jmax(1, expectedBlockSize);
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    engine.prepare(sampleRate, maximumBlockSize);

    fixedLatency = maximumLatencySamples(kind, sampleRate);
    wetAlign.prepare(fixedLatency);
    dryAlign.prepare(fixedLatency);
    dry.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    bypassRampSteps = dsp::stepsFor(kBypassRampSeconds, sampleRate);
    wetAmount.reset(isModuleOn() ? 1.0 : 0.0);
    setLatencySamples(fixedLatency);

    engine.setTargets(currentSlot(isModuleOn()));
    modern.prepare(sampleRate,maximumBlockSize);modern.targets(modernControls(),isModuleOn());
    modernBuffer.setSize(2,maximumBlockSize,false,false,true);modeSteps=dsp::stepsFor(.05,sampleRate);warmSteps=dsp::stepsFor(.015,sampleRate);
    previousModern=modernRaw[dsp::v2::Mode]->load()>.5f;modernAmount.reset(previousModern?1:0);modernWarm=0;
}

bool ModulePluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return (input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo()))
        || (input==juce::AudioChannelSet::mono()&&output==juce::AudioChannelSet::stereo());
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
    if(samples<=0||buffer.getNumChannels()<=0)return;
    for (int channel = inputs; channel < juce::jmin(outputs,buffer.getNumChannels()); ++channel)
        if(inputs==1&&outputs==2)buffer.copyFrom(channel,0,buffer,0,0,samples);else buffer.clear(channel, 0, samples);
    if (samples <= 0)
        return;

    const int channels = juce::jmin(outputs, buffer.getNumChannels(), dsp::kMaxChannels);
    if(channels<=0)return;
    const bool on = isModuleOn();
    wetAmount.setTarget(on ? 1.0 : 0.0, bypassRampSteps);

    engine.setTargets(currentSlot(on));
    const int extraWetDelay = juce::jmax(0, fixedLatency - engine.getLatencySamples());
    const auto transport = currentTransport();
    const auto controls=modernControls();const bool wantModern=controls[dsp::v2::Mode]>.5f;
    modern.targets(controls,on);
    if(!wantModern){modernWarm=0;modernAmount.setTarget(0,modeSteps);}
    else if(!previousModern)
    {if(modernAmount.value()==0){modern.clearHistory();modernWarm=warmSteps;}else modernAmount.setTarget(1,modeSteps);}
    previousModern=wantModern;

    for (int start = 0; start < samples; start += maximumBlockSize)
    {
        const int count = juce::jmin(maximumBlockSize, samples - start);
        const bool runModern=wantModern||modernAmount.value()>0||modernWarm>0;
        if(runModern)
        {
            for(int ch=0;ch<2;++ch)modernBuffer.copyFrom(ch,0,buffer,juce::jmin(ch,channels-1),start,count);
            modern.process(modernBuffer.getWritePointer(0),modernBuffer.getWritePointer(1),count,transport.bpm);
        }
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
            if(modernWarm>0&&--modernWarm==0&&wantModern)modernAmount.setTarget(1,modeSteps);
            const float blend=static_cast<float>(modernAmount.next());
            if(blend!=0&&runModern)for(int ch=0;ch<channels;++ch)
            {
                const float value=channels==1?.5f*(modernBuffer.getSample(0,i)+modernBuffer.getSample(1,i)):modernBuffer.getSample(ch,i);
                wet[ch][i]=blend==1?value:wet[ch][i]+blend*(value-wet[ch][i]);
            }
        }
    }
    if(wantModern){modernGuard.store(modern.guardDb(),std::memory_order_relaxed);modernPosition.store(modern.azimuth(),std::memory_order_relaxed);
        double ll=0,rr=0,lr=0;for(int i=0;i<samples;++i){const double l=buffer.getSample(0,i),r=buffer.getSample(channels>1?1:0,i);ll+=l*l;rr+=r*r;lr+=l*r;}modernCorrelation.store(ll*rr>1e-30?lr/std::sqrt(ll*rr):1,std::memory_order_relaxed);}
}

double ModulePluginProcessor::getTailLengthSeconds() const
{
    return (modernRaw[dsp::v2::Mode]->load()>.5f?3.0:0.0)+static_cast<double>(fixedLatency) / currentSampleRate.load(std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ModulePluginProcessor::createEditor()
{
    return new ModulePluginEditor(*this);
}

void ModulePluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state=parameters.copyState();bool neutral=true;
    for(const auto& spec:v2Parameters()){auto* p=parameter(spec.id);neutral=neutral&&p->getValue()==p->getDefaultValue();}
    if(neutral)
    {for(int i=state.getNumChildren()-1;i>=0;--i)if(state.getChild(i).getProperty("id").toString().startsWith("v2"))state.removeChild(i,nullptr);state.removeProperty("v2Program",nullptr);state.removeProperty("v2Modified",nullptr);}
    else{state.setProperty("v2Program",currentProgram,nullptr);state.setProperty("v2Modified",presetModified(),nullptr);}
    if(auto xml=state.createXml())copyXmlToBinary(*xml,destination);
}

void ModulePluginProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType().toString()))
        {
            for(const auto& spec:v2Parameters()){auto* p=parameter(spec.id);p->setValue(p->getDefaultValue());}
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
            currentProgram=juce::jlimit(0,getNumPrograms()-1,xml->getIntAttribute("v2Program",0));
            captureProgramAnchor();loadedModified.store(xml->getBoolAttribute("v2Modified",false));
        }
}

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
    const auto& list = presets::factoryModulePresets(getSpec().type);
    currentProgram = juce::jlimit(0, static_cast<int>(list.size()), index);
    for (const auto& spec : getSpec().parameters)
    {
        double value = spec.defaultValue;
        if (currentProgram > 0)
            value = presets::presetValue(list[static_cast<size_t>(currentProgram - 1)], spec.id, value);
        setParameterValue(spec.id, value);
    }
    for(const auto& spec:v2Parameters())setParameterValue(spec.id,spec.defaultValue);
    captureProgramAnchor();
}

dsp::v2::Controls ModulePluginProcessor::modernControls() const noexcept
{dsp::v2::Controls values{};for(size_t i=0;i<values.size();++i)values[i]=modernRaw[i]->load(std::memory_order_relaxed);return values;}
void ModulePluginProcessor::captureProgramAnchor() noexcept
{int i=0;for(auto* p:getParameters())programAnchor[static_cast<size_t>(i++)].store(p->getValue(),std::memory_order_relaxed);loadedModified.store(false);}
bool ModulePluginProcessor::presetModified() const noexcept
{if(loadedModified.load())return true;int i=0;for(auto* p:getParameters()){const auto anchor=programAnchor[static_cast<size_t>(i++)].load(std::memory_order_relaxed);const auto* named=dynamic_cast<const juce::AudioProcessorParameterWithID*>(p);if(named&&(named->paramID=="v2Page"||named->paramID=="v2EditNode"||named->paramID=="v2Restart"))continue;if(p->getValue()!=anchor)return true;}return false;}

} // namespace hpeq::standalone
