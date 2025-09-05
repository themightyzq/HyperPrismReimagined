//==============================================================================
// HyperPrism Revived - Phaser Plugin Entry Point
//==============================================================================

#include "PhaserProcessor.h"

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhaserProcessor();
}