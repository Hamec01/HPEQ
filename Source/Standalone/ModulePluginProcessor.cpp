#include "Standalone/ModulePluginProcessor.h"
#include "Core/ModulePresets.h"

#include "ZastavaS1Identity.h"
#include <stdexcept>
#include "Standalone/ModulePluginEditor.h"

namespace hpeq::standalone
{
namespace
{
constexpr double kBypassRampSeconds = 0.02;
}

ModulePluginProcessor::ModulePluginProcessor(ModuleKind moduleKind)
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withInput("External Key", juce::AudioChannelSet::stereo(), false)
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
    if(juce::String(dsp::zastavaS1SourceIdentity())!=ZASTAVA_S1_SOURCE_ID)throw std::runtime_error("Mixed Zastava S1 translation units");
    bypassParameter = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("bypass"));
    jassert(bypassParameter != nullptr);
    for(size_t i=0;i<modern::Count;++i)modernValues[i]=parameters.getRawParameterValue(modern::specs()[i].id);
}

void ModulePluginProcessor::prepareToPlay(double sampleRate, int expectedBlockSize)
{
    maximumBlockSize = juce::jmax(1, expectedBlockSize);
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    engine.prepare(sampleRate, maximumBlockSize);
    modernEngine.prepare(sampleRate, maximumBlockSize);
    modernEngine.set(modern::settings(modernValues,!isModuleOn()));
    lastModernMode=std::max(0,modern::engine(modernValues)-1);
    modernAudio.setSize(dsp::kMaxChannels,maximumBlockSize,false,false,true);
    keyAudio.setSize(dsp::kMaxChannels,maximumBlockSize,false,false,true);

    fixedLatency = maximumLatencySamples(kind, sampleRate);
    wetAlign.prepare(fixedLatency);
    dryAlign.prepare(fixedLatency);
    dry.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    bypassRampSteps = dsp::stepsFor(kBypassRampSeconds, sampleRate);
    wetAmount.reset(isModuleOn() ? 1.0 : 0.0);
    modernAmount.reset(modern::engine(modernValues)>0?1:0);
    resetModern.store(false,std::memory_order_release);
    setLatencySamples(fixedLatency);

    engine.setTargets(currentSlot(isModuleOn()));
}

bool ModulePluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if(layouts.inputBuses.size()!=2||layouts.outputBuses.size()!=1)return false;
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    const auto key=layouts.inputBuses[1];
    return input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && (key.isDisabled()||key==juce::AudioChannelSet::mono()||key==juce::AudioChannelSet::stereo());
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
{process(buffer,false);}
void ModulePluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&)
{process(buffer,true);}
void ModulePluginProcessor::process(juce::AudioBuffer<float>& buffer,bool forcedBypass)
{
    juce::ScopedNoDenormals noDenormals;
    const int inputs = getMainBusNumInputChannels();
    const int outputs = getMainBusNumOutputChannels();
    const int samples = buffer.getNumSamples();
    for (int channel = inputs; channel < outputs; ++channel)
        buffer.clear(channel, 0, samples);
    if (samples <= 0)
        return;

    const int channels = juce::jmin(inputs, outputs, buffer.getNumChannels(), dsp::kMaxChannels);
    const bool on = !forcedBypass&&isModuleOn();
    const bool modernSelected=modern::engine(modernValues)>0;
    if(resetModern.exchange(false,std::memory_order_acq_rel))
    {
        modernEngine.reset();modernAmount.reset(modernSelected?1:0);lastModernMode=std::max(0,modern::engine(modernValues)-1);
    }
    modernAmount.setTarget(modernSelected?1:0,bypassRampSteps);
    if(modernSelected)lastModernMode=modern::engine(modernValues)-1;
    modernEngine.set(modern::settings(modernValues,!on,lastModernMode));
    wetAmount.setTarget(on ? 1.0 : 0.0, bypassRampSteps);

    engine.setTargets(currentSlot(on));
    const int extraWetDelay = juce::jmax(0, fixedLatency - engine.getLatencySamples());
    const auto transport = currentTransport();

    for (int start = 0; start < samples; start += maximumBlockSize)
    {
        const int count = juce::jmin(maximumBlockSize, samples - start);
        float* wet[dsp::kMaxChannels] {};
        float* dryPointers[dsp::kMaxChannels] {};
        float* modernPointers[dsp::kMaxChannels] {};
        const float* keyPointers[dsp::kMaxChannels] {};
        const int keyChannels=getChannelCountOfBus(true,1);
        for (int channel = 0; channel < channels; ++channel)
        {
            wet[channel] = buffer.getWritePointer(channel, start);
            dryPointers[channel] = dry.getWritePointer(channel);
            juce::FloatVectorOperations::copy(dryPointers[channel], wet[channel], count);
            modernPointers[channel]=modernAudio.getWritePointer(channel);
            juce::FloatVectorOperations::copy(modernPointers[channel],wet[channel],count);
        }
        for(int c=0;c<keyChannels;++c)
            if(inputs+c<buffer.getNumChannels())
            {
                // A host may alias main and key input pointers. Preserve the key
                // before either prepared path writes the main output in place.
                juce::FloatVectorOperations::copy(keyAudio.getWritePointer(c),buffer.getReadPointer(inputs+c,start),count);
                keyPointers[c]=keyAudio.getReadPointer(c);
            }

        auto chunkTransport = transport;
        if (chunkTransport.hasPosition && start > 0)
            chunkTransport.ppqAtBlockStart += static_cast<double>(start) * chunkTransport.bpm
                                               / (60.0 * currentSampleRate.load(std::memory_order_relaxed));
        engine.process(wet, channels, count, chunkTransport);
        hamlo::zastava::Clock clock;clock.bpm=chunkTransport.bpm;clock.ppq=chunkTransport.ppqAtBlockStart;
        clock.hasPpq=chunkTransport.hasPosition;clock.playing=chunkTransport.playing;
        const bool keyPresent=keyChannels>0&&keyPointers[0]!=nullptr&&keyPointers[keyChannels-1]!=nullptr;
        modernEngine.process(modernPointers,channels,count,keyPresent?keyPointers:nullptr,keyPresent?keyChannels:0,clock);
        // Always clock the aligner, including at zero delay, so a later automated latency change
        // can draw from continuous history instead of newly-cleared memory.
        wetAlign.process(wet, wet, channels, count, extraWetDelay);
        dryAlign.process(dryPointers, dryPointers, channels, count, fixedLatency);

        for (int i = 0; i < count; ++i)
        {
            const float amount = static_cast<float>(wetAmount.next());
            const float blend=static_cast<float>(modernAmount.next());
            for (int channel = 0; channel < channels; ++channel)
            {
                wet[channel][i] = dryPointers[channel][i] + amount * (wet[channel][i] - dryPointers[channel][i]);
                if(blend==1)wet[channel][i]=modernPointers[channel][i];
                else if(blend!=0)wet[channel][i]+=blend*(modernPointers[channel][i]-wet[channel][i]);
            }
        }
    }
}

