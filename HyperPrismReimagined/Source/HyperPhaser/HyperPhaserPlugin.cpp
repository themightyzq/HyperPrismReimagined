//==============================================================================
// HyperPrism Revived - HyperPhaser Plugin Entry Point
//==============================================================================

#include "HyperPhaserProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HyperPhaserProcessor();
}