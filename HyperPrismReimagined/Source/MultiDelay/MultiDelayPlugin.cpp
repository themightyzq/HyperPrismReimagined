//==============================================================================
// HyperPrism Revived - Multi Delay Plugin Entry Point
//==============================================================================

#include "MultiDelayProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultiDelayProcessor();
}