#include "Plugin/HamloMagProcessor.h"

#include "Plugin/HamloMagEditor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"

namespace hamlo::saturator
{
namespace
{
constexpr double kRampSeconds = 0.020; // MIX and ON fades (HPEQ: kMixRampSeconds and the bypass ramp)

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& p : parameterSpecs())
    {
        const juce::ParameterID id { p.id, 1 };
        if (!p.choices.isEmpty())
        {
            layout.add(std::make_unique<juce::AudioParameterChoice>(id, p.name, p.choices, juce::roundToInt(p.defaultValue)));
            continue;
        }
        juce::NormalisableRange<float> range(p.minimum, p.maximum, 0.0f, p.skew);
        layout.add(std::make_unique<juce::AudioParameterFloat>(id, p.name, range, p.defaultValue,
                                                               juce::AudioParameterFloatAttributes {}.withLabel(p.unit)));
    }
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}
} // namespace

HamloMagProcessor::HamloMagProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier(kStateTag), createLayout())
{
    const auto& specs = parameterSpecs();
    for (size_t i = 0; i < specs.size(); ++i)
    {
        raw.push_back(parameters.getRawParameterValue(specs[i].id));
        jassert(raw.back() != nullptr);
        if (specs[i].slot == kMixSlot)
            mixIndex = static_cast<int>(i);
        if (juce::String(specs[i].id) == "match")
            matchIndex = static_cast<int>(i);
        if (juce::String(specs[i].id) == "delta")
            deltaIndex = static_cast<int>(i);
    }
    bypassParameter = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("bypass"));
    jassert(bypassParameter != nullptr);
}

dsp::FxSlotParams HamloMagProcessor::currentSettings() const noexcept
{
    dsp::FxSlotParams s; // HPEQ: makeSlot (ModuleVariant.cpp)
    s.active = true;
    s.type = dsp::FxType::Saturator;
    const auto& specs = parameterSpecs();
    for (size_t i = 0; i < specs.size(); ++i)
    {
        const float v = raw[i]->load(std::memory_order_relaxed);
        if (specs[i].slot >= 0)
            s.values[static_cast<size_t>(specs[i].slot)] = v;
        else if (specs[i].slot == kMixSlot)
            s.mix = juce::jlimit(0.0f, 1.0f, v);
    }
    return s;
}

void HamloMagProcessor::prepareToPlay(double sampleRate, int expectedBlockSize)
{
    maximumBlockSize = juce::jmax(1, expectedBlockSize);
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    core.prepare(sampleRate, maximumBlockSize);
    {
        dsp::FxSlotParams largest; // HPEQ: maximumLatencySamples (ModuleVariant.cpp): TAPE, WOW 1; v2: 8x, the SAMPLER
        largest.active = true;
        largest.values[dsp::slot::kOversampling] = 3.0f;
        largest.values[dsp::slot::kSatModel] = 1.0f;
        largest.values[dsp::slot::kSatWow] = 1.0f;
        largest.values[dsp::slot::kSatSampler] = 1.0f;
        latency = dsp::MagV2Processor::latencySamples(largest, sampleRate);
    }
    mixAlign.prepare(latency);
    wetAlign.prepare(latency);
    dryAlign.prepare(latency);
    dry.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    dryForMix.setSize(dsp::kMaxChannels, maximumBlockSize, false, false, true);
    rampSteps = dsp::stepsFor(kRampSeconds, sampleRate);
    kIn.design(sampleRate);
    kOut.design(sampleRate);
    meanIn = meanOut = matchDb = matchTarget = longIn = blockIn = blockOut = 0.0;
    blockCount = 0;
    mixAmount.reset(juce::jlimit(0.0, 1.0, static_cast<double>(raw[static_cast<size_t>(mixIndex)]->load(std::memory_order_relaxed))));
    onAmount.reset(isOn() ? 1.0 : 0.0);
    setLatencySamples(latency);
}

