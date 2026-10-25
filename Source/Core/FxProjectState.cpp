#include "Core/FxProjectState.h"

#include "Core/Revolution.h"

#include <charconv>

#include <cmath>

namespace hpeq
{
namespace
{
namespace ids
{
const juce::Identifier project { "HpeqProject" };
const juce::Identifier schemaVersion { "schemaVersion" };
const juce::Identifier routing { "routing" };
const juce::Identifier selectedLayer { "selectedLayer" };
const juce::Identifier nextNodeId { "nextNodeId" };
const juce::Identifier layer { "Layer" };
const juce::Identifier eqNode { "EqNode" };
const juce::Identifier fxNode { "FxNode" };
const juce::Identifier slot { "FxSlot" };
const juce::Identifier engineSlot { "engineSlot" };
const juce::Identifier mixChain { "MixChain" };
const juce::Identifier params { "Params" };
const juce::Identifier index { "index" };
const juce::Identifier id { "id" };
const juce::Identifier name { "name" };
const juce::Identifier enabled { "enabled" };
const juce::Identifier mute { "mute" };
const juce::Identifier solo { "solo" };
const juce::Identifier gainDb { "gainDb" };
const juce::Identifier mix { "mix" };
const juce::Identifier pan { "pan" };
const juce::Identifier width { "width" };
const juce::Identifier channel { "channel" };
const juce::Identifier shape { "shape" };
const juce::Identifier frequencyHz { "frequencyHz" };
const juce::Identifier q { "q" };
const juce::Identifier slope { "slopeDbPerOct" };
const juce::Identifier dynamic { "dynamic" };
const juce::Identifier dynThresholdDb { "dynThresholdDb" };
const juce::Identifier dynRatio { "dynRatio" };
const juce::Identifier dynAttackMs { "dynAttackMs" };
const juce::Identifier dynReleaseMs { "dynReleaseMs" };
const juce::Identifier dynRangeDb { "dynRangeDb" };
const juce::Identifier lowHz { "lowHz" };
const juce::Identifier highHz { "highHz" };
const juce::Identifier type { "type" };
} // namespace ids

// Enums are stored by name, never by ordinal, so reordering an enum cannot corrupt saved projects.
template <typename Enum, size_t N>
struct EnumNames
{
    std::array<std::pair<Enum, const char*>, N> entries;

    juce::String name(Enum value) const
    {
        for (const auto& [e, text] : entries)
            if (e == value)
                return text;
        return entries.front().second;
    }

