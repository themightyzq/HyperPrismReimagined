//==============================================================================
// HyperPrism Revived - Low-Pass Filter Plugin Entry Point
//==============================================================================

#include "LowPassProcessor.h"

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LowPassProcessor();
}