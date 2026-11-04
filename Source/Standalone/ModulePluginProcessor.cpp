#include "Standalone/ModulePluginProcessor.h"
#include "Core/ModulePresets.h"

#include "Standalone/ModulePluginEditor.h"
#include <charconv>

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
    controls.bind(parameters);
    rawValues.reserve(getSpec().parameters.size());
    for (const auto& spec : getSpec().parameters)
    {
        auto* value = parameters.getRawParameterValue(spec.id);
        jassert(value != nullptr);
        rawValues.push_back(value);
    }
    bypassParameter = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("bypass"));
    jassert(bypassParameter != nullptr);
}

void ModulePluginProcessor::prepareToPlay(double sampleRate, int expectedBlockSize)
{
    maximumBlockSize = juce::jmax(1, expectedBlockSize);
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    engine.prepare(sampleRate, maximumBlockSize);
    kernel.prepare(sampleRate,getTotalNumInputChannels());
    kernel.set(modernSettings());
    processedFrames=0;publishedFrames.store(0,std::memory_order_release);
    audioEpoch=epoch.fetch_add(1,std::memory_order_acq_rel)+1;
    selection.reset(controls.value(Engine)>.5f?1.0:0.0);
    modern.setSize(dsp::kMaxChannels,maximumBlockSize,false,false,true);

    fixedLatency = maximumLatencySamples(kind, sampleRate);
    wetAlign.prepare(fixedLatency);
    dryAlign.prepare(fixedLatency);
    dry.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    bypassRampSteps = dsp::stepsFor(kBypassRampSeconds, sampleRate);
    wetAmount.reset(isModuleOn() ? 1.0 : 0.0);
    setLatencySamples(fixedLatency);

    dsp::FaderSnapshot snapshot;
    snapshot.mix.active = true;
    snapshot.mix.channel = ChannelMode::Stereo;
    snapshot.mix.numSlots = 1;
    snapshot.mix.slots[0] = currentSlot(isModuleOn());
    engine.publish(snapshot);
    engine.update();
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
    callbackClock=hamlo::fader::Clock{};
    dsp::Transport transport;
    transport.bpm = hostBpm.load(std::memory_order_relaxed);
    callbackClock.bpm=transport.bpm;
    if (auto* hostPlayHead = getPlayHead())
        if (const auto position = hostPlayHead->getPosition())
        {
            if (const auto bpm = position->getBpm(); bpm.hasValue() && *bpm > 0.0)
            {
                transport.bpm = *bpm;
                hostBpm.store(*bpm, std::memory_order_relaxed);
            }
            callbackClock.bpm=transport.bpm;
            transport.playing = position->getIsPlaying();
            callbackClock.playing=transport.playing;
            if(const auto samples=position->getTimeInSamples();samples.hasValue())
            {callbackClock.hasSamples=true;callbackClock.samplePosition=*samples;}
            if (const auto ppq = position->getPpqPosition(); ppq.hasValue())
            {
                callbackClock.hasPpq=true;callbackClock.ppq=*ppq;
                transport.hasPosition = true;
                transport.ppqAtBlockStart = *ppq;
            }
        }
    return transport;
}

void ModulePluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    processAudio(buffer,false);
}
void ModulePluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    processAudio(buffer,true);
}
void ModulePluginProcessor::processAudio(juce::AudioBuffer<float>& buffer,bool forceBypass)
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
    const bool on = !forceBypass&&isModuleOn();
    const bool modernOn=controls.value(Engine)>.5f;
    kernel.set(modernSettings(forceBypass));
    const auto nowEpoch=epoch.load(std::memory_order_acquire);
    if(audioEpoch!=nowEpoch)
    {
        kernel.reset();audioEpoch=nowEpoch;
        selection.reset(modernOn?1.0:0.0);
    }
    selection.setTarget(modernOn?1.0:0.0,bypassRampSteps);
    wetAmount.setTarget(on ? 1.0 : 0.0, bypassRampSteps);

    dsp::FaderSnapshot snapshot;
    snapshot.mix.active = true;
    snapshot.mix.channel = ChannelMode::Stereo;
    snapshot.mix.numSlots = 1;
    snapshot.mix.slots[0] = currentSlot(on);
    engine.publish(snapshot);
    engine.update();
    const int extraWetDelay = juce::jmax(0, fixedLatency - engine.getLatencySamples());
    const auto transport = currentTransport();

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
            juce::FloatVectorOperations::copy(modern.getWritePointer(channel),wet[channel],count);
        }

        auto chunkTransport = transport;
        if (chunkTransport.hasPosition && start > 0)
            chunkTransport.ppqAtBlockStart += static_cast<double>(start) * chunkTransport.bpm
                                               / (60.0 * currentSampleRate.load(std::memory_order_relaxed));
        engine.process(wet, channels, count, chunkTransport);
        // Consume command events exactly, including commands crossing prepared chunks.
        for(int offset=0;offset<count;)
        {
            FaderCommand command;
            while(commands.peek(command))
            {
                if(command.epoch!=audioEpoch){commands.pop();continue;}
                if(command.frame>processedFrames+static_cast<std::uint64_t>(offset))break;
                commands.pop();
                if(command.action==FaderAction::Reset)kernel.reset();
                else if(command.action==FaderAction::StartLearn)kernel.startLearn();
                else kernel.stopLearn();
            }
            int length=count-offset;
            if(commands.peek(command)&&command.epoch==audioEpoch&&command.frame>processedFrames+static_cast<std::uint64_t>(offset))
                length=static_cast<int>(std::min<std::uint64_t>(static_cast<std::uint64_t>(length),command.frame-processedFrames-static_cast<std::uint64_t>(offset)));
            auto clock=callbackClock;
            const auto advance=start+offset;
            if(clock.hasPpq)clock.ppq+=advance*clock.bpm/(60.0*currentSampleRate.load(std::memory_order_relaxed));
            if(clock.hasSamples)clock.samplePosition+=advance;
            float* audio[dsp::kMaxChannels]{};
            for(int ch=0;ch<channels;++ch)audio[ch]=modern.getWritePointer(ch,offset);
            kernel.process(audio,channels,length,clock);offset+=length;
        }
        // Always clock the aligner, including at zero delay, so a later automated latency change
        // can draw from continuous history instead of newly-cleared memory.
        wetAlign.process(wet, wet, channels, count, extraWetDelay);
        dryAlign.process(dryPointers, dryPointers, channels, count, fixedLatency);

        for (int i = 0; i < count; ++i)
        {
            const float amount = static_cast<float>(wetAmount.next());
            const float choose=static_cast<float>(selection.next());
            for (int channel = 0; channel < channels; ++channel)
            {
                wet[channel][i] = dryPointers[channel][i] + amount * (wet[channel][i] - dryPointers[channel][i]);
                const float current=modern.getSample(channel,i);
                if(choose==1.0f)wet[channel][i]=current;
                else if(choose!=0.0f)wet[channel][i]+=choose*(current-wet[channel][i]);
            }
        }
        processedFrames+=static_cast<std::uint64_t>(count);
    }
    publishedFrames.store(processedFrames,std::memory_order_release);
    publishStatus(forceBypass||!on);
}

double ModulePluginProcessor::getTailLengthSeconds() const
{
    return controls.value(Engine)>.5f?.02:0.0;
}

juce::AudioProcessorEditor* ModulePluginProcessor::createEditor()
{
    return new ModulePluginEditor(*this);
}

void ModulePluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state=parameters.copyState();
    state.setProperty("faderVersion",3,nullptr);
    state.setProperty("faderProgram",getCurrentProgram(),nullptr);
    state.setProperty("faderPage",editorPage(),nullptr);
    if(auto xml=state.createXml())copyXmlToBinary(*xml,destination);
}
void ModulePluginProcessor::setStateInformation(const void* data,int size)
{
    if(size<=0)return;
    auto xml=getXmlFromBinary(data,size);
    if(!xml||!xml->hasTagName(parameters.state.getType().toString()))return;
    const bool old=!xml->hasAttribute("faderVersion");
    if(!old&&xml->getStringAttribute("faderVersion")!="3")return;
    auto incoming=juce::ValueTree::fromXml(*xml);
    if(!incoming.isValid())return;
    // Require a complete, finite, duplicate-free parameter set before mutation.
    // Historic partial old states retain the old parameters they did not contain.
    std::vector<juce::String> seen;
    for(auto child:incoming)
    {
        if(!child.hasType("PARAM"))return;
        const auto id=child.getProperty("id").toString();
        if(parameter(id)==nullptr||(!old&& !child.hasProperty("value")))return;
        if(std::find(seen.begin(),seen.end(),id)!=seen.end())return;
        const auto text=child.getProperty("value").toString().trim();
        const auto* begin=text.toRawUTF8();const auto* end=begin+text.getNumBytesAsUTF8();
        double value=0;const auto parsed=std::from_chars(begin,end,value);
        if(parsed.ec!=std::errc{}||parsed.ptr!=end||!std::isfinite(value))return;
        const auto& range=parameter(id)->getNormalisableRange();
        if(!old&&(value<range.start||value>range.end))return;
        seen.push_back(id);
    }
    if(!old&&seen.size()!=getParameters().size())return;
    if(old)
    {
        for(const auto& c:faderControls())
        {
            // Old projects cannot carry appended fields, even in a malformed extension.
            for(int i=incoming.getNumChildren();--i>=0;)
                if(incoming.getChild(i).getProperty("id").toString()==c.id)incoming.removeChild(i,nullptr);
            juce::ValueTree child("PARAM");child.setProperty("id",c.id,nullptr);
            child.setProperty("value",c.id=="engine"?0.0:static_cast<double>(c.initial),nullptr);incoming.addChild(child,-1,nullptr);
        }
        page.store(0,std::memory_order_relaxed);
    }
    else
    {
        currentProgram.store(juce::jlimit(0,getNumPrograms()-1,xml->getIntAttribute("faderProgram",0)),std::memory_order_relaxed);
        setEditorPage(xml->getIntAttribute("faderPage",0));
    }
    parameters.replaceState(incoming);
    epoch.fetch_add(1,std::memory_order_release);
}

