#include "Plugin/HamloMagProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new hamlo::saturator::HamloMagProcessor();
}
