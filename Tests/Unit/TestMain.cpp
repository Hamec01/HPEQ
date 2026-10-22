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

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit; // message thread for editor tests

    ConsoleRunner runner;
    runner.setAssertOnFailure(false);
    runner.setPassesAreLogged(false);
    if (argc > 1)
    {
        // Optional filter: run only the test classes whose name contains argv[1].
        juce::Array<juce::UnitTest*> selected;
        for (auto* test : juce::UnitTest::getTestsInCategory("HPEQ"))
            if (test->getName().containsIgnoreCase(argv[1]))
                selected.add(test);
        runner.runTests(selected);
    }
    else
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
