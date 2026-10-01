// hp_crossover_<Effect>: proves the band split sums flat. With the processing stage neutral
// (HP_CHECK_BASSMAXIMISER: Boost 0 dB, Harmonics 0, Tightness 0; HP_CHECK_MORESTEREO: Width
// 100 %, Bass Mono 0, Stereo Enhance 0, Ambience 0), a sine at each of 48 log-spaced
// frequencies from 20 Hz to 5 kHz must come out within +/-0.5 dB of its input level, at the
// default crossover and at a second one. Summing a Butterworth low-pass and high-pass at the
// same frequency (the code before this fix) puts an exact notch at the crossover.
//
// BassMaximiser also checks Harmonics: a 50 Hz tone below the 80 Hz crossover must gain a
// 25 Hz (one octave down) component at Harmonics 100 % at least 20 dB above what Harmonics 0
// gives, and within 30 dB of the input level. Before this fix the generator's phase started at
// 0 and its gate never opened, so the two renders were identical.
// Exit 0 on pass. Registered with CTest in CMakeLists.txt (add_hyperprism_console_check).

#include HP_CHECK_PROCESSOR_HEADER
#include <cmath>
#include <functional>
#include <iostream>
#include <vector>

namespace
{
int failures = 0;
constexpr double kSampleRate = 48000.0;
constexpr int kBlock = 512;

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

using Configure = std::function<void (juce::AudioProcessor&)>;

// Renders `seconds` of a sine at `freqHz` (amplitude 0.25, both channels) and returns channel 0.
std::vector<float> renderSine (double freqHz, double seconds, Configure configure)
{
    HP_CHECK_PROCESSOR_CLASS proc;
    configure (proc);
    proc.setPlayConfigDetails (2, 2, kSampleRate, kBlock);
    proc.prepareToPlay (kSampleRate, kBlock);

    const int length = (int) (seconds * kSampleRate);
    std::vector<float> out;
    out.reserve ((size_t) length);
    juce::AudioBuffer<float> buffer (2, kBlock);
    juce::MidiBuffer midi;

    for (int start = 0; start < length; start += kBlock)
    {
        for (int i = 0; i < kBlock; ++i)
        {
            const float v = 0.25f * (float) std::sin (juce::MathConstants<double>::twoPi * freqHz * (start + i) / kSampleRate);
            buffer.setSample (0, i, v);
            buffer.setSample (1, i, v);
        }
        proc.processBlock (buffer, midi);
        for (int i = 0; i < kBlock; ++i)
            out.push_back (buffer.getSample (0, i));
    }
    return out;
}

// Amplitude of the `freqHz` component over the last `seconds` of x (single-bin DFT over a whole
// number of cycles).
double amplitudeAt (const std::vector<float>& x, double freqHz, double seconds)
{
    const int cycles = juce::jmax (1, (int) std::floor (seconds * freqHz));
    const int n = (int) std::round (cycles * kSampleRate / freqHz);
    const int start = (int) x.size() - n;
    double re = 0.0, im = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double w = juce::MathConstants<double>::twoPi * freqHz * (start + i) / kSampleRate;
        re += x[(size_t) (start + i)] * std::cos (w);
        im += x[(size_t) (start + i)] * std::sin (w);
    }
    return 2.0 * std::sqrt (re * re + im * im) / n;
}

void checkFlatSum (const char* label, Configure configure)
{
    double worstDb = 0.0, worstFreq = 0.0;
    for (int k = 0; k < 48; ++k)
    {
        const double f = 20.0 * std::pow (250.0, k / 47.0); // 20 Hz .. 5 kHz
        const double seconds = juce::jmax (0.5, 30.0 / f);
        const auto out = renderSine (f, seconds + 0.5, configure);
        const double db = 20.0 * std::log10 (juce::jmax (1.0e-9, amplitudeAt (out, f, seconds) / 0.25));
        if (std::abs (db) > std::abs (worstDb))
        {
            worstDb = db;
            worstFreq = f;
        }
    }

    const bool pass = std::abs (worstDb) <= 0.5;
    std::cout << (pass ? "ok   " : "FAIL ") << label << ": largest deviation from flat "
              << worstDb << " dB at " << worstFreq << " Hz (limit +/-0.5 dB)\n";
    if (! pass)
        ++failures;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

#if HP_CHECK_BASSMAXIMISER
    auto neutral = [] (juce::AudioProcessor& p, float crossover)
    {
        setParam (p, "frequency", crossover);
        setParam (p, "boost", 0.0f);
        setParam (p, "harmonics", 0.0f);
        setParam (p, "tightness", 0.0f);
        setParam (p, "outputGain", 0.0f);
        setParam (p, "phaseInvert", 0.0f);
    };
    checkFlatSum ("neutral sum, crossover 80 Hz (default)", [&] (auto& p) { neutral (p, 80.0f); });
    checkFlatSum ("neutral sum, crossover 250 Hz", [&] (auto& p) { neutral (p, 250.0f); });

    // Harmonics: one octave down from a 50 Hz tone.
    auto withHarmonics = [] (float amount)
    {
        return [amount] (juce::AudioProcessor& p)
        {
            setParam (p, "frequency", 80.0f);
            setParam (p, "boost", 6.0f);
            setParam (p, "harmonics", amount);
            setParam (p, "tightness", 0.0f);
        };
    };
    const double off = amplitudeAt (renderSine (50.0, 2.0, withHarmonics (0.0f)), 25.0, 1.0);
    const double on = amplitudeAt (renderSine (50.0, 2.0, withHarmonics (100.0f)), 25.0, 1.0);
    const double onDb = 20.0 * std::log10 (juce::jmax (1.0e-9, on / 0.25));
    const double riseDb = 20.0 * std::log10 (juce::jmax (1.0e-9, on) / juce::jmax (1.0e-9, off));
    const bool harmonicsPass = riseDb >= 20.0 && onDb >= -30.0;
    std::cout << (harmonicsPass ? "ok   " : "FAIL ") << "Harmonics: 25 Hz sub-octave of a 50 Hz tone at "
              << onDb << " dB re input with Harmonics 100 %, " << riseDb << " dB above Harmonics 0\n";
    if (! harmonicsPass)
        ++failures;
#elif HP_CHECK_MORESTEREO
    auto neutral = [] (juce::AudioProcessor& p, float crossover)
    {
        setParam (p, "crossoverFreq", crossover);
        setParam (p, "width", 100.0f);
        setParam (p, "bassMono", 0.0f);
        setParam (p, "stereoEnhance", 0.0f);
        setParam (p, "ambience", 0.0f);
        setParam (p, "outputLevel", 0.0f);
    };
    checkFlatSum ("neutral sum, crossover 120 Hz (default)", [&] (auto& p) { neutral (p, 120.0f); });
    checkFlatSum ("neutral sum, crossover 300 Hz", [&] (auto& p) { neutral (p, 300.0f); });
#else
    #error "define HP_CHECK_BASSMAXIMISER or HP_CHECK_MORESTEREO"
#endif

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " crossover check\n";
    return failures == 0 ? 0 : 1;
}
