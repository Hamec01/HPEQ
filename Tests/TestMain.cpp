#include <iostream>

#include <juce_gui_basics/juce_gui_basics.h>

namespace
{
class ConsoleRunner final : public juce::UnitTestRunner
{
    void logMessage(const juce::String& message) override { std::cout << message << std::endl; }
};
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    ConsoleRunner runner;
    runner.setAssertOnFailure(false);
    runner.setPassesAreLogged(false);
    if (argc > 1)
    {
        // a filter: run the tests whose name contains the argument
        juce::Array<juce::UnitTest*> selected;
        for (auto* t : juce::UnitTest::getTestsInCategory("HamloFader"))
            if (t->getName().containsIgnoreCase(argv[1]))
                selected.add(t);
        runner.runTests(selected);
    }
    else
        runner.runTestsInCategory("HamloFader");

    int failures = 0, passes = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* result = runner.getResult(i);
        failures += result->failures;
        passes += result->passes;
    }
    std::cout << "\nHamloFader_Tests: " << passes << " expectations passed, " << failures << " failed\n";
    return failures == 0 ? 0 : 1;
}
