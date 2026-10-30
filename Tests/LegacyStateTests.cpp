#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Standalone/ModulePluginProcessor.h"

namespace
{
class LegacyStateTests final : public juce::UnitTest
{
public:
    LegacyStateTests() : juce::UnitTest("M0 state compatibility", "HamloEcho") {}

    void runTest() override
    {
        beginTest("All archived M0 states restore on fresh and previously edited instances");
        const auto fixtures = juce::File(__FILE__).getParentDirectory().getParentDirectory().getChildFile("out/m0/fixtures");
        const auto states = fixtures.findChildFiles(juce::File::findFiles, false, "*.state");
        if (states.isEmpty())
        {
            // Ordinary source checkouts do not contain the ignored machine-local binary archive.
            logMessage("SKIP: M0 archive missing; run EchoLab capture from the pinned M0 source first");
            return;
        }
        expectEquals(states.size(), 160, "Expected the complete original archive");
        for (const auto& file : states)
        {
            juce::MemoryBlock oldState;
            expect(file.loadFileAsData(oldState), file.getFileName());
            for (bool edited : { false, true })
            {
                auto p = std::make_unique<hpeq::standalone::ModulePluginProcessor>(hpeq::standalone::ModuleKind::Echo);
                if (edited)
                {
                    for (const auto& spec : p->getSpec().parameters) p->setParameterValue(spec.id, spec.maximum);
                    p->setParameterValue("bypass", 1);
                }
                p->setStateInformation(oldState.getData(), static_cast<int>(oldState.getSize()));
                juce::MemoryBlock restored;
                p->getStateInformation(restored);
                expect(restored == oldState, file.getFileName() + (edited ? " edited" : " fresh"));
            }
        }
    }
};
LegacyStateTests stateTests;
}
