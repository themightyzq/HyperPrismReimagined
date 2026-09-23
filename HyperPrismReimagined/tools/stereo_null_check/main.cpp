// hyperprism_stereo_null_check_<PluginTarget>: feeds an identical mono signal to both
// channels of the plugin at a strong setting and asserts the two outputs are identical.
// Catches per-channel DSP state that is shared between channels (SonicDecimator had one
// decimator and one bit crusher processed channel by channel until 2026-09-23). Built per
// plugin by add_hyperprism_stereo_null_check() in CMakeLists.txt. Exit 0 on pass.

#include HP_CHECK_PROCESSOR_HEADER
#include <iostream>
#include <cmath>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    HP_CHECK_PROCESSOR_CLASS proc;
    const double sr = 48000.0;
    const int block = 512;
    proc.setPlayConfigDetails (2, 2, sr, block);
    proc.prepareToPlay (sr, block);

    // Strong decimation so any state leak between channels is large.
    for (auto* p : proc.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            const auto id = ranged->paramID;
            if (id == "bitDepth")   ranged->setValueNotifyingHost (ranged->convertTo0to1 (3.0f));
            if (id == "sampleRate") ranged->setValueNotifyingHost (ranged->convertTo0to1 (2900.0f));
            if (id == "dither")     ranged->setValueNotifyingHost (0.0f); // dither is random per channel by design
            if (id == "mix")        ranged->setValueNotifyingHost (1.0f);
        }

    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    juce::Random rng (1234);
    float maxDiff = 0.0f;
    double phase = 0.0;
    for (int b = 0; b < 40; ++b)
    {
        for (int i = 0; i < block; ++i)
        {
            const float v = 0.6f * (float) std::sin (phase) + 0.2f * (rng.nextFloat() * 2.0f - 1.0f);
            phase += juce::MathConstants<double>::twoPi * 997.0 / sr;
            buf.setSample (0, i, v);
            buf.setSample (1, i, v);
        }
        proc.processBlock (buf, midi);
        for (int i = 0; i < block; ++i)
            maxDiff = std::max (maxDiff, std::abs (buf.getSample (0, i) - buf.getSample (1, i)));
    }

    std::cout << "max |L-R| for identical input: " << maxDiff << "\n";
    const bool pass = maxDiff <= 1.0e-6f;
    std::cout << (pass ? "PASS" : "FAIL") << " stereo null check\n";
    return pass ? 0 : 1;
}
