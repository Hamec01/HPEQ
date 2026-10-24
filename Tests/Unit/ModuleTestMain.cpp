#include <iostream>

#include <juce_gui_basics/juce_gui_basics.h>

namespace
{
class ConsoleRunner final : public juce::UnitTestRunner
{
    void logMessage(const juce::String& message) override { std::cout << message << std::endl; }
};
}
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    ConsoleRunner runner;
    runner.setAssertOnFailure(false);
    runner.setPassesAreLogged(false);
    auto selected = juce::UnitTest::getTestsInCategory("HPEQ Modules");
    runner.runTests(selected);

    int failures = 0, passes = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* result = runner.getResult(i);
        failures += result->failures;
        passes += result->passes;
    }
    std::cout << "\nHPEQ_ModuleTests: " << passes << " expectations passed, " << failures << " failed\n";
    return failures == 0 ? 0 : 1;
}

