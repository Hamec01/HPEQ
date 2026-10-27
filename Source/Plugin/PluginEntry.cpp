#include "Plugin/HamloUdarnikProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new hamlo::udarnik::HamloUdarnikProcessor();
}
