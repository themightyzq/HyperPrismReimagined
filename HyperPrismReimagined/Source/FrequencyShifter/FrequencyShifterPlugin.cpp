//==============================================================================
// HyperPrism Revived - Frequency Shifter Plugin Entry Point
//==============================================================================

#include "FrequencyShifterProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FrequencyShifterProcessor();
}