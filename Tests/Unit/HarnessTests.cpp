// The harness's own analytic checks live in `HPEQ_Harness selfcheck`; these guard the pieces the
// plugin tests below rely on.

#include <juce_core/juce_core.h>

#include "Harness/AudioMetrics.h"
#include "Harness/TestSignalFactory.h"

namespace hpeq::test
{
class HarnessTests final : public juce::UnitTest
{
public:
    HarnessTests() : juce::UnitTest("Harness", "HPEQ") {}

    void runTest() override
    {
        beginTest("compareAligned detects a one-ulp change");
        {
            auto x = makePinkNoise({ 48000.0, 2, 0.1, kFixtureSeed }, -20.0);
            auto y = x;
            y.setSample(1, 100, std::nextafter(y.getSample(1, 100), 1.0f));
            const auto r = compareAligned(y, x);
            expect(!r.bitExact);
            expect(r.errorDb < -100.0);
        }

        beginTest("fixture set is complete and deterministic");
        {
            const auto a = makeFixtureSet({ 44100.0, 2, 0.2, kFixtureSeed });
            const auto b = makeFixtureSet({ 44100.0, 2, 0.2, kFixtureSeed });
            expectEquals(static_cast<int>(a.size()), 26);
            for (size_t i = 0; i < a.size(); ++i)
                expect(compareAligned(a[i].audio, b[i].audio).bitExact, a[i].name);
        }
    }
};

static HarnessTests harnessTests;
} // namespace hpeq::test
