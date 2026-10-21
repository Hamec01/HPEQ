// STATE row (M1 scope) and schema v1: round trip, sound after restore, rejection of unknown
// versions, sanitising limits (incl. the 6-slot chain from DECISIONS D2).

#include <cmath>
#include <memory>
#include <limits>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/FxProjectState.h"
#include "Harness/AudioMetrics.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxParameterContract.h"
#include "Plugin/FxPluginProcessor.h"

namespace hpeq::test
{
namespace
{
FxProjectState makeRichProject()
{
    FxProjectState project;
    project.routing = LayerRouting::Serial;
    project.selectedLayer = 3;
    auto& layer = project.layers[3];
    layer.name = "Drum bus";
    layer.mute = true;
    layer.gainDb = -3.5f;
    layer.pan = 0.25f;
    layer.width = 1.4f;
    layer.channel = ChannelMode::Side;

    EqNodeState cut;
    cut.id = 7;
    cut.shape = EqShape::LowCut;
    cut.frequencyHz = 200.0;
    cut.slopeDbPerOct = 96;
    cut.channel = ChannelMode::Side;
    layer.eqNodes.push_back(cut);

    FxNodeState node;
    node.id = 9;
    node.lowHz = 250.0;
    node.highHz = 700.0;
    node.mix = 0.5f;
    node.chain.clear();
    const FxType types[] { FxType::Compressor, FxType::Saturator, FxType::TransientShaper,
                           FxType::Reverb, FxType::Delay, FxType::MonstaFx };
    for (auto type : types)
    {
        FxSlotState slot;
        slot.type = type;
        slot.mix = 0.75f;
        slot.parameters.set("drive", 0.3);
        node.chain.push_back(slot);
    }
    layer.fxNodes.push_back(node);
    sanitize(project);
    return project;
}

void setParameter(FxPluginProcessor& p, const juce::ParameterID& id, float plainValue)
{
    auto* parameter = p.getParameters().getParameter(id.getParamID());
    parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
}

juce::AudioBuffer<float> renderWith(FxPluginProcessor& p)
{
    p.setRateAndBufferSizeDetails(48000.0, 512);
    p.prepareToPlay(48000.0, 512);
    auto audio = makeSyntheticDrums({ 48000.0, 2, 0.5, kFixtureSeed });
    juce::MidiBuffer midi;
    for (int start = 0; start < audio.getNumSamples(); start += 512)
    {
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), 2, start, juce::jmin(512, audio.getNumSamples() - start));
        p.processBlock(view, midi);
    }
    return audio;
}
} // namespace

class StateTests final : public juce::UnitTest
{
public:
    StateTests() : juce::UnitTest("State schema v1", "HPEQ") {}

    void runTest() override
    {
        beginTest("project tree round trip is lossless");
        {
            const auto project = makeRichProject();
            const auto tree = toValueTree(project);
            juce::String error;
            const auto restored = fromValueTree(tree, &error);
            expect(restored.has_value(), error);
            expect(tree.isEquivalentTo(toValueTree(*restored)));
            expectEquals(static_cast<int>(restored->layers[3].fxNodes[0].chain.size()), 6);
            expect(restored->layers[3].fxNodes[0].chain[5].type == FxType::MonstaFx);
            expectEquals(static_cast<int>(tree[juce::Identifier("schemaVersion")]), kStateSchemaVersion);
        }

        beginTest("plugin state: parameters, project and sound survive save/restore");
        {
            auto aPtr = std::make_unique<FxPluginProcessor>();
            auto& a = *aPtr;
            setParameter(a, params::outputGain, -4.5f);
            a.setProject(makeRichProject());
            juce::MemoryBlock blob;
            a.getStateInformation(blob);

            auto bPtr = std::make_unique<FxPluginProcessor>();

            auto& b = *bPtr;
            b.setStateInformation(blob.getData(), static_cast<int>(blob.getSize()));
            expectWithinAbsoluteError(b.getParameters().getRawParameterValue("outputGain")->load(), -4.5f, 1.0e-4f);
            expect(toValueTree(a.getProject()).isEquivalentTo(toValueTree(b.getProject())));
            expectEquals(b.getProject().selectedLayer, 3);

            const auto outA = renderWith(a);
            const auto outB = renderWith(b);
            expect(compareAligned(outB, outA).bitExact, "restored instance sounds different");
        }

        beginTest("unknown schema versions and garbage are rejected without touching state");
        {
            auto tree = toValueTree(makeRichProject());
            tree.setProperty("schemaVersion", kStateSchemaVersion + 1, nullptr);
            juce::String error;
            expect(!fromValueTree(tree, &error).has_value());
            expect(error.contains("unsupported"));

            tree.removeProperty("schemaVersion", nullptr);
            expect(!fromValueTree(tree).has_value());

            auto pPtr = std::make_unique<FxPluginProcessor>();

            auto& p = *pPtr;
            p.setProject(makeRichProject());
            const auto before = toValueTree(p.getProject());
            const char garbage[] = "definitely not a plugin state";
            p.setStateInformation(garbage, static_cast<int>(sizeof(garbage)));
            p.setStateInformation(nullptr, 0);
            expect(before.isEquivalentTo(toValueTree(p.getProject())));
        }

        beginTest("sanitize enforces limits and repairs invalid values");
        {
            FxProjectState project;
            auto& layer = project.layers[0];
            layer.gainDb = std::numeric_limits<float>::quiet_NaN();
            for (int i = 0; i < kMaxEqNodesPerLayer + 5; ++i)
                layer.eqNodes.push_back({});
            layer.eqNodes[0].frequencyHz = -5.0;
            layer.eqNodes[1].slopeDbPerOct = 50;
            FxNodeState node;
            node.lowHz = 5000.0;
            node.highHz = 100.0;
            node.chain.resize(9);
            for (int i = 0; i < kMaxFxNodesPerLayer + 3; ++i)
                layer.fxNodes.push_back(node);
            layer.fxNodes[1].chain.clear();
            project.selectedLayer = 42;

            sanitize(project);
            expectEquals(layer.gainDb, 0.0f);
            expectEquals(static_cast<int>(layer.eqNodes.size()), kMaxEqNodesPerLayer);
            expectEquals(layer.eqNodes[0].frequencyHz, kMinFrequencyHz);
            expectEquals(layer.eqNodes[1].slopeDbPerOct, 48);
            expectEquals(static_cast<int>(layer.fxNodes.size()), kMaxFxNodesPerLayer);
            expectEquals(static_cast<int>(layer.fxNodes[0].chain.size()), kMaxFxPerNode);
            expectEquals(static_cast<int>(layer.fxNodes[1].chain.size()), 1);
            expect(layer.fxNodes[0].lowHz < layer.fxNodes[0].highHz);
            expectEquals(project.selectedLayer, kNumLayers - 1);

            const auto once = toValueTree(project);
            sanitize(project);
            expect(once.isEquivalentTo(toValueTree(project)), "sanitize is not idempotent");
        }

        beginTest("all ten layers exist with stable indices");
        {
            const auto tree = toValueTree(FxProjectState{});
            int layers = 0;
            for (const auto& child : tree)
                if (child.hasType("Layer"))
                    expectEquals(static_cast<int>(child["index"]), layers++);
            expectEquals(layers, kNumLayers);
        }
    }
};

static StateTests stateTests;
} // namespace hpeq::test
