#include "RingModulatorProcessor.h"

// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RingModulatorProcessor();
}