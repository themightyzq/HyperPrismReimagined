// hp_blocksize_<Effect>: proves a plugin's output does not depend on the host's block size.
// The same stereo input (independent noise per channel plus a 220 Hz sine) is rendered with
// prepareToPlay/processBlock at 64 and at 1000 samples (non-power-of-two); the two renders must
// be identical (max difference <= 1e-6).
// HP_CHECK_TREMOLO: Stereo Phase 90 degrees, Depth 100 %, Rate 5 Hz, sine. The right channel's
// LFO phase used to be re-derived from the left LFO's phase at the END of each block, so the
// left/right relationship depended on block size. Also checks that with Stereo Phase 0 and
// Depth and Mix stepped mid-render, identical input on both channels gives identical outputs
// (each smoother advances once per sample and both channels share its value).
// Exit 0 on pass. Registered with CTest in CMakeLists.txt (add_hyperprism_light_check).

#include HP_CHECK_PROCESSOR_HEADER
#include <cmath>
#include <functional>
#include <iostream>
#include <vector>

namespace
{
int failures = 0;
constexpr double kSampleRate = 48000.0;
constexpr int kLength = 48000;

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

using Setter = std::function<void (juce::AudioProcessor&)>;

// Renders both channels; `atHalf` (if set) is applied before the block containing kLength / 2.
std::vector<float> render (const std::vector<float>& l, const std::vector<float>& r, int block,
                           Setter setup, Setter atHalf = nullptr)
{
    HP_CHECK_PROCESSOR_CLASS proc;
    setup (proc);
    proc.setPlayConfigDetails (2, 2, kSampleRate, block);
    proc.prepareToPlay (kSampleRate, block);

    std::vector<float> out ((size_t) (2 * kLength), 0.0f);
    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;
    bool stepped = false;
    for (int start = 0; start < kLength; start += block)
    {
        const int n = juce::jmin (block, kLength - start);
        if (atHalf && ! stepped && start + n > kLength / 2)
        {
            atHalf (proc);
            stepped = true;
        }
        buffer.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i)
        {
            buffer.setSample (0, i, l[(size_t) (start + i)]);
            buffer.setSample (1, i, r[(size_t) (start + i)]);
        }
        proc.processBlock (buffer, midi);
        for (int i = 0; i < n; ++i)
        {
            out[(size_t) (start + i)] = buffer.getSample (0, i);
            out[(size_t) (kLength + start + i)] = buffer.getSample (1, i);
        }
    }
    return out;
}

std::vector<float> signal (int seed)
{
    std::vector<float> x ((size_t) kLength);
    juce::Random rng (seed);
    for (int i = 0; i < kLength; ++i)
        x[(size_t) i] = 0.3f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * i / kSampleRate)
                      + 0.2f * (rng.nextFloat() * 2.0f - 1.0f);
    return x;
}

float maxDiff (const std::vector<float>& a, const std::vector<float>& b, size_t from = 0, size_t to = 0)
{
    if (to == 0)
        to = a.size();
    float d = 0.0f;
    for (size_t i = from; i < to; ++i)
        d = juce::jmax (d, std::abs (a[i] - b[i]));
    return d;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

#if HP_CHECK_TREMOLO
    const Setter setup = [] (juce::AudioProcessor& p)
    {
        setParam (p, "stereoPhase", 90.0f);
        setParam (p, "depth", 100.0f);
        setParam (p, "rate", 5.0f);
        setParam (p, "mix", 100.0f);
    };
#else
    #error "define HP_CHECK_TREMOLO"
#endif

    const auto l = signal (11);
    const auto r = signal (12);
    const auto a = render (l, r, 64, setup);
    const auto b = render (l, r, 1000, setup);
    const float d = maxDiff (a, b);
    const bool pass = d <= 1.0e-6f;
    std::cout << (pass ? "ok   " : "FAIL ") << "block sizes 64 vs 1000: max difference " << d << "\n";
    if (! pass)
        ++failures;

#if HP_CHECK_TREMOLO
    {
        const Setter phase0 = [] (juce::AudioProcessor& p)
        {
            setParam (p, "stereoPhase", 0.0f);
            setParam (p, "depth", 30.0f);
            setParam (p, "rate", 5.0f);
            setParam (p, "mix", 40.0f);
        };
        const Setter step = [] (juce::AudioProcessor& p)
        {
            setParam (p, "depth", 100.0f);
            setParam (p, "mix", 100.0f);
        };
        const auto out = render (l, l, 512, phase0, step);
        const float lr = maxDiff (std::vector<float> (out.begin(), out.begin() + kLength),
                                  std::vector<float> (out.begin() + kLength, out.end()));
        const bool nullPass = lr <= 1.0e-6f;
        std::cout << (nullPass ? "ok   " : "FAIL ") << "stereo phase 0, Depth and Mix stepped mid-render: max |L-R| for identical input "
                  << lr << "\n";
        if (! nullPass)
            ++failures;
    }
#endif

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " block size check\n";
    return failures == 0 ? 0 : 1;
}
