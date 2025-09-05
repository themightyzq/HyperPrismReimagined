//==============================================================================
// HyperPrism Revived - Tremolo Plugin Entry Point
//==============================================================================

#include "TremoloProcessor.h"

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TremoloProcessor();
}