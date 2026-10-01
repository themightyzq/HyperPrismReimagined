// hp_start_<Effect>: proves the plugin's parameter smoothers start at the current parameter
// values when playback starts, instead of ramping in from zero. Instance A is prepared and fed
// noise straight away; instance B is prepared, fed 1 s of silence (so every smoother has long
// settled), then fed the same noise. The first 300 ms of A and B must be identical
// (max difference <= 1e-6). HP_CHECK_ECHO: Echo's delay, feedback and mix smoothers started at
// 0 on every prepareToPlay, so each playback start began dry with a 0 ms delay gliding up to the
// set time. Exit 0 on pass. Registered with CTest in CMakeLists.txt (add_hyperprism_light_check).

#include HP_CHECK_PROCESSOR_HEADER
#include <cmath>
#include <iostream>
#include <vector>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

#if ! HP_CHECK_ECHO
    #error "define HP_CHECK_ECHO"
#endif

    constexpr double sr = 48000.0;
    constexpr int block = 512;
    const int length = (int) (0.3 * sr);

    std::vector<float> noise ((size_t) length);
    juce::Random rng (9);
    for (auto& v : noise)
        v = rng.nextFloat() * 0.8f - 0.4f;

    auto run = [&] (bool preRoll)
    {
        HP_CHECK_PROCESSOR_CLASS proc;
        proc.setPlayConfigDetails (2, 2, sr, block);
        proc.prepareToPlay (sr, block);
        juce::AudioBuffer<float> buffer (2, block);
        juce::MidiBuffer midi;
        if (preRoll)
            for (int b = 0; b < (int) sr / block; ++b)
            {
                buffer.clear();
                proc.processBlock (buffer, midi);
            }

        std::vector<float> out ((size_t) (2 * length));
        for (int start = 0; start < length; start += block)
        {
            const int n = juce::jmin (block, length - start);
            buffer.setSize (2, n, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < n; ++i)
                    buffer.setSample (ch, i, noise[(size_t) (start + i)]);
            proc.processBlock (buffer, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < n; ++i)
                    out[(size_t) (ch * length + start + i)] = buffer.getSample (ch, i);
        }
        return out;
    };

    const auto a = run (false);
    const auto b = run (true);
    float d = 0.0f;
    for (size_t i = 0; i < a.size(); ++i)
        d = juce::jmax (d, std::abs (a[i] - b[i]));

    const bool pass = d <= 1.0e-6f;
    std::cout << (pass ? "ok   " : "FAIL ") << "first 300 ms after prepareToPlay vs after 1 s of settled silence: max difference "
              << d << "\n";
    std::cout << (pass ? "PASS" : "FAIL") << " start check\n";
    return pass ? 0 : 1;
}
