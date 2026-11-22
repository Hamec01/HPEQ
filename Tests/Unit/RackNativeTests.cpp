#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

#include "Core/FxProjectState.h"
#include "DSP/ProjectEngine.h"
#include "DSP/FxNodeProcessor.h"
#include "Harness/RtAllocationProbe.h"
#include "Plugin/FxPluginProcessor.h"
#include "Rack/NativeApi.h"
#include "Rack/NativeRegistry.h"
#include "Rack/RackPresets.h"
#include <set>

namespace hpeq::test
{
namespace
{
constexpr FxType nativeKinds[] {
    FxType::Compressor, FxType::Limiter, FxType::Reverb, FxType::Delay,
    FxType::TransientShaper, FxType::Saturator, FxType::MonstaFx,
    FxType::StereoProcessor, FxType::GateExpander, FxType::Gain
};

class RackHead final : public juce::AudioPlayHead
{
public:
    double rate = 48000, bpm = 123;
    int offset = 0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm(bpm);
        p.setIsPlaying(true);
        p.setPpqPosition(offset * bpm / (60.0 * rate));
        return p;
    }
};

float inputSample(int frame, int channel, double rate)
{
    const auto t = 2.0 * juce::MathConstants<double>::pi * frame / rate;
    const auto envelope = frame % 4000 < 1800 ? 0.7 : 0.15;
    return static_cast<float>(envelope * (0.19 * std::sin(97.0 * t) + 0.075 * std::cos((channel == 0 ? 1637 : 1921) * t)));
}

class RackNativeTests final : public juce::UnitTest
{
public:
    RackNativeTests() : UnitTest("HamloRack native runtime", "HPEQ") {}

