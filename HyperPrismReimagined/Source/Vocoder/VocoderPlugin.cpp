//==============================================================================
// HyperPrism Revived - Vocoder Plugin Entry Point
//==============================================================================

#include "VocoderProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocoderProcessor();
}