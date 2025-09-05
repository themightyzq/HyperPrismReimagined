//==============================================================================
// HyperPrism Revived - Echo Plugin Entry Point
//==============================================================================

#include "EchoProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EchoProcessor();
}