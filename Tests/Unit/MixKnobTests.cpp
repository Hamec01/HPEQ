// Owner, 2026-10-25: every module page has a MIX knob. Comp, Reverb, Delay and Limiter got one; it is
// the module's mix (FxSlotState::mix: parallel for inserts, the send amount for Reverb / Delay), not a
// parameter of the processor.

#include <memory>

#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/HamloCompPanel.h"
#include "UI/HamloEchoPanel.h"
#include "UI/HamloRedWallPanel.h"
#include "UI/HamloRevolutionPanel.h"
#include "UI/HamloVerbPanel.h"

namespace hpeq::test
{
namespace
{
template <typename T>
T* findChild(juce::Component* root)
{
    juce::Array<juce::Component*> stack { root };
    while (!stack.isEmpty())
    {
        auto* c = stack.removeAndReturn(stack.size() - 1);
        for (auto* child : c->getChildren())
            stack.add(child);
        if (auto* t = dynamic_cast<T*>(c))
            return t;
    }
    return nullptr;
}
} // namespace

class MixKnobTests final : public juce::UnitTest
{
public:
    MixKnobTests() : juce::UnitTest("Mix knobs", "HPEQ") {}

    template <typename Panel>
    void check(FxType type, const juce::String& name)
    {
        beginTest(name + ": the MIX knob sets the module's mix (not a parameter), starts at the module's default and comes back from the project");
        auto plugin = std::make_unique<FxPluginProcessor>();
        FxProjectState p;
        FxNodeState n;
        n.id = 1;
        n.chain = { defaultModule(type) };
        p.layers[0].fxNodes = { n };
        p.nextNodeId = 2;
        plugin->setProject(p);
        auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
        editor->setSize(1400, 900);
        editor->selectNode(true, 1, 0);
        auto* panel = findChild<Panel>(editor.get());
        expect(panel != nullptr, "no page");
        if (panel == nullptr)
            return;
        // a new reverb / delay starts with MIX 0 (owner 2026-10-26: nothing added until it is raised), the others at 100 %
        expectWithinAbsoluteError(panel->getValues().knobs[Panel::Mix], static_cast<double>(defaultModule(type).mix), 1.0e-9);
        panel->onKnobChange(Panel::Mix, 0.35);
        const auto project = plugin->getProject();
        const auto& m = project.layers[0].fxNodes[0].chain[0];
        expectWithinAbsoluteError(static_cast<double>(m.mix), 0.35, 1.0e-6);
        expect(!m.parameters.contains("mix"), "mix must not become a processor parameter");
        editor->selectNode(true, 1, 0);
        auto* rebuilt = findChild<Panel>(editor.get());
        expect(rebuilt != nullptr && std::abs(rebuilt->getValues().knobs[Panel::Mix] - 0.35) < 1.0e-6);
        expect(Panel::format(Panel::Mix, 0.35) == "35 %");
    }

    void runTest() override
    {
        check<ui::HamloCompPanel>(FxType::Compressor, "HamloComp");
        check<ui::HamloVerbPanel>(FxType::Reverb, "HamloVerb");
        check<ui::HamloEchoPanel>(FxType::Delay, "HamloEcho");
        check<ui::HamloRedWallPanel>(FxType::Limiter, "HamloRedWall");
        check<ui::HamloRevolutionPanel>(FxType::MonstaFx, "HamloRevolution");
    }
};

static MixKnobTests mixKnobTests;
} // namespace hpeq::test
