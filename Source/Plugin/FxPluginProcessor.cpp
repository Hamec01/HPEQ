#include "Plugin/FxPluginProcessor.h"
#include "Rack/RackPresets.h"

#include <cmath>

#include "Plugin/FxParameterContract.h"
#include "Plugin/FxPluginEditor.h"

namespace hpeq
{
namespace
{
class ProjectEdit final : public juce::UndoableAction
{
public:
    ProjectEdit(FxPluginProcessor& p, FxProjectState beforeEdit, FxProjectState afterEdit)
        : processor(p), before(std::move(beforeEdit)), after(std::move(afterEdit)) {}
    bool perform() override { return processor.setProject(after); }
    bool undo() override { return processor.setProject(before); }
    int getSizeInUnits() override { return 1; }

private:
    FxPluginProcessor& processor;
    FxProjectState before, after;
};

const juce::Identifier kStateRoot { "HPEQ" };
const juce::Identifier kStateSchema { "schemaVersion" };
const juce::Identifier kParametersTree { "Parameters" };

constexpr double kGainRampSeconds = 0.02;

float targetGain(float gainDb, bool bypassed) noexcept
{
    return bypassed ? 1.0f : juce::Decibels::decibelsToGain(gainDb);
}
} // namespace

FxPluginProcessor::FxPluginProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, kParametersTree, params::createLayout())
{
    outputGainDb = parameters.getRawParameterValue(params::outputGain.getParamID());
    bypassParameter = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter(params::bypass.getParamID()));
    jassert(outputGainDb != nullptr && bypassParameter != nullptr);
    engine.publish(project);
}

void FxPluginProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    const juce::ScopedLock lock(projectLock); // serializes native topology allocation with message-thread publication
    const int maxBlock = juce::jmax(1, maximumExpectedSamplesPerBlock);
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    capture.reset();
    inputLoudness.prepare(sampleRate, false);
    outputLoudness.prepare(sampleRate, true);
    engine.prepare(sampleRate, maxBlock,getTotalNumInputChannels());
    tubes.prepare(sampleRate, maxBlock);
    dry.setSize(dsp::kMaxChannels, maxBlock);
    dryAlign.prepare(dsp::ProjectEngine::maxLatency(sampleRate) + tubes.latencySamples());
    bypassRampSteps = dsp::stepsFor(dsp::kMixRampSeconds, sampleRate);
    wetAmount.reset(bypassParameter->get() ? 0.0 : 1.0);
    gain.reset(sampleRate, kGainRampSeconds);
    gain.setCurrentAndTargetValue(targetGain(outputGainDb->load(), bypassParameter->get()));
    updateLatencyAndTail(project);
}

void FxPluginProcessor::releaseResources() {}

bool FxPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    // Mono -> mono and stereo -> stereo only. Mono -> stereo needs a defined upmix rule first (01 §10).
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void FxPluginProcessor::measure(const juce::AudioBuffer<float>& buffer,
                                int numChannels,
                                std::atomic<float>& peak,
                                std::atomic<float>& rms) noexcept
{
    float blockPeak = 0.0f;
    double sumSquares = 0.0;
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float v = data[i];
            blockPeak = std::max(blockPeak, std::abs(v));
            sumSquares += static_cast<double>(v) * v;
        }
    }
    const auto count = static_cast<double>(numSamples) * juce::jmax(1, numChannels);
    peak.store(blockPeak, std::memory_order_relaxed);
    rms.store(static_cast<float>(std::sqrt(sumSquares / count)), std::memory_order_relaxed);
}

void FxPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numInputs = getTotalNumInputChannels();
    const int numOutputs = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    for (int ch = numInputs; ch < numOutputs; ++ch)
        buffer.clear(ch, 0, numSamples);
    if (numSamples <= 0)
        return;

    const int channels = juce::jmin(numInputs, numOutputs, buffer.getNumChannels());
    measure(buffer, channels, inputPeak, inputRms);
    inputLoudness.process(buffer.getArrayOfReadPointers(), channels, numSamples);

    const bool bypassed = bypassParameter->get();
    gain.setTargetValue(targetGain(outputGainDb->load(std::memory_order_relaxed), bypassed));
    wetAmount.setTarget(bypassed ? 0.0 : 1.0, bypassRampSteps);

    // The layer keeps running while bypassed (warm filter/detector state, no click on return); bypass
    // crossfades to the dry copy and, once settled, outputs the dry samples exactly.
    dsp::Transport transport;
    transport.bpm=fallbackBpm.load(std::memory_order_relaxed);
    if (auto* hostPlayHead = getPlayHead())
        if (const auto position = hostPlayHead->getPosition())
        {
            if (const auto bpm = position->getBpm(); bpm.hasValue() && *bpm > 0.0)
            {
                transport.bpm = *bpm;
            }
            transport.playing = position->getIsPlaying();
            if (const auto ppq = position->getPpqPosition(); ppq.hasValue())
            {
                transport.hasPosition = true;
                transport.ppqAtBlockStart = *ppq;
            }
        }

    hostBpm.store(transport.bpm,std::memory_order_relaxed);
    auto* const* data = buffer.getArrayOfWritePointers();
    const int capacity = dry.getNumSamples();
    if (capacity <= 0)
        return; // not prepared yet: leave the input untouched
    for (int start = 0; start < numSamples; start += capacity)
    {
        const int n = juce::jmin(capacity, numSamples - start);
        float* chunk[dsp::kMaxChannels] {};
        for (int ch = 0; ch < channels; ++ch)
        {
            chunk[ch] = data[ch] + start;
            dry.copyFrom(ch, 0, chunk[ch], n);
        }

        if (start > 0 && transport.hasPosition)
            transport.ppqAtBlockStart += static_cast<double>(capacity) * transport.bpm / (60.0 * currentSampleRate.load(std::memory_order_relaxed));
        const bool master=masterRoute.load(std::memory_order_relaxed);
        const auto applyGain=[&]
        {
            if(gain.isSmoothing())
                for(int i=0;i<n;++i)
                {
                    const auto g=gain.getNextValue();
                    for(int ch=0;ch<channels;++ch)chunk[ch][i]*=g;
                }
            else if(const auto g=gain.getTargetValue();g!=1.0f)
                for(int ch=0;ch<channels;++ch)juce::FloatVectorOperations::multiply(chunk[ch],g,n);
        };
        // BRING LENIN / STALIN BACK: the tube colour of the whole output (its latency only while it runs)
        tubes.setTargets(tubeLenin.load(std::memory_order_relaxed), tubeStalin.load(std::memory_order_relaxed),
                         tubeLeninMix.load(std::memory_order_relaxed), tubeStalinMix.load(std::memory_order_relaxed));
        const int tubeLatency = tubes.isRunning() ? tubes.latencySamples() : 0;
        if(master){applyGain();tubes.process(chunk,channels,n);}
        engine.process(chunk,channels,n,transport);
        if(!master)tubes.process(chunk, channels, n);
        // Align the dry copy with the layer output (PDC): bypass and the difference view stay in time.
        if (const int latency = engine.getLatencySamples() + tubeLatency; latency > 0)
        {
            auto* const* dryData = dry.getArrayOfWritePointers();
            dryAlign.process(dryData, dryData, channels, n, latency);
        }

        if (wetAmount.isRamping())
        {
            for (int i = 0; i < n; ++i)
            {
                const auto w = static_cast<float>(wetAmount.next());
                for (int ch = 0; ch < channels; ++ch)
                    chunk[ch][i] = dry.getSample(ch, i) + w * (chunk[ch][i] - dry.getSample(ch, i));
            }
        }
        else if (wetAmount.value() == 0.0)
        {
            for (int ch = 0; ch < channels; ++ch)
                std::copy(dry.getReadPointer(ch), dry.getReadPointer(ch) + n, chunk[ch]);
        }

        if (!master && gain.isSmoothing())
        {
            // One gain value per sample frame, identical for every channel.
            for (int i = 0; i < n; ++i)
            {
                const float g = gain.getNextValue();
                for (int ch = 0; ch < channels; ++ch)
                    chunk[ch][i] *= g;
            }
        }
        else if (const float g = gain.getTargetValue(); !master && g != 1.0f)
        {
            for (int ch = 0; ch < channels; ++ch)
                juce::FloatVectorOperations::multiply(chunk[ch], g, n);
        }
        // g == 1 and not ramping: untouched, so pass-through and bypass are bit-exact.
        capture.push(dry.getArrayOfReadPointers(), chunk, channels, n);
    }

    measure(buffer, channels, outputPeak, outputRms);
    outputLoudness.process(buffer.getArrayOfReadPointers(), channels, numSamples);
    nativeTailHint.store(engine.liveNativeTailSeconds(),std::memory_order_relaxed);
}