bool HamloMagProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void HamloMagProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    const auto settings = currentSettings();
    const double fs = currentSampleRate.load(std::memory_order_relaxed);
    const int slotLatency = dsp::MagV2Processor::latencySamples(settings, fs);
    const bool match = raw[static_cast<size_t>(matchIndex)]->load(std::memory_order_relaxed) > 0.5f;
    const bool delta = raw[static_cast<size_t>(deltaIndex)]->load(std::memory_order_relaxed) > 0.5f;
    const int blockLength = juce::jmax(1, juce::roundToInt(0.1 * fs));
    const double meanC = std::exp(-0.1 / 1.5), longC = std::exp(-0.1 / 10.0), smoothC = std::exp(-1.0 / (0.5 * fs));
    const int extra = juce::jmax(0, latency - slotLatency);
    mixAmount.setTarget(juce::jlimit(0.0, 1.0, static_cast<double>(raw[static_cast<size_t>(mixIndex)]->load(std::memory_order_relaxed))), rampSteps);
    onAmount.setTarget(isOn() ? 1.0 : 0.0, rampSteps);

    for (int start = 0; start < samples; start += maximumBlockSize)
    {
        const int count = juce::jmin(maximumBlockSize, samples - start);
        float* wet[dsp::kMaxChannels] {};
        float* dryPointers[dsp::kMaxChannels] {};
        float* mixDry[dsp::kMaxChannels] {};
        for (int channel = 0; channel < channels; ++channel)
        {
            wet[channel] = buffer.getWritePointer(channel, start);
            dryPointers[channel] = dry.getWritePointer(channel);
            mixDry[channel] = dryForMix.getWritePointer(channel);
            juce::FloatVectorOperations::copy(dryPointers[channel], wet[channel], count);
        }
        // the slot's dry reference, delayed by the slot's own latency (HPEQ: FxNodeProcessor's slot align)
        mixAlign.process(dryPointers, mixDry, channels, count, slotLatency);
        core.process(wet, channels, count, settings);
        for (int i = 0; i < count; ++i)
        {
            const auto m = static_cast<float>(mixAmount.next());
            for (int channel = 0; channel < channels; ++channel)
            {
                const float d = mixDry[channel][i];
                wet[channel][i] = m == 1.0f ? wet[channel][i] : (m == 0.0f ? d : d + m * (wet[channel][i] - d));
            }
        }
        if (match) // HamloMag v2: loudness-matched output
            for (int i = 0; i < count; ++i)
            {
                double pin = 0.0, pout = 0.0;
                for (int channel = 0; channel < channels; ++channel)
                {
                    const double a = kIn.process(channel, mixDry[channel][i]), b = kOut.process(channel, wet[channel][i]);
                    pin += a * a;
                    pout += b * b;
                }
                // 100 ms blocks, gated like BS.1770's integrated loudness (the block counts when it is no more than
                // 10 dB under the input's 10 s level), averaged over 1.5 s (run 1 matched every sample's power, ungated:
                // 0.6 LU off on the mixes, the quiet parts weighed in where the loudness measure leaves them out)
                blockIn += pin;
                blockOut += pout;
                if (++blockCount >= blockLength)
                {
                    const double bi = blockIn / blockCount, bo = blockOut / blockCount;
                    longIn = bi + longC * (longIn - bi);
                    if (bi > 1.0e-9 && bo > 1.0e-12 && bi > 0.1 * longIn)
                    {
                        meanIn = bi + meanC * (meanIn - bi);
                        meanOut = bo + meanC * (meanOut - bo);
                        matchTarget = juce::jlimit(-24.0, 24.0, 10.0 * std::log10(meanIn / meanOut));
                    }
                    blockIn = blockOut = 0.0;
                    blockCount = 0;
                }
                matchDb = matchTarget + smoothC * (matchDb - matchTarget);
                const auto g = static_cast<float>(std::pow(10.0, matchDb / 20.0));
                for (int channel = 0; channel < channels; ++channel)
                    wet[channel][i] *= g;
            }
        if (delta) // HamloMag v2: only what HamloMag adds
            for (int channel = 0; channel < channels; ++channel)
                juce::FloatVectorOperations::subtract(wet[channel], mixDry[channel], count);
        // Always clock the aligners (also at zero delay), so a later latency change reads continuous history.
        wetAlign.process(wet, wet, channels, count, extra);
        dryAlign.process(dryPointers, dryPointers, channels, count, latency);
        for (int i = 0; i < count; ++i)
        {
            const auto on = static_cast<float>(onAmount.next());
            if (on == 1.0f)
                continue; // exact at 1 (and at 0 below)
            for (int channel = 0; channel < channels; ++channel)
                wet[channel][i] = on == 0.0f ? dryPointers[channel][i] : dryPointers[channel][i] + on * (wet[channel][i] - dryPointers[channel][i]);
        }
    }
}

