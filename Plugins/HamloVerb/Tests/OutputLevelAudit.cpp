#include "VerbLabHelpers.h"
#include "TestSignals.h"

namespace hamlo::test
{
using namespace hamlo::verblab;
class OutputLevelAudit final : public juce::UnitTest
{
public:
    OutputLevelAudit() : juce::UnitTest("Output level audit", "HamloVerb") {}
    void runTest() override
    {
        const auto input = sine(.1, 1000, -12);
        const double dryEnergy = energy(input,0,4096);
        beginTest("Modern Insert dry coefficient before predelay equals 1-MIX for all six characters");
        for(int character=1;character<=6;++character)
            for(double mix : {.25,.5,1.})
            {
                auto s=neutral(character,2,500);set(s,"routing",0);set(s,"mix",mix);
                auto p=processor(s);settle(*p);const auto y=render(*p,input);
                const double ratio=std::sqrt(energy(y,0,4096)/dryEnergy);
                logMessage("LEVEL character="+juce::String(character)+" MIX="+juce::String(mix,2)
                    +" dry_gain="+juce::String(ratio,7)+" dB="+(ratio>0?juce::String(20*std::log10(ratio),6):juce::String("-inf")));
                expectWithinAbsoluteError(ratio,1-mix,1e-5);
            }
        beginTest("Legacy adds wet over unity dry; Send is wet only; Output scales the full modern mix");
        {
            auto s=neutral(0,2,500);set(s,"routing",0);set(s,"mix",.5);
            auto p=processor(s);settle(*p);const auto legacy=render(*p,input);
            expectWithinAbsoluteError(std::sqrt(energy(legacy,0,4096)/dryEnergy),1.,1e-5);
            s=neutral(1,2,500);set(s,"mix",.5);auto send=processor(s);settle(*send);
            expectEquals(energy(render(*send,input),0,4096),0.);
            set(s,"routing",0);set(s,"outputDb",-6);auto trimmed=processor(s);settle(*trimmed);
            const double ratio=std::sqrt(energy(render(*trimmed,input),0,4096)/dryEnergy);
            logMessage("OUTPUT MIX=.5 Output=-6: "+juce::String(20*std::log10(ratio),6)+" dB vs input");
            expectWithinAbsoluteError(ratio,.5*std::pow(10.,-6./20),1e-5);
        }
        beginTest("Current program bank mixing and output ranges");
        {
            auto p=processor({});double minMix=1,maxMix=0,minOutput=12,maxOutput=-24;int modern=0,negative=0;
            for(int i=0;i<p->getNumPrograms();++i)
            {
                p->setCurrentProgram(i);if(p->parameterValue("engine")==0)continue;++modern;
                const double mix=p->parameterValue("mix"),out=p->parameterValue("outputDb");
                minMix=std::min(minMix,mix);maxMix=std::max(maxMix,mix);minOutput=std::min(minOutput,out);maxOutput=std::max(maxOutput,out);negative+=out<-.001;
            }
            logMessage("BANK programs="+juce::String(p->getNumPrograms())+" modern="+juce::String(modern)
                +" MIX="+juce::String(minMix,6)+".."+juce::String(maxMix,6)+" OUTPUT="+juce::String(minOutput,4)+".."+juce::String(maxOutput,4)
                +" negative_output="+juce::String(negative));
            expectEquals(p->getNumPrograms(),208);expectEquals(modern,200);
        }
    }
};static OutputLevelAudit outputLevelAudit;
}