hamlo::fader::Settings ModulePluginProcessor::modernSettings(bool forced) const noexcept
{
    auto s=controls.read(rawValues[0]->load(std::memory_order_relaxed),rawValues[3]->load(std::memory_order_relaxed),
        rawValues[2]->load(std::memory_order_relaxed),static_cast<int>(rawValues[4]->load(std::memory_order_relaxed)),
        rawValues[5]->load(std::memory_order_relaxed)>.5f,forced||!isModuleOn());
    s.enabled=s.enabled&&controls.value(Engine)>.5f;
    return s;
}
void ModulePluginProcessor::publishStatus(bool bypassed) noexcept
{
    FaderStatus s;s.reading=kernel.reading();s.proposal=kernel.proposal();
    s.commandSequence=commands.consumed();s.bypassed=bypassed;
    s.epoch=audioEpoch;s.frame=processedFrames;s.legacy=controls.value(Engine)<.5f;
    s.legacyInput=engine.getMixNode().getFaderInputDb(0);
    s.legacyOutput=engine.getMixNode().getFaderOutputDb(0);
    s.legacyLong=engine.getMixNode().getFaderInputLongDb(0);
    status.publish(s);
}
bool ModulePluginProcessor::requestAction(FaderAction action,std::uint64_t frame) noexcept
{
    if(frame==std::numeric_limits<std::uint64_t>::max())frame=publishedFrames.load(std::memory_order_acquire);
    return commands.push({action,frame,epoch.load(std::memory_order_acquire)});
}
bool ModulePluginProcessor::matchProposal(FaderStatus& s) const
{
    if(!readStatus(s)||s.epoch!=epoch.load(std::memory_order_acquire)||!s.proposal.valid||s.reading.learning||s.legacy||s.bypassed||s.commandSequence!=commands.produced()
        ||controls.value(Engine)<.5f||static_cast<int>(controls.value(Mode))!=1||controls.value(Enabled)<.5f
        ||rawValues[5]->load(std::memory_order_relaxed)>.5f||!isModuleOn())return false;
    return true;
}
bool ModulePluginProcessor::canApplyMatch() const { FaderStatus s; return matchProposal(s); }
bool ModulePluginProcessor::applyMatch()
{
    FaderStatus s;if(!matchProposal(s))return false;
    auto* p=parameter("matchedDb");p->beginChangeGesture();setParameterValue("matchedDb",s.proposal.gainDb);p->endChangeGesture();return true;
}
void ModulePluginProcessor::resetNewParameters(bool legacy)
{
    for(const auto& c:faderControls())setParameterValue(c.id,c.id=="engine"&&legacy?0.0:c.initial);
    page.store(0,std::memory_order_relaxed);
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
    currentProgram.store(juce::jlimit(0,static_cast<int>(list.size()),index),std::memory_order_relaxed);
    resetNewParameters(true);
    for (const auto& spec : getSpec().parameters)
    {
        double value = spec.defaultValue;
        if (getCurrentProgram() > 0)
            value = presets::presetValue(list[static_cast<size_t>(getCurrentProgram() - 1)], spec.id, value);
        setParameterValue(spec.id, value);
    }
    epoch.fetch_add(1,std::memory_order_release);
}

bool ModulePluginProcessor::isProgramModified() const
{
    const auto& list=presets::factoryModulePresets(getSpec().type);const int index=currentProgram.load(std::memory_order_relaxed);
    auto expected=[&](const juce::String& id,double fallback){return index>0&&index<=static_cast<int>(list.size())?presets::presetValue(list[static_cast<size_t>(index-1)],id,fallback):fallback;};
    auto differs=[&](const juce::String& id,double value){auto* par=parameter(id);return par&&std::abs(par->getValue()-par->convertTo0to1(static_cast<float>(value)))>1e-6f;};
    for(const auto& spec:getSpec().parameters)if(differs(spec.id,expected(spec.id,spec.defaultValue)))return true;
    for(const auto& c:faderControls())if(differs(c.id,expected(c.id,c.id=="engine"?0.:c.initial)))return true;
    return false;
}

} // namespace hpeq::standalone
