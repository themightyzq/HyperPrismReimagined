// hp_isolation_<Effect>: proves the plugin keeps no DSP state outside its own instance and
// channel. Three renders at a setting chosen per plugin so the state in question is audible
// (HP_CHECK_PHASER: feedback 80 %, mix 100 %; HP_CHECK_TUBETAPE: Transformer, warmth 100 %):
//   1. Instance isolation: instance A renders signal X alone; then a fresh A' renders X while
//      a second instance B renders a different signal Y block-interleaved with it. A' must
//      equal A sample for sample. Function-level `static` filter state shared by every
//      instance (Phaser's feedback memory and TubeTape's hysteresis until this fix) fails this.
//   2. Stereo null: identical input on both channels gives identical outputs.
//   3. Mono vs stereo: channel 0 of a stereo render equals a mono-layout render of the same
//      input, so per-sample state (LFOs, smoothers) advances once per sample, not once per
//      channel (Phaser's LFO ran at twice the set rate in stereo until this fix).
// Exit 0 on pass. Registered with CTest in CMakeLists.txt (add_hyperprism_console_check).

#include HP_CHECK_PROCESSOR_HEADER
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
int failures = 0;
constexpr double kSampleRate = 48000.0;
constexpr int kBlock = 480;
constexpr int kNumBlocks = 60;

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

void configure (juce::AudioProcessor& proc)
{
#if HP_CHECK_PHASER
    setParam (proc, "feedback", 80.0f);
    setParam (proc, "mix", 100.0f);
    setParam (proc, "depth", 100.0f);
    setParam (proc, "rate", 2.0f);
#elif HP_CHECK_TUBETAPE
    setParam (proc, "type", 2.0f);      // Transformer
    setParam (proc, "warmth", 100.0f);
    setParam (proc, "drive", 80.0f);
#else
    #error "define HP_CHECK_PHASER or HP_CHECK_TUBETAPE"
#endif
}

std::vector<float> signal (int seed, int length)
{
    std::vector<float> x ((size_t) length);
    juce::Random rng (seed);
    double phase = 0.0;
    for (auto& v : x)
    {
        v = 0.5f * (float) std::sin (phase) + 0.2f * (rng.nextFloat() * 2.0f - 1.0f);
        phase += juce::MathConstants<double>::twoPi * (seed == 1 ? 220.0 : 1375.0) / kSampleRate;
    }
    return x;
}

struct Instance
{
    explicit Instance (int numChannels) : buffer (numChannels, kBlock)
    {
        configure (proc);
        proc.setPlayConfigDetails (numChannels, numChannels, kSampleRate, kBlock);
        proc.prepareToPlay (kSampleRate, kBlock);
    }

    // Processes block b of x (same signal on every channel); appends every channel's output.
    void processBlock (const std::vector<float>& x, int b, std::vector<std::vector<float>>& out)
    {
        out.resize ((size_t) buffer.getNumChannels());
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < kBlock; ++i)
                buffer.setSample (ch, i, x[(size_t) (b * kBlock + i)]);
        proc.processBlock (buffer, midi);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < kBlock; ++i)
                out[(size_t) ch].push_back (buffer.getSample (ch, i));
    }

    HP_CHECK_PROCESSOR_CLASS proc;
    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;
};

float maxDiff (const std::vector<float>& a, const std::vector<float>& b)
{
    float d = 0.0f;
    for (size_t i = 0; i < juce::jmin (a.size(), b.size()); ++i)
        d = juce::jmax (d, std::abs (a[i] - b[i]));
    return d;
}

void report (const char* what, float diff)
{
    const bool pass = diff <= 1.0e-6f;
    std::cout << (pass ? "ok   " : "FAIL ") << what << ": max difference " << diff << "\n";
    if (! pass)
        ++failures;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    const int length = kBlock * kNumBlocks;
    const auto x = signal (1, length);
    const auto y = signal (2, length);

    // 1. Instance isolation.
    std::vector<std::vector<float>> alone, together, other;
    {
        Instance a (2);
        for (int b = 0; b < kNumBlocks; ++b)
            a.processBlock (x, b, alone);
    }
    {
        Instance a (2), bInst (2);
        for (int b = 0; b < kNumBlocks; ++b)
        {
            bInst.processBlock (y, b, other);
            a.processBlock (x, b, together);
        }
    }
    report ("instance isolation (A alone vs A interleaved with B)",
            juce::jmax (maxDiff (alone[0], together[0]), maxDiff (alone[1], together[1])));

    // 2. Stereo null.
    report ("stereo null (identical input on L and R)", maxDiff (alone[0], alone[1]));

    // 3. Mono layout vs stereo channel 0.
    std::vector<std::vector<float>> mono;
    {
        Instance m (1);
        for (int b = 0; b < kNumBlocks; ++b)
            m.processBlock (x, b, mono);
    }
    report ("mono render vs stereo left", maxDiff (mono[0], alone[0]));

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " isolation check\n";
    return failures == 0 ? 0 : 1;
}
