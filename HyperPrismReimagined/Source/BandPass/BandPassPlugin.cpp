//==============================================================================
// HyperPrism Revived - Band-Pass Filter Plugin Entry Point
//==============================================================================

#include "BandPassProcessor.h"

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BandPassProcessor();
}