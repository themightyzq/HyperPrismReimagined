//==============================================================================
// HyperPrism Revived - Vibrato Plugin Entry Point
//==============================================================================

#include "VibratoProcessor.h"

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibratoProcessor();
}