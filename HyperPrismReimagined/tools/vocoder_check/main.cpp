// hp_vocoder_Vocoder: proves the Vocoder's filter bank is built correctly. At the default
// 8 bands (centres log-spaced 80 Hz .. 8 kHz, so band 3 is at 576 Hz and band 4 at 1112 Hz):
//   1. Bandwidth: at 44.1 kHz a modulator tone at band 3's centre must give an output RMS above
//      1e-3, and a tone midway (geometric mean, 800 Hz) between band 3 and band 4 must come out
//      within 6 dB of it. Before the fix each band's bandwidth in Hz (hundreds) was passed as
//      the filter's Q, so the bands were a few Hz wide: the output was about 7e-5 RMS and the
//      in-between tone about 10 dB down.
//   2. Sample rate: the 1112 Hz band-centre tone must give the same output level (within 6 dB)
//      at 96 kHz as at 44.1 kHz. Before the fix the coefficients were built once, at 44.1 kHz,
//      and never rebuilt, so at 96 kHz every band sat at 2.18x its nominal frequency.
//   3. Stereo null: identical input on both channels gives identical outputs. Before the fix one
//      oscillator and one filter bank were shared by both channels.
// Exit 0 on pass. Registered with CTest in CMakeLists.txt (add_hyperprism_console_check).

#include HP_CHECK_PROCESSOR_HEADER
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
int failures = 0;

struct Render
{
    double rms = 0.0;        // channel 0, last half
    float maxLRDiff = 0.0f;  // over the whole render
};

Render render (double sampleRate, double toneHz)
{
    HP_CHECK_PROCESSOR_CLASS proc;
    const int block = 512;
    proc.setPlayConfigDetails (2, 2, sampleRate, block);
    proc.prepareToPlay (sampleRate, block);

    const int length = (int) sampleRate; // 1 s
    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;
    Render r;
    double sum = 0.0;
    int count = 0;

    for (int start = 0; start < length; start += block)
    {
        for (int i = 0; i < block; ++i)
        {
            const float v = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * toneHz * (start + i) / sampleRate);
            buffer.setSample (0, i, v);
            buffer.setSample (1, i, v);
        }
        proc.processBlock (buffer, midi);
        for (int i = 0; i < block; ++i)
        {
            const float l = buffer.getSample (0, i);
            r.maxLRDiff = juce::jmax (r.maxLRDiff, std::abs (l - buffer.getSample (1, i)));
            if (start + i >= length / 2)
            {
                sum += (double) l * l;
                ++count;
            }
        }
    }
    r.rms = std::sqrt (sum / juce::jmax (1, count));
    return r;
}

double db (double ratio) { return 20.0 * std::log10 (juce::jmax (1.0e-12, ratio)); }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    const double band3 = 80.0 * std::pow (100.0, 3.0 / 7.0);
    const double band4 = 80.0 * std::pow (100.0, 4.0 / 7.0);
    const double between = std::sqrt (band3 * band4);

    // 1. Bandwidth.
    {
        const auto centre = render (44100.0, band3);
        const auto mid = render (44100.0, between);
        const double rel = db (mid.rms / juce::jmax (1.0e-12, centre.rms));
        const bool pass = centre.rms > 1.0e-3 && rel > -6.0;
        std::cout << (pass ? "ok   " : "FAIL ") << "bandwidth: tone between bands 3 and 4 ("
                  << between << " Hz) is " << rel << " dB relative to band 3's centre (" << band3
                  << " Hz, output RMS " << centre.rms << ")\n";
        if (! pass)
            ++failures;
    }

    // 2. Sample rate, and 3. stereo null on the same renders.
    {
        const auto at44 = render (44100.0, band4);
        const auto at96 = render (96000.0, band4);
        const double rel = db (at96.rms / juce::jmax (1.0e-12, at44.rms));
        const bool pass = at44.rms > 1.0e-4 && std::abs (rel) < 6.0;
        std::cout << (pass ? "ok   " : "FAIL ") << "sample rate: " << band4 << " Hz tone at 96 kHz is "
                  << rel << " dB relative to 44.1 kHz (output RMS " << at44.rms << ")\n";
        if (! pass)
            ++failures;

        const float lr = juce::jmax (at44.maxLRDiff, at96.maxLRDiff);
        const bool nullPass = lr <= 1.0e-6f;
        std::cout << (nullPass ? "ok   " : "FAIL ") << "stereo null: max |L-R| for identical input " << lr << "\n";
        if (! nullPass)
            ++failures;
    }

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " vocoder check\n";
    return failures == 0 ? 0 : 1;
}