double ModulePluginProcessor::getTailLengthSeconds() const
{return static_cast<double>(fixedLatency)/currentSampleRate.load(std::memory_order_relaxed);}


juce::AudioProcessorEditor* ModulePluginProcessor::createEditor()
{
    return new ModulePluginEditor(*this);
}

void ModulePluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto tree=parameters.copyState();
    if(compactLegacyState())
    {
        for(int i=tree.getNumChildren();--i>=0;)if(tree.getChild(i).getProperty("id").toString().startsWith("v2_"))tree.removeChild(i,nullptr);
        tree.removeProperty("v2_schema",nullptr);tree.removeProperty("v2_program",nullptr);
    }
    else
    {
        tree.setProperty("v2_schema",1,nullptr);tree.setProperty("v2_program",getCurrentProgram(),nullptr);
        for(const auto& spec:modern::specs())
            tree.getChildWithProperty("id",spec.id).setProperty("value",parameterValue(spec.id),nullptr);
    }
    if (auto xml = tree.createXml())
        copyXmlToBinary(*xml, destination);
}

bool ModulePluginProcessor::compactLegacyState() const noexcept
{
    if(modern::engine(modernValues)!=0)return false;
    for(size_t i=1;i<modern::Count;++i)
    {
        const auto* p=parameter(modern::specs()[i].id);
        if(p->getValue()!=p->getDefaultValue())return false;
    }
    return true;
}

void ModulePluginProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType().toString()))
        {
            auto tree=juce::ValueTree::fromXml(*xml);
            const int schema=static_cast<int>(tree.getProperty("v2_schema",0));
            if(schema>1)return;
            bool engineField=false;
            for(const auto& child:tree)engineField=engineField||child.getProperty("id").toString()=="v2_engine";
            const bool full=schema==1&&engineField;
            for(size_t i=0;i<modern::Count;++i)
            {
                const auto& spec=modern::specs()[i];auto child=tree.getChildWithProperty("id",spec.id);
                double value=full&&child.isValid()?static_cast<double>(child.getProperty("value",spec.defaultValue)):spec.defaultValue;
                if(!full&&i==modern::Engine)value=0;
                if(!std::isfinite(value))value=spec.defaultValue;
                value=parameter(spec.id)->getNormalisableRange().snapToLegalValue(static_cast<float>(juce::jlimit<double>(spec.minimum,spec.maximum,value)));
                if(!child.isValid()){child=juce::ValueTree("PARAM");child.setProperty("id",spec.id,nullptr);tree.addChild(child,-1,nullptr);}
                child.setProperty("value",value,nullptr);
            }
            if(full)currentProgram.store(juce::jlimit(0,getNumPrograms()-1,static_cast<int>(tree.getProperty("v2_program",0))),std::memory_order_relaxed);
            parameters.replaceState(tree);resetModern.store(true,std::memory_order_release);
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
    for(size_t i=0;i<modern::Count;++i)setParameterValue(modern::specs()[i].id,i==modern::Engine?0:modern::specs()[i].defaultValue);
}

} // namespace hpeq::standalone
