//==============================================================================
// HyperPrism Revived - Auto Pan Plugin Entry Point
//==============================================================================

#include "AutoPanProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoPanProcessor();
}