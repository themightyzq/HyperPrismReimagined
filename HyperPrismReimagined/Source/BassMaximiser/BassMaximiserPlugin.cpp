//==============================================================================
// HyperPrism Revived - Bass Maximiser Plugin Entry Point
//==============================================================================

#include "BassMaximiserProcessor.h"

// This creates the plugin instance for JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassMaximiserProcessor();
}