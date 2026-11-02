#include "Standalone/ModulePluginProcessor.h"

#ifndef HPEQ_MODULE_KIND
#error HPEQ_MODULE_KIND must select one hpeq::standalone::ModuleKind value
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new hpeq::standalone::ModulePluginProcessor(
        static_cast<hpeq::standalone::ModuleKind>(HPEQ_MODULE_KIND));
}
