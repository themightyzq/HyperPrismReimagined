//==============================================================================
// HyperPrism Revived - Delay Plugin Entry Point
//==============================================================================

#include "DelayProcessor.h"

// This creates the plugin instance for JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayProcessor();
}