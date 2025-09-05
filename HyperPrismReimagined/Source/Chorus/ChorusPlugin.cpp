//==============================================================================
// HyperPrism Revived - Chorus Plugin Entry Point
//==============================================================================

#include "ChorusProcessor.h"

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChorusProcessor();
}