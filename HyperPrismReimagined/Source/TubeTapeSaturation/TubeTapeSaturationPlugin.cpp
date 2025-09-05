//==============================================================================
// HyperPrism Revived - Tube/Tape Saturation Plugin Entry Point
//==============================================================================

#include "TubeTapeSaturationProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TubeTapeSaturationProcessor();
}