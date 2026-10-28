#include "Plugin/HamloCompProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new hamlo::comp::HamloCompProcessor();
}
