//==============================================================================
// HyperPrism Revived - Single Delay Plugin Entry Point
//==============================================================================

#include "SingleDelayProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SingleDelayProcessor();
}