    void runTest() override
    {
        beginTest("native public APVTS and prepared runtime produce exact output at all rates, channels and blocks");
        for (auto kind : nativeKinds)
            parity(kind);

        beginTest("all authored native engine combinations retain exact public-API parity");
        std::set<juce::String> modes;
        for(const auto& recipe:rack::recipes())
        {
            FxProjectState project;juce::String error;
            if(!rack::makeProject(recipe,project,error,false)){expect(false,error);continue;}
            const auto check=[&](const std::vector<FxSlotState>& chain) {
                for(const auto& slot:chain)
                {
                    auto key=native::productName(slot.type);
                    for(auto* id:{"engine","model","mode","v2_engine","v2_mode","v2Mode","oversampling","sampler","wow","glitchEngine","timeStretch","lookahead"})
                        if(slot.parameters.contains(id))key+="/"+juce::String(id)+"="+slot.parameters[id].toString();
                    if(modes.insert(key).second)parity(slot.type,&slot.parameters,&slot.nativeState);
                }
            };
            for(const auto& layer:project.layers){check(layer.mixChain);for(const auto& node:layer.fxNodes)check(node.chain);}
        }
        expect(modes.size()>10);

        beginTest("versioned native state, strict revisions and undo round-trip");
        auto processor = std::make_unique<FxPluginProcessor>();
        processor->prepareToPlay(48000, 256);
        auto project = processor->getProject();
        auto nativeSlot = makeNativeModule(FxType::Gain);
        nativeSlot.parameters.set("gainDb", -6.375);
        project.layers[0].mixChain = { nativeSlot };
        expect(processor->setProject(project));
        juce::MemoryBlock state;
        processor->getStateInformation(state);
        auto restored = std::make_unique<FxPluginProcessor>();
        restored->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        expectEquals(projectSchemaVersion(restored->getProject()), 2);
        expectEquals(restored->getProject().layers[0].mixChain[0].engineRevision, native::kRevision);
        expect(restored->editProject("Native gain", [](FxProjectState& p) {
            p.layers[0].mixChain[0].parameters.set("gainDb", -12.0);
        }));
        expect(restored->getUndoManager().undo());
        expectEquals(static_cast<double>(restored->getProject().layers[0].mixChain[0].parameters["gainDb"]), -6.375);
        expect(restored->getUndoManager().redo());
        expectEquals(static_cast<double>(restored->getProject().layers[0].mixChain[0].parameters["gainDb"]), -12.0);
        auto invalid = project;
        invalid.layers[0].mixChain[0].engineRevision = 999;
        expect(!restored->setProject(invalid));
        expectEquals(restored->getProject().layers[0].mixChain[0].engineRevision, native::kRevision);

        beginTest("Revolution scene codec preserves all seven lanes and rejects illegal actions");
        native::Scene scene;
        scene.lanes[0][3] = 2;
        scene.lanes[1][7] = 7;
        scene.lanes[2][11] = 12;
        scene.lanes[3][1] = 15;
        scene.lanes[6][13] = 9;
        juce::String error;
        auto params = native::defaultParameters(FxType::MonstaFx);
        params.set("timeOn", 1);
        params.set("glitchOn", 1);
        const auto blob = native::makeRevolutionScene(params, scene, error);
        expect(blob.getSize() > 0, error);
        native::Scene decoded;
        expect(native::readRevolutionScene(blob, decoded, error), error);
        expect(scene.lanes == decoded.lanes && scene.locks == decoded.locks && scene.allow == decoded.allow);
        auto encoded = makeNativeModule(FxType::MonstaFx);
        encoded.parameters = params;
        encoded.nativeState = blob;
        expect(isModuleSupported(encoded, &error), error);
        native::Runtime runtime(FxType::MonstaFx, blob, params, 48000, 256, error);
        expect(runtime.valid(), error);
        if (runtime.valid())
        {
            const auto first = runtime.state();
            native::Runtime reopened(FxType::MonstaFx, first, params, 48000, 256, error);
            expect(reopened.valid(), error);
            expect(first == reopened.state(), "native scene state changes when reopened");
        }
        scene.lanes[0][0] = 255;
        error.clear();
        expect(native::makeRevolutionScene(params, scene, error).getSize() == 0);
        expect(error.isNotEmpty());

        beginTest("native node removal releases its runtime and slot mix has exact endpoints");
        auto slot = makeNativeModule(FxType::Gain);
        slot.parameters.set("gainDb", -6.0);
        native::Runtime gain(FxType::Gain, {}, slot.parameters, 48000, 256, error);
        expect(gain.valid(), error);
        dsp::FxNodeParams nodeParams;
        nodeParams.active = true;
        nodeParams.numSlots = 1;
        nodeParams.slots[0].active = true;
        nodeParams.slots[0].type = FxType::Gain;
        nodeParams.slots[0].engineRevision = native::kRevision;
        expect(native::describe(FxType::Gain, slot.parameters, 48000, nodeParams.slots[0].nativeValues, error), error);
        nodeParams.slots[0].nativeValues.runtime = &gain;
        auto node = std::make_unique<dsp::FxNodeProcessor>();
        node->prepare(48000, 256, nullptr);
        for (float amount : { 0.0f, 0.5f, 1.0f })
        {
            node->reset();
            nodeParams.slots[0].mix = amount;
            node->setTargets(nodeParams);
            juce::AudioBuffer<float> input(2, 256), delta(2, 256);
            std::array<float, 256> presence;
            presence.fill(1.0f);
            double maximumError = 0;
            AllocationCounts counts;
            for (int block = 0; block < 24; ++block)
            {
                input.clear();
                delta.clear();
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < 256; ++i)
                        input.setSample(c, i, 0.25f);
                {
                    ScopedAllocationProbe probe;
                    node->process(input.getArrayOfReadPointers(), presence.data(), delta.getArrayOfWritePointers(), 2, 256, {}, 0);
                    counts.allocations += probe.counts().allocations;
                    counts.frees += probe.counts().frees;
                }
                if (block > 12)
                    for (int i = 0; i < 256; ++i)
                        maximumError = std::max(maximumError, std::abs(static_cast<double>(delta.getSample(0, i))
                            - amount * 0.25 * (std::pow(10.0, -6.0 / 20.0) - 1.0)));
            }
            expect(maximumError <= 2.e-6, "Native slot MIX was applied twice or lost its dry reference");
            expectEquals(static_cast<juce::int64>(counts.allocations), juce::int64(0));
            expectEquals(static_cast<juce::int64>(counts.frees), juce::int64(0));
            expectEquals(gain.inUse(), 1);
            auto removed = nodeParams;
            removed.numSlots = 0;
            {
                ScopedAllocationProbe probe;
                node->setTargets(removed);
                expectEquals(static_cast<juce::int64>(probe.counts().allocations), juce::int64(0));
            }
            expectEquals(gain.inUse(), 0);
        }

