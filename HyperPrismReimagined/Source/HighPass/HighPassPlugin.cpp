//==============================================================================
// HyperPrism Revived - High-Pass Filter Plugin Entry Point
//==============================================================================

#include "HighPassProcessor.h"

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HighPassProcessor();
}