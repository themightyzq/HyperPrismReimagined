#include "NoiseGateProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoiseGateProcessor();
}