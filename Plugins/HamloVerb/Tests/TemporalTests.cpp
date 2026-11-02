#include <juce_core/juce_core.h>
#include "DSP/VerbTemporalEffects.h"

namespace
{
class TemporalTests final : public juce::UnitTest
{
public:
    TemporalTests() : juce::UnitTest("Temporal signal contracts", "HamloVerb") {}
    void runTest() override
    {
        using namespace hpeq::dsp::detail;
        beginTest("Reverse Audio matches independently indexed reversed captures and overlap windows");
        for (const int length : { 2400, 12000, 96000 })
        {
            ReverseFragments effect; effect.prepare(48000);
            std::vector<double> input(static_cast<size_t>(length*4));
            for (size_t n=0; n<input.size(); ++n) input[n] = .1*std::sin(.013*n)+.03*std::sin(.071*n);
            double error=0;
            for (int n=0; n<static_cast<int>(input.size()); ++n)
            {
                const auto actual=effect.tick({input[static_cast<size_t>(n)],-input[static_cast<size_t>(n)]},length);
                double expected=0;
                for (int launch=length;launch<=n;launch+=length/2)
                {
                    const int age=n-launch;
                    if (age>=length) continue;
                    const double w=.5-.5*std::cos(2*pi*(age+.5)/length);
                    expected+=w*input[static_cast<size_t>(launch-1-age)];
                }
                error=std::max({error,std::abs(actual[0]-expected),std::abs(actual[1]+expected)});
            }
            expectWithinAbsoluteError(error,0.,1e-12,"fragment samples "+juce::String(length));
        }
        beginTest("Shimmer shifts a tone down/up an octave and up two octaves");
        for (const double ratio : { .5, 2., 4. })
        {
            PitchFeedback effect; effect.prepare(48000);
            double real=0,imag=0,power=0;
            constexpr int count=96000,start=24000;
            for (int n=0;n<count;++n)
            {
                const double input=.1*std::sin(2*pi*1000*n/48000);
                const double out=effect.tick({input,input},ratio,0,48000)[0];
                if(n>=start)
                {
                    const double phase=2*pi*(1000*ratio)*n/48000;
                    real+=out*std::cos(phase);imag+=out*std::sin(phase);power+=out*out;
                }
            }
            const double coherent=2*(real*real+imag*imag)/(count-start);
            expect(coherent/std::max(1e-30,power)>.98,"pitch ratio "+juce::String(ratio));
        }
    }
};
TemporalTests temporalTests;
}
