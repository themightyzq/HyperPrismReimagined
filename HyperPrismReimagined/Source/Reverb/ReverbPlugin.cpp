//==============================================================================
// HyperPrism Revived - Reverb Plugin Entry Point
//==============================================================================

#include "ReverbProcessor.h"

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverbProcessor();
}