bool FxPluginProcessor::editProject(const juce::String& actionName, const std::function<void(FxProjectState&)>& change)
{
    auto before = getProject();
    auto after = before;
    change(after);
    sanitize(after);
    if (toValueTree(after).isEquivalentTo(toValueTree(before)))
        return true;
    return undoManager.perform(new ProjectEdit(*this, std::move(before), std::move(after)), actionName);
}

void FxPluginProcessor::selectLayer(int index)
{
    {
        const juce::ScopedLock lock(projectLock);
        index = juce::jlimit(0, kNumLayers - 1, index);
        if (project.selectedLayer == index)
            return;
        project.selectedLayer = index;
    }
    sendChangeMessage();
}

void FxPluginProcessor::copyLayer(int index)
{
    layerClipboard = getProject().layers[static_cast<size_t>(juce::jlimit(0, kNumLayers - 1, index))];
}

void FxPluginProcessor::pasteLayer(int index)
{
    if (!layerClipboard)
        return;
    const auto source = *layerClipboard;
    editProject("Paste layer", [&](FxProjectState& p) { copyLayerInto(p, source, index); });
}

void FxPluginProcessor::selectAB(int slot)
{
    slot = juce::jlimit(0, 1, slot);
    if (slot == activeAB)
        return;
    abStore[static_cast<size_t>(activeAB)] = getProject();
    if (!abUsed[static_cast<size_t>(slot)])
    {
        abStore[static_cast<size_t>(slot)] = abStore[static_cast<size_t>(activeAB)];
        abUsed[static_cast<size_t>(slot)] = true;
    }
    activeAB = slot;
    undoManager.clearUndoHistory();
    setProject(abStore[static_cast<size_t>(slot)]);
}

void FxPluginProcessor::copyToOtherAB()
{
    const int other = 1 - activeAB;
    abStore[static_cast<size_t>(other)] = getProject();
    abUsed[static_cast<size_t>(other)] = true;
}

FxPluginProcessor::MeterSnapshot FxPluginProcessor::getMeters() const noexcept
{
    return { inputPeak.load(std::memory_order_relaxed),
             inputRms.load(std::memory_order_relaxed),
             outputPeak.load(std::memory_order_relaxed),
             outputRms.load(std::memory_order_relaxed) };
}

FxProjectState FxPluginProcessor::getProject() const
{
    const juce::ScopedLock lock(projectLock);
    return project;
}

bool FxPluginProcessor::setProject(FxProjectState newProject)
{
    sanitize(newProject);
    const juce::ScopedLock lock(projectLock);
    if(!engine.publish(newProject))return false;
    project = std::move(newProject);
    fallbackBpm.store(project.referenceBpm,std::memory_order_relaxed);
    masterRoute.store(project.masterRoute,std::memory_order_relaxed);
    tubeLenin.store(project.lenin, std::memory_order_relaxed);
    tubeStalin.store(project.stalin, std::memory_order_relaxed);
    tubeLeninMix.store(project.leninMix, std::memory_order_relaxed);
    tubeStalinMix.store(project.stalinMix, std::memory_order_relaxed);

    updateLatencyAndTail(project);
    sendChangeMessage(); // the editor refreshes on the message thread
    return true;
}
bool FxPluginProcessor::loadRackPreset(int index)
{
    const auto& bank=rack::recipes();if(index<0||index>=static_cast<int>(bank.size()))return false;
    FxProjectState p;juce::String error;const auto& recipe=bank[static_cast<size_t>(index)];
    if(!rack::makeProject(recipe,p,error))return false;
    undoManager.beginNewTransaction("Rack preset "+recipe.name);
    if(!editProject("Rack preset "+recipe.name,[&](FxProjectState& target){target=p;}))return false;
    if(auto* parameter=parameters.getParameter(params::outputGain.getParamID()))parameter->setValueNotifyingHost(parameter->convertTo0to1(0));
    setPresetName(recipe.name);return true;
}

