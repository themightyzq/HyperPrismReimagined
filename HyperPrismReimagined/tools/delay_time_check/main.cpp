// hp_delay_time_<Effect>: proves a fixed delay inside the plugin lasts the same number of
// milliseconds at every sample rate. An impulse (both channels) is rendered at 44.1, 48, 96 and
// 192 kHz, and the largest output peak inside a search window is located per channel.
//   HP_CHECK_MORESTEREO: Ambience 100 %, Width 100 %, Bass Mono 0, Stereo Enhance 0. The
//      ambience path adds a delayed copy of the input 3 ms later on the left and 7 ms later on
//      the right (window 1-20 ms). Those delays were written as 3 * 48 and 7 * 48 samples, so
//      they were only right at 48 kHz (1.5 / 3.5 ms at 96 kHz, 3.27 / 7.62 ms at 44.1 kHz).
//   HP_CHECK_QUASISTEREO: Delay Time 50 ms (its maximum), Width 100 %, Frequency Shift 0, Phase
//      Shift 0, High Freq Enhance 0. The delayed copy must sit 50 ms after the impulse (window
//      5-80 ms). The delay line held 4800 samples, so at 192 kHz the delay stopped at 25 ms.
// Each peak must be within 1 sample of the expected time. Exit 0 on pass. Registered with
// CTest in CMakeLists.txt (add_hyperprism_light_check).

#include HP_CHECK_PROCESSOR_HEADER
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
int failures = 0;

void setParam (juce::AudioProcessor& proc, const juce::String& id, float rawValue)
{
    for (auto* p : proc.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (r->paramID == id)
            {
                r->setValueNotifyingHost (r->convertTo0to1 (rawValue));
                return;
            }
    std::cerr << "FAIL unknown parameter " << id << "\n";
    ++failures;
}

void configure (juce::AudioProcessor& p)
{
#if HP_CHECK_MORESTEREO
    setParam (p, "ambience", 100.0f);
    setParam (p, "width", 100.0f);
    setParam (p, "bassMono", 0.0f);
    setParam (p, "stereoEnhance", 0.0f);
    setParam (p, "outputLevel", 0.0f);
#elif HP_CHECK_QUASISTEREO
    setParam (p, "delayTime", 50.0f);
    setParam (p, "width", 100.0f);
    setParam (p, "frequencyShift", 0.0f);
    setParam (p, "phaseShift", 0.0f);
    setParam (p, "highFreqEnhance", 0.0f);
    setParam (p, "outputLevel", 0.0f);
#else
    #error "define HP_CHECK_MORESTEREO or HP_CHECK_QUASISTEREO"
#endif
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

#if HP_CHECK_MORESTEREO
    const double expectedMs[2] = { 3.0, 7.0 };
    const double windowMs[2] = { 1.0, 20.0 };
#else
    const double expectedMs[2] = { 50.0, 50.0 };
    const double windowMs[2] = { 5.0, 80.0 };
#endif

    for (double sr : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        HP_CHECK_PROCESSOR_CLASS proc;
        configure (proc);
        const int block = 512;
        proc.setPlayConfigDetails (2, 2, sr, block);
        proc.prepareToPlay (sr, block);

        const int length = (int) (0.1 * sr);
        std::vector<float> out[2] = { std::vector<float> ((size_t) length), std::vector<float> ((size_t) length) };
        juce::AudioBuffer<float> buffer (2, block);
        juce::MidiBuffer midi;
        for (int start = 0; start < length; start += block)
        {
            const int n = juce::jmin (block, length - start);
            buffer.setSize (2, n, false, false, true);
            buffer.clear();
            if (start == 0)
            {
                buffer.setSample (0, 0, 0.5f);
                buffer.setSample (1, 0, 0.5f);
            }
            proc.processBlock (buffer, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < n; ++i)
                    out[ch][(size_t) (start + i)] = buffer.getSample (ch, i);
        }

        for (int ch = 0; ch < 2; ++ch)
        {
            const int from = (int) (windowMs[0] * 0.001 * sr);
            const int to = juce::jmin (length, (int) (windowMs[1] * 0.001 * sr));
            int peakAt = from;
            for (int i = from; i < to; ++i)
                if (std::abs (out[ch][(size_t) i]) > std::abs (out[ch][(size_t) peakAt]))
                    peakAt = i;
            const double ms = peakAt * 1000.0 / sr;
            const double expectedSamples = expectedMs[ch] * 0.001 * sr;
            const bool pass = std::abs (peakAt - expectedSamples) <= 1.0 && std::abs (out[ch][(size_t) peakAt]) > 1.0e-3f;
            std::cout << (pass ? "ok   " : "FAIL ") << sr << " Hz " << (ch == 0 ? "left " : "right")
                      << ": delayed copy at " << ms << " ms (expected " << expectedMs[ch] << " ms, peak "
                      << out[ch][(size_t) peakAt] << ")\n";
            if (! pass)
                ++failures;
        }
    }

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " delay time check\n";
    return failures == 0 ? 0 : 1;
}