    Enum parse(const juce::var& text, Enum fallback) const
    {
        for (const auto& [e, entryText] : entries)
            if (text.toString() == entryText)
                return e;
        return fallback;
    }
};

const EnumNames<ChannelMode, 7> channelNames { { { { ChannelMode::Stereo, "Stereo" },
                                                   { ChannelMode::Left, "Left" },
                                                   { ChannelMode::Right, "Right" },
                                                   { ChannelMode::Mid, "Mid" },
                                                   { ChannelMode::Side, "Side" },
                                                   { ChannelMode::LinkedStereo, "LinkedStereo" },
                                                   { ChannelMode::DualMono, "DualMono" } } } };

const EnumNames<EqShape, 9> shapeNames { { { { EqShape::Bell, "Bell" },
                                             { EqShape::LowShelf, "LowShelf" },
                                             { EqShape::HighShelf, "HighShelf" },
                                             { EqShape::LowCut, "LowCut" },
                                             { EqShape::HighCut, "HighCut" },
                                             { EqShape::Notch, "Notch" },
                                             { EqShape::BandPass, "BandPass" },
                                             { EqShape::Tilt, "Tilt" },
                                             { EqShape::AllPass, "AllPass" } } } };

const EnumNames<FxType, 10> fxNames { { { { FxType::Compressor, "Compressor" },
                                         { FxType::Limiter, "Limiter" },
                                         { FxType::Reverb, "Reverb" },
                                         { FxType::Delay, "Delay" },
                                         { FxType::TransientShaper, "TransientShaper" },
                                         { FxType::Saturator, "Saturator" },
                                         { FxType::MonstaFx, "MonstaFx" },
                                         { FxType::StereoProcessor, "StereoProcessor" },
                                         { FxType::GateExpander, "GateExpander" },
                                         { FxType::Gain, "Gain" } } } };

const EnumNames<LayerRouting, 2> routingNames { { { { LayerRouting::Parallel, "Parallel" },
                                                    { LayerRouting::Serial, "Serial" } } } };

constexpr int kAllowedSlopes[] { 6, 12, 18, 24, 36, 48, 72, 96 };

template <typename T>
T clampFinite(T value, T low, T high, T fallback)
{
    return std::isfinite(static_cast<double>(value)) ? juce::jlimit(low, high, value) : fallback;
}

int nearestAllowedSlope(int slope)
{
    int best = kAllowedSlopes[0];
    for (int s : kAllowedSlopes)
        if (std::abs(s - slope) < std::abs(best - slope))
            best = s;
    return best;
}

// Exact text -> double (std::from_chars rounds correctly). JUCE's own parser can be one ulp off, so a
// load + save cycle drifted (found by the M8 T11 host test); JUCE writes enough digits (>= 17).
double exactDouble(const juce::var& value)
{
    if (value.isString())
    {
        const auto text = value.toString().trim().toStdString();
        double result = 0.0;
        const auto* begin = text.data();
        const auto* end = text.data() + text.size();
        if (!text.empty() && text.front() == '+')
            ++begin;
        if (const auto r = std::from_chars(begin, end, result); r.ec == std::errc() && r.ptr == end)
            return result;
    }
    return static_cast<double>(value);
}

// Shortest text that reads back as exactly the same double (std::to_chars). JUCE's writer uses 15-16
// significant digits in some ranges, which is not enough for an exact save / load cycle.
juce::var exactText(double value)
{
    char buffer[64];
    const auto r = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return juce::String(buffer, static_cast<size_t>(r.ptr - buffer));
}

// A processor parameter as text: integers stay integers (so "2" is saved as "2" again), anything else
// is an exactly parsed double.
juce::var exactNumber(const juce::var& value)
{
    const auto text = value.toString().trim().toStdString();
    long long whole = 0;
    if (const auto r = std::from_chars(text.data(), text.data() + text.size(), whole); !text.empty() && r.ec == std::errc() && r.ptr == text.data() + text.size())
        return juce::var(static_cast<juce::int64>(whole));
    return juce::var(exactDouble(value));
}

double readDouble(const juce::ValueTree& tree, const juce::Identifier& key, double fallback)
{
    return tree.hasProperty(key) ? exactDouble(tree[key]) : fallback;
}

juce::ValueTree slotToTree(const FxSlotState& slot)
{
    juce::ValueTree tree(ids::slot);
    tree.setProperty(ids::type, fxNames.name(slot.type), nullptr);
    tree.setProperty(ids::enabled, slot.enabled, nullptr);
    tree.setProperty(ids::mix, exactText(slot.mix), nullptr);
    juce::ValueTree params(ids::params);
    for (const auto& p : slot.parameters)
        params.setProperty(p.name, p.value.isDouble() ? exactText(static_cast<double>(p.value)) : p.value, nullptr);
    tree.appendChild(params, nullptr);
    return tree;
}

FxSlotState slotFromTree(const juce::ValueTree& tree)
{
    FxSlotState slot;
    slot.type = fxNames.parse(tree[ids::type], FxType::Compressor);
    slot.enabled = tree.getProperty(ids::enabled, true);
    slot.mix = static_cast<float>(readDouble(tree, ids::mix, 1.0));
    const auto params = tree.getChildWithName(ids::params);
    for (int i = 0; i < params.getNumProperties(); ++i)
    {
        const auto key = params.getPropertyName(i);
        const auto& value = params[key];
        slot.parameters.set(key, value.isString() ? exactNumber(value) : value);
    }
    return slot;
}
} // namespace

std::array<LayerState, kNumLayers> FxProjectState::makeDefaultLayers()
{
    std::array<LayerState, kNumLayers> layers;
    for (int i = 0; i < kNumLayers; ++i)
        layers[static_cast<size_t>(i)].name = "Layer " + juce::String(i + 1);
    return layers;
}

void copyLayerInto(FxProjectState& project, const LayerState& source, int targetIndex)
{
    auto& target = project.layers[static_cast<size_t>(juce::jlimit(0, kNumLayers - 1, targetIndex))];
    target = source;
    for (auto& n : target.eqNodes)
        n.id = project.nextNodeId++;
    for (auto& n : target.fxNodes)
        n.id = project.nextNodeId++;
}

FxSlotState defaultModule(FxType type)
{
    FxSlotState m;
    m.type = type;
    if (type == FxType::Limiter)
    {
        // HamloRedWall as the UI creates it: true peak and auto release on, a safe ceiling.
        m.parameters.set("thresholdDb", 0);
        m.parameters.set("ceilingDb", -1);
        m.parameters.set("releaseMs", 60);
        m.parameters.set("lookaheadMs", 2);
        m.parameters.set("truePeak", 1);
        m.parameters.set("autoRelease", 1);
        m.parameters.set("style", 0);
        m.parameters.set("link", 1);
        m.parameters.set("monitor", 0);
        m.parameters.set("outputDb", 0);
    }
    if (type == FxType::StereoProcessor)
    {
        // HamloShirota as the UI creates it: EDINSTVO, neutral width, SAFE on (the mono sum never falls apart).
        m.parameters.set("style", 0);
        m.parameters.set("width", 1.0);
        m.parameters.set("balance", 0.0);
        m.parameters.set("stereoize", 0.5);
        m.parameters.set("space", 0.5);
        m.parameters.set("monoBassHz", 0);
        m.parameters.set("rotation", 0.0);
        m.parameters.set("safe", 1);
        m.parameters.set("monoListen", 0);
        m.parameters.set("outputDb", 0);
    }
    if (type == FxType::GateExpander)
    {
        // HamloZastava as the UI creates it: the M6 gate settings with VOSTOK look-ahead (2 ms).
        m.parameters.set("thresholdDb", -40);
        m.parameters.set("ratio", 4);
        m.parameters.set("rangeDb", 40);
        m.parameters.set("attackMs", 0.5);
        m.parameters.set("holdMs", 20);
        m.parameters.set("releaseMs", 100);
        m.parameters.set("hysteresisDb", 4);
        m.parameters.set("style", 0);
        m.parameters.set("lookahead", 1);
        m.parameters.set("keyHpHz", 20);
        m.parameters.set("keyLpHz", 20000);
        m.parameters.set("keyListen", 0);
        m.parameters.set("flip", 0);
        m.parameters.set("outputDb", 0);
    }
    if (type == FxType::MonstaFx)
    {
        // HamloRevolution as the UI creates it: sync, 1/16, 16 steps, order T-G-M, a Musical pattern from
        // seed 1 (projects without "mode" keep the M6 Monsta).
        m.parameters.set("mode", 1);
        m.parameters.set("order", 0);
        m.parameters.set("character", static_cast<int>(revolution::Musical));
        m.parameters.set("grid", 2);
        m.parameters.set("triplet", 0);
        m.parameters.set("swing", 0.0);
        m.parameters.set("steps", 16);
        m.parameters.set("free", 0);
        m.parameters.set("freeMs", 125.0);
        m.parameters.set("humanize", 0.0);
        m.parameters.set("revSeed", 1);
        for (const auto* name : { "time", "glitch", "motion" })
        {
            const juce::String e(name);
            m.parameters.set(e + "On", 1);
            m.parameters.set(e + "Mix", 1.0);
            m.parameters.set(e + "Depth", e == "motion" ? 0.6 : 1.0);
        }
        m.parameters.set("motionTarget", static_cast<int>(revolution::LowPass));
        m.parameters.set("motionSmooth", 0.3);
        m.parameters.set("fadeMs", 2.0);
        m.parameters.set("timeStretch", 1); // RHYTHM: half time keeps the pitch
        m.parameters.set("smartJoin", 1);
        m.parameters.set("listen", 0);
        revolution::writePattern(m.parameters, revolution::generate({}, 1, revolution::Musical));
    }
    if (type == FxType::TransientShaper)
    {
        // HamloUdarnik as the UI creates it: PRAVDA, VOSTOK look-ahead (2 ms), groove kept; neutral otherwise.
        m.parameters.set("attackDb", 0);
        m.parameters.set("sustainDb", 0);
        m.parameters.set("speed", 0.5);
        m.parameters.set("style", 0);
        m.parameters.set("lookahead", 1);
        m.parameters.set("rise", 1);
        m.parameters.set("decay", 1);
        m.parameters.set("sensitivity", 1);
        m.parameters.set("preserve", 1);
        m.parameters.set("listen", 0);
        m.parameters.set("outputDb", 0);
    }
    if (type == FxType::Compressor)
    {
        m.parameters.set("thresholdDb", -30);
        m.parameters.set("ratio", 4);
        m.parameters.set("attackMs", 1); // module pass: 5 ms let drum hits through (band crest -1.1 dB only)
        m.parameters.set("releaseMs", 80);
        m.parameters.set("character", 1);
        m.parameters.set("autoMakeup", 1);
    }
    else if (type == FxType::Reverb)
    {
        // HamloVerb pass: a reverb added from the UI starts in the Hall space (projects without "space"
        // keep the M4 reverb, Classic).
        m.parameters.set("space", 3);
        m.parameters.set("color", 2);
        m.parameters.set("predelayMs", 20);
        m.parameters.set("decaySeconds", 2.4);
        m.parameters.set("size", 1.0);
        m.parameters.set("hfDecayRatio", 0.5);
        m.parameters.set("lowCutHz", 80);
        m.parameters.set("width", 1.0);
    }
    else if (type == FxType::Saturator)
    {
        // HamloMag pass: a saturator added from the UI starts as a tape machine at 19 cm/s, 2x oversampled
        // (projects without "model" keep the M6 saturator, Classic).
        m.parameters.set("model", 1);
        m.parameters.set("driveDb", 6.0);
        m.parameters.set("oversampling", 1);
        m.parameters.set("autoGain", 1);
    }
    else if (type == FxType::Delay)
    {
        // HamloEcho pass: a delay added from the UI starts as a tape echo, 1/4 synced (projects without
        // "character" keep the M6 delay, Classic).
        m.parameters.set("character", 2);
        m.parameters.set("sync", 8);
        m.parameters.set("timeMs", 250.0);
        m.parameters.set("feedback", 0.35);
        m.parameters.set("dampingHz", 8000.0);
        m.parameters.set("lowCutHz", 120.0);
        m.parameters.set("width", 1.0);
    }
    return m;
}

std::pair<double, double> pointModuleBandHz(const EqNodeState& node)
{
    const double q = std::max(0.025, node.q);
    const double half = 1.0 / (2.0 * q);
    const double root = std::sqrt(1.0 + half * half);
    return { node.frequencyHz * (root - half), node.frequencyHz * (root + half) };
}

void assignEngineSlots(LayerState& layer)
{
    std::array<bool, kMaxFxNodesPerLayer> used {};
    const auto keep = [&used](int& slot)
    {
        if (slot >= 0 && slot < kMaxFxNodesPerLayer && !used[static_cast<size_t>(slot)])
            used[static_cast<size_t>(slot)] = true;
        else
            slot = -1;
    };
    for (auto& n : layer.fxNodes)
        keep(n.engineSlot);
    for (auto& n : layer.eqNodes)
    {
        if (n.chain.empty())
            n.engineSlot = -1;
        else
            keep(n.engineSlot);
    }
    const auto take = [&used]
    {
        for (int s = 0; s < kMaxFxNodesPerLayer; ++s)
            if (!used[static_cast<size_t>(s)])
            {
                used[static_cast<size_t>(s)] = true;
                return s;
            }
        return -1;
    };
    for (auto& n : layer.fxNodes)
        if (n.engineSlot < 0)
            n.engineSlot = take();
    for (auto& n : layer.eqNodes)
        if (!n.chain.empty() && n.engineSlot < 0)
            if ((n.engineSlot = take()) < 0)
                n.chain.clear();
}

int freeEngineSlots(const LayerState& layer)
{
    auto copy = layer;
    assignEngineSlots(copy);
    int used = 0;
    for (const auto& n : copy.fxNodes)
        used += n.engineSlot >= 0 ? 1 : 0;
    for (const auto& n : copy.eqNodes)
        used += n.engineSlot >= 0 ? 1 : 0;
    return kMaxFxNodesPerLayer - used;
}

bool layerHasContent(const LayerState& layer)
{
    return !layer.eqNodes.empty() || !layer.fxNodes.empty() || !layer.mixChain.empty() || layer.gainDb != 0.0f || layer.pan != 0.0f || layer.width != 1.0f
        || layer.channel != ChannelMode::Stereo;
}

void sanitize(FxProjectState& project)
{
    project.selectedLayer = juce::jlimit(0, kNumLayers - 1, project.selectedLayer);
    std::uint32_t maxId = 0;

    for (auto& layer : project.layers)
    {
        layer.gainDb = clampFinite(layer.gainDb, -48.0f, 24.0f, 0.0f);
        layer.mix = clampFinite(layer.mix, 0.0f, 1.0f, 1.0f);
        layer.pan = clampFinite(layer.pan, -1.0f, 1.0f, 0.0f);
        layer.width = clampFinite(layer.width, 0.0f, 2.0f, 1.0f);
        if (layer.name.isEmpty())
            layer.name = "Layer";

        if (layer.eqNodes.size() > static_cast<size_t>(kMaxEqNodesPerLayer))
            layer.eqNodes.resize(static_cast<size_t>(kMaxEqNodesPerLayer));
        for (auto& node : layer.eqNodes)
        {
            node.frequencyHz = clampFinite(node.frequencyHz, kMinFrequencyHz, kMaxFrequencyHz, 1000.0);
            node.gainDb = clampFinite(node.gainDb, -36.0, 36.0, 0.0);
            node.q = clampFinite(node.q, 0.025, 40.0, 0.707);
            node.slopeDbPerOct = nearestAllowedSlope(node.slopeDbPerOct);
            node.thresholdDb = clampFinite(node.thresholdDb, -60.0, 0.0, -30.0);
            node.ratio = clampFinite(node.ratio, 1.0, 20.0, 3.0);
            node.attackMs = clampFinite(node.attackMs, 0.1, 200.0, 10.0);
            node.releaseMs = clampFinite(node.releaseMs, 5.0, 2000.0, 120.0);
            node.rangeDb = clampFinite(node.rangeDb, -24.0, 24.0, -6.0);
            if (node.chain.size() > static_cast<size_t>(kMaxFxPerNode))
                node.chain.resize(static_cast<size_t>(kMaxFxPerNode));
            for (auto& slot : node.chain)
                slot.mix = clampFinite(slot.mix, 0.0f, 1.0f, 1.0f);
            maxId = std::max(maxId, node.id);
        }

        if (layer.fxNodes.size() > static_cast<size_t>(kMaxFxNodesPerLayer))
            layer.fxNodes.resize(static_cast<size_t>(kMaxFxNodesPerLayer));
        for (auto& node : layer.fxNodes)
        {
            node.lowHz = clampFinite(node.lowHz, kMinFrequencyHz, kMaxFrequencyHz, 250.0);
            node.highHz = clampFinite(node.highHz, kMinFrequencyHz, kMaxFrequencyHz, 700.0);
            if (node.highHz < node.lowHz)
                std::swap(node.lowHz, node.highHz);
            node.mix = clampFinite(node.mix, 0.0f, 1.0f, 1.0f);
            if (node.chain.empty())
                node.chain.push_back({});
            if (node.chain.size() > static_cast<size_t>(kMaxFxPerNode))
                node.chain.resize(static_cast<size_t>(kMaxFxPerNode));
            for (auto& slot : node.chain)
                slot.mix = clampFinite(slot.mix, 0.0f, 1.0f, 1.0f);
            maxId = std::max(maxId, node.id);
        }
        if (layer.mixChain.size() > static_cast<size_t>(kMaxFxPerNode))
            layer.mixChain.resize(static_cast<size_t>(kMaxFxPerNode));
        for (auto& slot : layer.mixChain)
            slot.mix = clampFinite(slot.mix, 0.0f, 1.0f, 1.0f);
        assignEngineSlots(layer);
    }
    project.nextNodeId = std::max(project.nextNodeId, maxId + 1);
}

juce::ValueTree toValueTree(const FxProjectState& project)
{
    juce::ValueTree root(ids::project);
    root.setProperty(ids::schemaVersion, kStateSchemaVersion, nullptr);
    root.setProperty(ids::routing, routingNames.name(project.routing), nullptr);
    root.setProperty(ids::selectedLayer, project.selectedLayer, nullptr);
    root.setProperty(ids::nextNodeId, static_cast<juce::int64>(project.nextNodeId), nullptr);

    for (int i = 0; i < kNumLayers; ++i)
    {
        const auto& layer = project.layers[static_cast<size_t>(i)];
        juce::ValueTree lt(ids::layer);
        lt.setProperty(ids::index, i, nullptr);
        lt.setProperty(ids::name, layer.name, nullptr);
        lt.setProperty(ids::enabled, layer.enabled, nullptr);
        lt.setProperty(ids::mute, layer.mute, nullptr);
        lt.setProperty(ids::solo, layer.solo, nullptr);
        lt.setProperty(ids::gainDb, exactText(layer.gainDb), nullptr);
        lt.setProperty(ids::mix, exactText(layer.mix), nullptr);
        lt.setProperty(ids::pan, exactText(layer.pan), nullptr);
        lt.setProperty(ids::width, exactText(layer.width), nullptr);
        lt.setProperty(ids::channel, channelNames.name(layer.channel), nullptr);

        for (const auto& node : layer.eqNodes)
        {
            juce::ValueTree nt(ids::eqNode);
            nt.setProperty(ids::id, static_cast<juce::int64>(node.id), nullptr);
            nt.setProperty(ids::enabled, node.enabled, nullptr);
            nt.setProperty(ids::shape, shapeNames.name(node.shape), nullptr);
            nt.setProperty(ids::frequencyHz, exactText(node.frequencyHz), nullptr);
            nt.setProperty(ids::gainDb, exactText(node.gainDb), nullptr);
            nt.setProperty(ids::q, exactText(node.q), nullptr);
            nt.setProperty(ids::slope, node.slopeDbPerOct, nullptr);
            nt.setProperty(ids::channel, channelNames.name(node.channel), nullptr);
            // EQ pass: written only when used, so older projects and their saved state stay as they were
            if (node.solo)
                nt.setProperty(ids::solo, true, nullptr);
            const EqNodeState plain;
            if (node.dynamic || node.thresholdDb != plain.thresholdDb || node.ratio != plain.ratio || node.attackMs != plain.attackMs
                || node.releaseMs != plain.releaseMs || node.rangeDb != plain.rangeDb)
            {
                nt.setProperty(ids::dynamic, node.dynamic, nullptr);
                nt.setProperty(ids::dynThresholdDb, exactText(node.thresholdDb), nullptr);
                nt.setProperty(ids::dynRatio, exactText(node.ratio), nullptr);
                nt.setProperty(ids::dynAttackMs, exactText(node.attackMs), nullptr);
                nt.setProperty(ids::dynReleaseMs, exactText(node.releaseMs), nullptr);
                nt.setProperty(ids::dynRangeDb, exactText(node.rangeDb), nullptr);
            }
            if (!node.chain.empty())
            {
                nt.setProperty(ids::engineSlot, node.engineSlot, nullptr);
                for (const auto& slot : node.chain)
                    nt.appendChild(slotToTree(slot), nullptr);
            }
            lt.appendChild(nt, nullptr);
        }

        for (const auto& node : layer.fxNodes)
        {
            juce::ValueTree nt(ids::fxNode);
            nt.setProperty(ids::id, static_cast<juce::int64>(node.id), nullptr);
            nt.setProperty(ids::enabled, node.enabled, nullptr);
            nt.setProperty(ids::solo, node.solo, nullptr);
            nt.setProperty(ids::lowHz, exactText(node.lowHz), nullptr);
            nt.setProperty(ids::highHz, exactText(node.highHz), nullptr);
            nt.setProperty(ids::mix, exactText(node.mix), nullptr);
            nt.setProperty(ids::channel, channelNames.name(node.channel), nullptr);
            nt.setProperty(ids::engineSlot, node.engineSlot, nullptr);
            for (const auto& slot : node.chain)
                nt.appendChild(slotToTree(slot), nullptr);
            lt.appendChild(nt, nullptr);
        }
        if (!layer.mixChain.empty())
        {
            juce::ValueTree mt(ids::mixChain);
            for (const auto& slot : layer.mixChain)
                mt.appendChild(slotToTree(slot), nullptr);
            lt.appendChild(mt, nullptr);
        }
        root.appendChild(lt, nullptr);
    }
    return root;
}

std::optional<FxProjectState> fromValueTree(const juce::ValueTree& root, juce::String* error)
{
    const auto fail = [error](juce::String why) -> std::optional<FxProjectState>
    {
        if (error != nullptr)
            *error = std::move(why);
        return std::nullopt;
    };

    if (!root.hasType(ids::project))
        return fail("not an HPEQ project tree");
    if (!root.hasProperty(ids::schemaVersion))
        return fail("missing schemaVersion");
    const int version = root[ids::schemaVersion];
    if (version < 1 || version > kStateSchemaVersion)
        return fail("unsupported schemaVersion " + juce::String(version));

    FxProjectState project;
    project.routing = routingNames.parse(root[ids::routing], LayerRouting::Parallel);
    project.selectedLayer = root.getProperty(ids::selectedLayer, 0);
    project.nextNodeId = static_cast<std::uint32_t>(static_cast<juce::int64>(root.getProperty(ids::nextNodeId, 1)));

    for (const auto& lt : root)
    {
        if (!lt.hasType(ids::layer))
            continue;
        const int index = lt.getProperty(ids::index, -1);
        if (index < 0 || index >= kNumLayers)
            continue;
        auto& layer = project.layers[static_cast<size_t>(index)];
        layer.name = lt.getProperty(ids::name, layer.name).toString();
        layer.enabled = lt.getProperty(ids::enabled, true);
        layer.mute = lt.getProperty(ids::mute, false);
        layer.solo = lt.getProperty(ids::solo, false);
        layer.gainDb = static_cast<float>(readDouble(lt, ids::gainDb, 0.0));
        layer.mix = static_cast<float>(readDouble(lt, ids::mix, 1.0));
        layer.pan = static_cast<float>(readDouble(lt, ids::pan, 0.0));
        layer.width = static_cast<float>(readDouble(lt, ids::width, 1.0));
        layer.channel = channelNames.parse(lt[ids::channel], ChannelMode::Stereo);

        for (const auto& nt : lt)
        {
            if (nt.hasType(ids::eqNode))
            {
                EqNodeState node;
                node.id = static_cast<std::uint32_t>(static_cast<juce::int64>(nt.getProperty(ids::id, 0)));
                node.enabled = nt.getProperty(ids::enabled, true);
                node.shape = shapeNames.parse(nt[ids::shape], EqShape::Bell);
                node.frequencyHz = readDouble(nt, ids::frequencyHz, 1000.0);
                node.gainDb = readDouble(nt, ids::gainDb, 0.0);
                node.q = readDouble(nt, ids::q, 0.707);
                node.slopeDbPerOct = nt.getProperty(ids::slope, 12);
                node.channel = channelNames.parse(nt[ids::channel], ChannelMode::Stereo);
                node.solo = nt.getProperty(ids::solo, false);
                node.dynamic = nt.getProperty(ids::dynamic, false);
                node.thresholdDb = readDouble(nt, ids::dynThresholdDb, -30.0);
                node.ratio = readDouble(nt, ids::dynRatio, 3.0);
                node.attackMs = readDouble(nt, ids::dynAttackMs, 10.0);
                node.releaseMs = readDouble(nt, ids::dynReleaseMs, 120.0);
                node.rangeDb = readDouble(nt, ids::dynRangeDb, -6.0);
                node.engineSlot = nt.getProperty(ids::engineSlot, -1);
                for (const auto& st : nt)
                    if (st.hasType(ids::slot))
                        node.chain.push_back(slotFromTree(st));
                layer.eqNodes.push_back(node);
            }
            else if (nt.hasType(ids::fxNode))
            {
                FxNodeState node;
                node.id = static_cast<std::uint32_t>(static_cast<juce::int64>(nt.getProperty(ids::id, 0)));
                node.enabled = nt.getProperty(ids::enabled, true);
                node.solo = nt.getProperty(ids::solo, false);
                node.lowHz = readDouble(nt, ids::lowHz, 250.0);
                node.highHz = readDouble(nt, ids::highHz, 700.0);
                node.mix = static_cast<float>(readDouble(nt, ids::mix, 1.0));
                node.channel = channelNames.parse(nt[ids::channel], ChannelMode::Stereo);
                node.engineSlot = nt.getProperty(ids::engineSlot, -1);
                node.chain.clear();
                for (const auto& st : nt)
                    if (st.hasType(ids::slot))
                        node.chain.push_back(slotFromTree(st));
                layer.fxNodes.push_back(std::move(node));
            }
            else if (nt.hasType(ids::mixChain))
            {
                for (const auto& st : nt)
                    if (st.hasType(ids::slot))
                        layer.mixChain.push_back(slotFromTree(st));
            }
        }
    }

    sanitize(project);
    return project;
}

juce::String toString(ChannelMode m) { return channelNames.name(m); }
juce::String toString(EqShape s) { return shapeNames.name(s); }
juce::String toString(FxType t) { return fxNames.name(t); }
juce::String toString(LayerRouting r) { return routingNames.name(r); }
} // namespace hpeq