        beginTest("native mono and stereo channel routing preserves the untouched components");
        for (auto mode : { ChannelMode::Stereo, ChannelMode::Left, ChannelMode::Right, ChannelMode::Mid, ChannelMode::Side })
        {
            node->reset();
            nodeParams.channel = mode;
            nodeParams.slots[0].mix = 1;
            node->setTargets(nodeParams);
            juce::AudioBuffer<float> input(2, 256), delta(2, 256);
            std::array<float, 256> presence;
            presence.fill(1);
            for (int block = 0; block < 24; ++block)
            {
                for (int i = 0; i < 256; ++i)
                {
                    input.setSample(0, i, 0.25f);
                    input.setSample(1, i, 0.125f);
                }
                delta.clear();
                node->process(input.getArrayOfReadPointers(), presence.data(), delta.getArrayOfWritePointers(), 2, 256, {}, 0);
            }
            const double attenuation = std::pow(10.0, -6.0 / 20.0) - 1.0;
            double left = 0.25, right = 0.125;
            if (mode == ChannelMode::Left) right = 0;
            if (mode == ChannelMode::Right) left = 0;
            if (mode == ChannelMode::Mid) left = right = 0.1875;
            if (mode == ChannelMode::Side) { left = 0.0625; right = -0.0625; }
            expect(std::abs(delta.getSample(0, 255) - left * attenuation) <= 2.e-6);
            expect(std::abs(delta.getSample(1, 255) - right * attenuation) <= 2.e-6);
        }

