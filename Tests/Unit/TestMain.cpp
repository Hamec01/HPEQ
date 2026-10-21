#include <iostream>

#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Harness/RtAllocationProbe.h"

namespace
{
// JUCE's runner logs to the debugger only; tests must leave a readable console log.
class ConsoleRunner final : public juce::UnitTestRunner
{
    void logMessage(const juce::String& message) override
    {
        std::cout << message << std::endl;
    }
};
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit; // message thread for editor tests

    ConsoleRunner runner;
    runner.setAssertOnFailure(false);
    runner.setPassesAreLogged(false);
    runner.runTestsInCategory("HPEQ");

    int failures = 0, passes = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* result = runner.getResult(i);
        failures += result->failures;
        passes += result->passes;
    }
    std::cout << "\nallocation probe: " << hpeq::test::allocationCoverageName() << "\n"
              << "HPEQ_Tests: " << passes << " expectations passed, " << failures << " failed\n";
    return failures == 0 ? 0 : 1;
}
