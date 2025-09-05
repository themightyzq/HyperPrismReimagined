#include "CompressorProcessor.h"

// This creates the plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorProcessor();
}