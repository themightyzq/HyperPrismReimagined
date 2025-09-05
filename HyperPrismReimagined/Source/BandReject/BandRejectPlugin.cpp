//==============================================================================
// HyperPrism Revived - Band-Reject (Notch) Filter Plugin Entry Point
//==============================================================================

#include "BandRejectProcessor.h"

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BandRejectProcessor();
}