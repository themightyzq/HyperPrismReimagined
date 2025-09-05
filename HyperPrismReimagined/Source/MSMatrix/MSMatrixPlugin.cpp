//==============================================================================
// HyperPrism Revived - M+S Matrix Plugin Entry Point
//==============================================================================

#include "MSMatrixProcessor.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MSMatrixProcessor();
}