//==============================================================================
// HyperPrism Revived - Pan Plugin Entry Point
//==============================================================================

#include "PanProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PanProcessor();
}