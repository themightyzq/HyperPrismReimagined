//==============================================================================
// HyperPrism Revived - Quasi Stereo Plugin Entry Point
//==============================================================================

#include "QuasiStereoProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QuasiStereoProcessor();
}