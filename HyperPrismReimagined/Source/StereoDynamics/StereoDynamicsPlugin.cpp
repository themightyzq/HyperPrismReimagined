//==============================================================================
// HyperPrism Revived - Stereo Dynamics Plugin Entry Point
//==============================================================================

#include "StereoDynamicsProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StereoDynamicsProcessor();
}