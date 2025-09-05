//==============================================================================
// HyperPrism Revived - Pitch Changer Plugin Entry Point
//==============================================================================

#include "PitchChangerProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchChangerProcessor();
}