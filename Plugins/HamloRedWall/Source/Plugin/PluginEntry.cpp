#include "Plugin/HamloRedWallProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new hamlo::redwall::HamloRedWallProcessor();
}
