//==============================================================================
// HyperPrism Revived - Flanger Plugin Entry Point
//==============================================================================

#include "FlangerProcessor.h"

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FlangerProcessor();
}