        beginTest("native project geometry, failed publication and reprepare");
        auto rack = std::make_unique<dsp::ProjectEngine>();
        rack->prepare(48000, 256);
        FxProjectState nativeProject;
        nativeProject.layers[0].mixChain = { slot };
        expect(rack->publish(nativeProject), rack->publicationError());
        juce::AudioBuffer<float> audio(2, 512);
        const auto processRack = [&] {
            AllocationCounts counts;
            {
                ScopedAllocationProbe probe;
                rack->process(audio.getArrayOfWritePointers(), 2, 512);
                counts = probe.counts();
            }
            expectEquals(static_cast<juce::int64>(counts.allocations), juce::int64(0));
            expectEquals(static_cast<juce::int64>(counts.frees), juce::int64(0));
        };
        audio.clear();
        processRack();
        auto unsupported = nativeProject;
        unsupported.layers[0].mixChain[0].parameters.set("unknown_control", 1);
        expect(!rack->publish(unsupported));
        processRack();
        for (auto mode : { ChannelMode::Left, ChannelMode::Right, ChannelMode::Mid, ChannelMode::Side })
        {
            nativeProject.layers[0].channel = mode;
            expect(rack->publish(nativeProject), rack->publicationError());
            for (int i = 0; i < 12; ++i)
            {
                audio.clear();
                processRack();
            }
        }
        rack->prepare(96000, 256);
        processRack();
        expectEquals(rack->getLatencySamples(), 0);
    }

private:
    void parity(FxType kind,const juce::NamedValueSet* supplied=nullptr,const juce::MemoryBlock* state=nullptr)
    {
        const auto product = native::productName(kind);
        auto params = native::defaultParameters(kind);
        if (kind == FxType::Compressor)
        {
            params.set("thresholdDb", -23.317);
            params.set("ratio", 3.17);
            params.set("attackMs", 7.213);
        }
        if (kind == FxType::Saturator) params.set("driveDb", 8.137);
        if (kind == FxType::Gain) params.set("gainDb", -5.713);
        juce::MemoryBlock structural;
        if (kind == FxType::MonstaFx)
        {
            native::Scene scene;
            scene.lanes[0][0] = 1;
            scene.lanes[1][2] = 3;
            scene.lanes[2][4] = 7;
            params.set("timeOn", 1);
            params.set("glitchOn", 1);
            params.set("motionOn", 1);
            juce::String error;
            structural = native::makeRevolutionScene(params, scene, error);
            expect(structural.getSize() > 0, error);
        }

        if(supplied)params=*supplied;
        if(state)structural=*state;
        for (double rate : { 44100.0, 48000.0, 96000.0 })
            for (int channels : { 1, 2 })
            {
                auto reference = native::create(kind);
                if (structural.getSize() > 0)
                    reference.processor->setStateInformation(structural.getData(), static_cast<int>(structural.getSize()));
                for (const auto& binding : reference.bindings)
                {
                    const auto id = binding.parameter->paramID;
                    binding.parameter->setValueNotifyingHost(binding.parameter->convertTo0to1(static_cast<float>(params[id])));
                }
                RackHead head;
                head.rate = rate;
                reference.processor->setPlayHead(&head);
                reference.processor->setPlayConfigDetails(channels, channels, rate, 256);
                reference.processor->prepareToPlay(rate, 256);
                juce::String error;
                native::Runtime adapter(kind, structural, params, rate, 256, error,channels);
                expect(adapter.valid(), product + ": " + error);
                if (!adapter.valid())
                    continue;
                native::Values values;
                expect(native::describe(kind, params, rate, values, error), error);
                expectEquals(adapter.latency(), reference.processor->getLatencySamples(), product + " latency");
                bool rawExact = true;
                for (int i = 0; i < values.count; ++i)
                    rawExact = rawExact && values.physical[static_cast<size_t>(i)] == reference.bindings[static_cast<size_t>(i)].raw->load();
                expect(rawExact, product + " physical snapshot differs from public native parameter conversion");
                int offset = 0;
                bool exact = true, finite = true;
                AllocationCounts counts;
                for (int block : { 1, 64, 127, 512, 4096 })
                {
                    juce::AudioBuffer<float> a(channels, block), b(channels, block);
                    for (int c = 0; c < channels; ++c)
                        for (int i = 0; i < block; ++i)
                            a.setSample(c, i, inputSample(offset + i, c, rate));
                    b.makeCopyOf(a);
                    // Native wrapper gets the same prepared-size chunks as Runtime.
                    juce::MidiBuffer midi;
                    for (int local = 0; local < block; local += 256)
                    {
                        head.offset = offset + local;
                        float* ptr[] { a.getWritePointer(0, local), channels > 1 ? a.getWritePointer(1, local) : nullptr };
                        juce::AudioBuffer<float> part(ptr, channels, std::min(256, block - local));
                        reference.processor->processBlock(part, midi);
                    }
                    {
                        ScopedAllocationProbe probe;
                        adapter.process(b.getArrayOfWritePointers(), channels, block, values, head.bpm, true, true,
                                        offset * head.bpm / (60.0 * rate));
                        counts.allocations += probe.counts().allocations;
                        counts.frees += probe.counts().frees;
                    }
                    for (int c = 0; c < channels; ++c)
                    {
                        exact = exact && std::memcmp(a.getReadPointer(c), b.getReadPointer(c), sizeof(float) * static_cast<size_t>(block)) == 0;
                        for (int i = 0; i < block; ++i)
                            finite = finite && std::isfinite(b.getSample(c, i));
                    }
                    offset += block;
                }
                expect(exact, product + " native/adapter output differs at " + juce::String(rate) + " Hz / " + juce::String(channels) + " ch");
                expect(finite, product + " produced a nonfinite sample");
                expectEquals(static_cast<juce::int64>(counts.allocations), juce::int64(0), product + " audio allocations");
                expectEquals(static_cast<juce::int64>(counts.frees), juce::int64(0), product + " audio frees");
                reference.processor->setPlayHead(nullptr);
                reference.processor->releaseResources();
            }
    }
} rackNativeTests;
}
}
