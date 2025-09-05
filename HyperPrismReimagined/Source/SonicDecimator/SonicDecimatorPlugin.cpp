//==============================================================================
// HyperPrism Revived - Sonic Decimator Plugin Entry Point
//==============================================================================

#include "SonicDecimatorProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SonicDecimatorProcessor();
}