native::FaderView FxPluginProcessor::nativeFaderView(int layer,int node,int slot) const
{
    const juce::ScopedLock lock(projectLock);
    if(layer<0||layer>=kNumLayers||slot<0||slot>=kMaxFxPerNode)return {};
    if(auto* runtime=engine.nativeRuntime(layer,node,slot);runtime&&runtime->kind()==FxType::Gain)
    {
        auto view=native::readFader(runtime->referenceProcessor());
        if(auto* right=engine.nativeRightRuntime(layer,node,slot))
        {
            const auto r=native::readFader(right->referenceProcessor());
            const std::vector<FxSlotState>* chain=node==8?&project.layers[static_cast<size_t>(layer)].mixChain:nullptr;
            for(const auto& n:project.layers[static_cast<size_t>(layer)].fxNodes)if(n.engineSlot==node)chain=&n.chain;
            for(const auto& n:project.layers[static_cast<size_t>(layer)].eqNodes)if(n.engineSlot==node)chain=&n.chain;
            if(chain==nullptr||slot>=static_cast<int>(chain->size()))return {};
            const auto& values=(*chain)[static_cast<size_t>(slot)].parameters;
            const auto value=[&](const char* id,double fallback){return static_cast<double>(values.getWithDefault(id,fallback));};
            const auto powerSum=[](double a,double b,double divisor){return 10*std::log10(std::max(1.e-12,(std::pow(10.,a/10)+std::pow(10.,b/10))/divisor));};
            view.available=view.available&&r.available;view.learning=view.learning||r.learning;
            view.canApply=(view.canApply||view.measured<=-60)&&(r.canApply||r.measured<=-60)
                &&(view.canApply||r.canApply)&&!view.learning&&view.available;
            const double peak=std::max(view.predictedPeak-view.proposal,r.predictedPeak-r.proposal);
            view.measured=powerSum(view.measured,r.measured,value("measure",0)==0?2.:1.);
            view.input=powerSum(view.input,r.input,1.);view.output=powerSum(view.output,r.output,1.);
            view.proposal=juce::jlimit(-value("matchCut",24),value("matchBoost",12),value("targetDb",-18)-view.measured);
            if(value("peakGuard",1)!=0)view.proposal=std::min(view.proposal,value("matchCeiling",-1)-peak);
            if(view.proposal< -value("matchCut",24))view.canApply=false;
            view.predictedPeak=peak+view.proposal;
        }
        return view;
    }
    return {};
}
native::Meters FxPluginProcessor::nativeMeters(int layer,int node,int slot) const
{
    const juce::ScopedLock lock(projectLock);
    if(auto* runtime=engine.nativeRuntime(layer,node,slot))
    {
        auto m=runtime->meters();
        if(auto* right=engine.nativeRightRuntime(layer,node,slot))
        {
            const auto r=right->meters();
            const auto rms=[](float a,float b){return static_cast<float>(10*std::log10((std::pow(10.,a/10)+std::pow(10.,b/10))*.5));};
            m.input=rms(m.input,r.input);m.output=rms(m.output,r.output);m.reduction=std::max(m.reduction,r.reduction);
        }
        return m;
    }
    return {};
}
bool FxPluginProcessor::nativeFaderAction(int layer,int node,int slot,int action)
{
    const juce::ScopedLock lock(projectLock);
    if(layer<0||layer>=kNumLayers||slot<0||slot>=kMaxFxPerNode)return false;
    auto p=project;auto* chain=&p.layers[static_cast<size_t>(layer)].mixChain;
    if(node!=8)
    {
        chain=nullptr;
        for(auto& n:p.layers[static_cast<size_t>(layer)].fxNodes)if(n.engineSlot==node)chain=&n.chain;
        for(auto& n:p.layers[static_cast<size_t>(layer)].eqNodes)if(n.engineSlot==node)chain=&n.chain;
    }
    if(chain==nullptr||slot>=static_cast<int>(chain->size())||(*chain)[static_cast<size_t>(slot)].type!=FxType::Gain)return false;
    auto* runtime=engine.nativeRuntime(layer,node,slot);double applied=0;
    if(runtime==nullptr)return false;
    auto* right=engine.nativeRightRuntime(layer,node,slot);
    if(right&&action==3)
    {const auto view=nativeFaderView(layer,node,slot);if(!view.canApply)return false;applied=view.proposal;}
    else
    {
        if(!native::actionFader(runtime->referenceProcessor(),action,applied))return false;
        if(right&&!native::actionFader(right->referenceProcessor(),action,applied))return false;
    }
    if(action==3)
    {
        undoManager.beginNewTransaction("Apply Fader Match");
        return editProject("Apply Fader Match",[=](FxProjectState& edited) {
            auto* c=&edited.layers[static_cast<size_t>(layer)].mixChain;
            if(node!=8)
            {
                for(auto& n:edited.layers[static_cast<size_t>(layer)].fxNodes)if(n.engineSlot==node)c=&n.chain;
                for(auto& n:edited.layers[static_cast<size_t>(layer)].eqNodes)if(n.engineSlot==node)c=&n.chain;
            }
            (*c)[static_cast<size_t>(slot)].parameters.set("matchedDb",applied);
        });
    }
    return true;
}

