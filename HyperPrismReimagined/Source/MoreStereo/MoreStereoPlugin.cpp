//==============================================================================
// HyperPrism Revived - More Stereo Plugin Entry Point
//==============================================================================

#include "MoreStereoProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MoreStereoProcessor();
}