void HamloMagProcessor::KWeighting::design(double sampleRate) noexcept
{
    // ITU-R BS.1770 K-weighting at any rate (the pre-filter shelf, then the RLB high-pass)
    const double pi = juce::MathConstants<double>::pi;
    {
        const double f0 = 1681.974450955533, gain = 3.999843853973347, q = 0.7071752369554196;
        const double k = std::tan(pi * f0 / sampleRate), vh = std::pow(10.0, gain / 20.0), vb = std::pow(vh, 0.4996667741545416);
        const double a0 = 1.0 + k / q + k * k;
        b[0][0] = (vh + vb * k / q + k * k) / a0;
        b[0][1] = 2.0 * (k * k - vh) / a0;
        b[0][2] = (vh - vb * k / q + k * k) / a0;
        a[0][1] = 2.0 * (k * k - 1.0) / a0;
        a[0][2] = (1.0 - k / q + k * k) / a0;
    }
    {
        const double f0 = 38.13547087602444, q = 0.5003270373238773;
        const double k = std::tan(pi * f0 / sampleRate), a0 = 1.0 + k / q + k * k;
        b[1][0] = 1.0;
        b[1][1] = -2.0;
        b[1][2] = 1.0;
        a[1][1] = 2.0 * (k * k - 1.0) / a0;
        a[1][2] = (1.0 - k / q + k * k) / a0;
    }
    for (auto& c : z)
        for (auto& s : c)
            s[0] = s[1] = 0.0;
}

double HamloMagProcessor::KWeighting::process(int channel, double x) noexcept
{
    for (int s = 0; s < 2; ++s)
    {
        auto& st = z[channel][s];
        const double y = b[s][0] * x + st[0];
        st[0] = b[s][1] * x - a[s][1] * y + st[1];
        st[1] = b[s][2] * x - a[s][2] * y;
        x = y;
    }
    return x;
}

double HamloMagProcessor::getTailLengthSeconds() const
{
    return static_cast<double>(latency) / currentSampleRate.load(std::memory_order_relaxed);
}

juce::AudioProcessorEditor* HamloMagProcessor::createEditor()
{
    return new HamloMagEditor(*this);
}

void HamloMagProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    state.setProperty("program", currentProgram, nullptr); // the preset screen shows it again after a reload
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}

void HamloMagProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType().toString()))
        {
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
            currentProgram = juce::jlimit(0, static_cast<int>(factoryPresets().size()), static_cast<int>(xml->getIntAttribute("program", 0)));
        }
}

juce::RangedAudioParameter* HamloMagProcessor::parameter(const juce::String& id) const noexcept
{
    return parameters.getParameter(id);
}

double HamloMagProcessor::parameterValue(const juce::String& id) const noexcept
{
    if (const auto* p = parameter(id))
        return p->convertFrom0to1(p->getValue());
    return 0.0;
}

void HamloMagProcessor::setParameterValue(const juce::String& id, double actualValue)
{
    if (auto* p = parameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(actualValue)));
}

bool HamloMagProcessor::programModified() const
{
    const auto& list = factoryPresets();
    const Preset* preset = currentProgram > 0 && currentProgram <= static_cast<int>(list.size()) ? &list[static_cast<size_t>(currentProgram - 1)] : nullptr;
    for (const auto& spec : parameterSpecs())
    {
        const double expected = preset != nullptr ? presetValue(*preset, spec.id, spec.defaultValue) : spec.defaultValue;
        const double tolerance = 1.0e-3 * (spec.maximum - spec.minimum);
        if (std::abs(parameterValue(spec.id) - juce::jlimit(static_cast<double>(spec.minimum), static_cast<double>(spec.maximum), expected)) > tolerance)
            return true;
    }
    return false;
}

int HamloMagProcessor::getNumPrograms()
{
    return 1 + static_cast<int>(factoryPresets().size());
}

const juce::String HamloMagProcessor::getProgramName(int index)
{
    const auto& list = factoryPresets();
    if (index <= 0 || index > static_cast<int>(list.size()))
        return "Default";
    const auto& preset = list[static_cast<size_t>(index - 1)];
    // the genre presets carry their genre and group in the host's program list (the preset screen shows them apart)
    return juce::String(preset.genre).isEmpty() ? juce::String(preset.name) : juce::String(preset.genre) + " / " + preset.group + " / " + preset.name;
}

void HamloMagProcessor::setCurrentProgram(int index)
{
    const auto& list = factoryPresets();
    currentProgram = juce::jlimit(0, static_cast<int>(list.size()), index);
    for (const auto& spec : parameterSpecs())
        setParameterValue(spec.id, currentProgram > 0 ? presetValue(list[static_cast<size_t>(currentProgram - 1)], spec.id, spec.defaultValue)
                                                      : spec.defaultValue);
}
} // namespace hamlo::saturator