void FxPluginProcessor::updateLatencyAndTail(const FxProjectState& state)
{
    // Tail: the longest Reverb / Delay tail of a layer; Serial layers can feed one tail into the next
    // layer's tail, so their tails add up. Plus the latency.
    const double sampleRate = currentSampleRate.load(std::memory_order_relaxed);
    const int latency = dsp::ProjectEngine::latencyFor(state, sampleRate) + (state.lenin || state.stalin ? tubes.latencySamples() : 0);
    double sum = 0.0, max = 0.0;
    for (const auto& layerState : state.layers)
    {
        if (!layerState.enabled)
            continue;
        double tail = 0.0;
        const auto snapshot = dsp::makeSnapshot(layerState,sampleRate);
        std::array<dsp::FxNodeParams, kMaxFxNodesPerLayer + 1> all {};
        std::copy(snapshot.fx.begin(), snapshot.fx.end(), all.begin());
        all.back() = snapshot.mix;
        for (const auto& node : all)
            for (int k = 0; node.active && k < node.numSlots; ++k)
                if (const auto& slot = node.slots[static_cast<size_t>(k)]; slot.active && slot.engineRevision==0)
                {
                    if (slot.type == FxType::Reverb)
                        tail = std::max(tail, dsp::ReverbProcessor::tailSeconds(slot, hostBpm.load(std::memory_order_relaxed)));
                    else if (slot.type == FxType::Delay)
                        tail = std::max(tail, dsp::DelayProcessor::tailSeconds(slot, hostBpm.load(std::memory_order_relaxed)));
                }
        sum += tail;
        max = std::max(max, tail);
    }
    nativeTailHint.store(engine.nativeTailSeconds(),std::memory_order_relaxed);
    tailSeconds.store((state.routing == LayerRouting::Serial ? sum : max) + latency / sampleRate, std::memory_order_relaxed);
    if (latency != getLatencySamples())
        setLatencySamples(latency);
}

void FxPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree root(kStateRoot);
    const auto saved=getProject();
    root.setProperty(kStateSchema, projectSchemaVersion(saved), nullptr);
    if (const auto name = getPresetName(); name.isNotEmpty())
        root.setProperty("presetName", name, nullptr);
    root.appendChild(parameters.copyState(), nullptr);
    root.appendChild(toValueTree(saved), nullptr);
    if (auto xml = root.createXml())
        copyXmlToBinary(*xml, destData);
}

void FxPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr)
        return;
    const auto root = juce::ValueTree::fromXml(*xml);
    if (!root.hasType(kStateRoot))
        return;
    const int version = root.getProperty(kStateSchema, 0);
    if (version < 1 || version > kStateSchemaVersion)
        return; // a newer or broken state: keep the current one rather than half-load it

    const auto projectTree = root.getChildWithName("HpeqProject");
    auto restored = fromValueTree(projectTree);
    if (!restored.has_value())
        return;

    if(!setProject(std::move(*restored)))return;

    const auto parametersTree = root.getChildWithName(kParametersTree);
    if (parametersTree.isValid())
        parameters.replaceState(parametersTree);
    {
        const juce::ScopedLock lock(projectLock);
        presetName = root["presetName"].toString();
    }
}

bool FxPluginProcessor::loadPreset(const void* data, int sizeInBytes, const juce::String& name)
{
    const auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr)
        return false;
    const auto root = juce::ValueTree::fromXml(*xml);
    const int version = root.getProperty(kStateSchema, 0);
    if (!root.hasType(kStateRoot) || version < 1 || version > kStateSchemaVersion)
        return false;
    auto restored = fromValueTree(root.getChildWithName("HpeqProject"));
    if (!restored.has_value())
        return false;

    undoManager.beginNewTransaction("Preset " + name);
    if(!editProject("Preset " + name, [&restored](FxProjectState& p) { p = *restored; }))return false;
    // Output gain comes with the preset (not undoable: it is a host parameter); Bypass stays.
    const auto parametersTree = root.getChildWithName(kParametersTree);
    for (const auto& child : parametersTree)
        if (child["id"].toString() == params::outputGain.getParamID())
            if (auto* p = parameters.getParameter(params::outputGain.getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(child["value"])));
    setPresetName(name);
    return true;
}

juce::AudioProcessorEditor* FxPluginProcessor::createEditor()
{
    return new FxPluginEditor(*this);
}
} // namespace hpeq

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new hpeq::FxPluginProcessor();
}
