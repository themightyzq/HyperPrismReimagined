// hp_lookahead_<Effect>: proves a lookahead plugin reports its delay exactly
// (HP_CHECK_NOISEGATE, HP_CHECK_LIMITER).
//   1. getLatencySamples() is positive and the same for block sizes 64/100/512/1024 at 44.1, 48
//      and 96 kHz.
//   2. Bypassed, the output is the input delayed by exactly the reported latency (bypass runs
//      through the same delay, so toggling it does not jump the timing).
//   3. At a neutral setting (gate: threshold -60 dB, range 0 dB; limiter: ceiling 0 dB with a
//      0.5-peak signal) the output is likewise the input delayed by exactly the latency.
//   4. Lookahead acts early: with Lookahead 5 ms, the gain change for a level step starts
//      before the step reaches the output (gate: already open, at least 90 % gain, for the
//      first 1 ms of a burst after silence; limiter: gain already reduced by at least 10 % in
//      the 2.5 ms before a step from 0.1 to 1.0 against a -6 dB ceiling).
//   5. A 64-sample and a 1000-sample render of the step signal are identical.
// Before this fix NoiseGate delayed its audio by the lookahead time but reported 0 latency and
// skipped the delay when bypassed, and Limiter's Lookahead did nothing at all.
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

std::vector<float> render (const std::vector<float>& input, int block, Configure configure,
                           int* latencyOut = nullptr, double sampleRate = kSampleRate)
{
    HP_CHECK_PROCESSOR_CLASS proc;
    if (configure)
        configure (proc);
    proc.setPlayConfigDetails (2, 2, sampleRate, block);
    proc.prepareToPlay (sampleRate, block);
    if (latencyOut != nullptr)
        *latencyOut = proc.getLatencySamples();

    std::vector<float> out (input.size(), 0.0f);
    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;
    for (size_t start = 0; start < input.size(); start += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, input.size() - start);
        buffer.setSize (2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                buffer.setSample (ch, i, input[start + (size_t) i]);
        proc.processBlock (buffer, midi);
        for (int i = 0; i < n; ++i)
            out[start + (size_t) i] = buffer.getSample (0, i);
    }
    return out;
}

void expectDelayedCopy (const char* what, const std::vector<float>& in, Configure configure)
{
    int latency = -1;
    const auto out = render (in, 512, configure, &latency);
    float err = 0.0f;
    for (size_t n = 0; n < out.size(); ++n)
    {
        const float expected = (int) n >= latency ? in[n - (size_t) juce::jmax (0, latency)] : 0.0f;
        err = juce::jmax (err, std::abs (out[n] - expected));
    }
    const bool pass = latency > 0 && err <= 1.0e-6f;
    std::cout << (pass ? "ok   " : "FAIL ") << what << ": output vs input delayed by the reported "
              << latency << " samples, max error " << err << "\n";
    if (! pass)
        ++failures;
}

std::vector<float> sine (double hz, float amp, int length)
{
    std::vector<float> x ((size_t) length);
    for (int i = 0; i < length; ++i)
        x[(size_t) i] = amp * (float) std::sin (juce::MathConstants<double>::twoPi * hz * i / kSampleRate);
    return x;
}

#if HP_CHECK_NOISEGATE
const Configure neutral = [] (juce::AudioProcessor& p) { setParam (p, "threshold", -60.0f); setParam (p, "range", 0.0f); };
const Configure active  = [] (juce::AudioProcessor& p) { setParam (p, "lookahead", 5.0f); setParam (p, "attack", 0.5f); };

// Silence, then a 1 kHz burst at 0.5 from sample `stepAt`.
std::vector<float> stepSignal (int length, int stepAt)
{
    auto x = sine (1000.0, 0.5f, length);
    for (int i = 0; i < stepAt; ++i)
        x[(size_t) i] = 0.0f;
    return x;
}

void checkLookaheadActsEarly()
{
    const int stepAt = 24000;
    const auto in = stepSignal (36000, stepAt);
    int latency = -1;
    const auto out = render (in, 512, active, &latency);

    // Gain over the first 1 ms of the burst as it reaches the output.
    double num = 0.0, den = 0.0;
    for (int n = stepAt + latency; n < stepAt + latency + 48; ++n)
    {
        num += std::abs (out[(size_t) n]);
        den += std::abs (in[(size_t) (n - latency)]);
    }
    const double gain = den > 0.0 ? num / den : 0.0;
    const bool pass = gain >= 0.9;
    std::cout << (pass ? "ok   " : "FAIL ") << "lookahead 5 ms: gate gain over the first 1 ms of a burst is "
              << gain << " (needs >= 0.9: the gate opened before the burst arrived)\n";
    if (! pass)
        ++failures;
}
#elif HP_CHECK_LIMITER
const Configure neutral = [] (juce::AudioProcessor& p) { setParam (p, "ceiling", 0.0f); setParam (p, "softclip", 0.0f); };
const Configure active  = [] (juce::AudioProcessor& p) { setParam (p, "lookahead", 5.0f); setParam (p, "ceiling", -6.0f); };

// 1 kHz at 0.1, stepping to 1.0 at sample `stepAt`.
std::vector<float> stepSignal (int length, int stepAt)
{
    auto x = sine (1000.0, 1.0f, length);
    for (int i = 0; i < stepAt; ++i)
        x[(size_t) i] *= 0.1f;
    return x;
}

void checkLookaheadActsEarly()
{
    const int stepAt = 24000;
    const auto in = stepSignal (36000, stepAt);
    int latency = -1;
    const auto out = render (in, 512, active, &latency);

    // Gain in the 2.5 ms before the step reaches the output.
    double num = 0.0, den = 0.0;
    for (int n = stepAt + latency - 120; n < stepAt + latency; ++n)
    {
        num += std::abs (out[(size_t) n]);
        den += std::abs (in[(size_t) (n - latency)]);
    }
    const double gain = den > 0.0 ? num / den : 1.0;
    const bool pass = gain <= 0.9;
    std::cout << (pass ? "ok   " : "FAIL ") << "lookahead 5 ms: gain in the 2.5 ms before a +20 dB step is "
              << gain << " (needs <= 0.9: gain reduction started before the step arrived)\n";
    if (! pass)
        ++failures;
}
#else
    #error "define HP_CHECK_NOISEGATE or HP_CHECK_LIMITER"
#endif

void checkLatencyInvariance()
{
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        int first = -1;
        bool same = true;
        for (int bs : { 64, 100, 512, 1024 })
        {
            int latency = -1;
            render (std::vector<float> (16, 0.0f), bs, nullptr, &latency, sr);
            if (first < 0)
                first = latency;
            else if (latency != first)
                same = false;
        }
        const bool pass = same && first > 0;
        std::cout << (pass ? "ok   " : "FAIL ") << "latency at " << sr << " Hz: " << first
                  << " samples" << (same ? ", identical across block sizes 64/100/512/1024" : ", CHANGES with block size") << "\n";
        if (! pass)
            ++failures;
    }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    checkLatencyInvariance();

    juce::Random rng (3);
    std::vector<float> noise (20000);
    for (auto& v : noise)
        v = 0.5f * (rng.nextFloat() * 2.0f - 1.0f);

    expectDelayedCopy ("bypass", noise, [] (juce::AudioProcessor& p) { setParam (p, "bypass", 1.0f); });
    expectDelayedCopy ("neutral setting", noise, neutral);

    checkLookaheadActsEarly();

    {
        const auto in = stepSignal (36000, 24000);
        const auto a = render (in, 64, active);
        const auto b = render (in, 1000, active);
        float d = 0.0f;
        for (size_t i = 0; i < a.size(); ++i)
            d = juce::jmax (d, std::abs (a[i] - b[i]));
        const bool pass = d <= 1.0e-6f;
        std::cout << (pass ? "ok   " : "FAIL ") << "block sizes 64 vs 1000: max difference " << d << "\n";
        if (! pass)
            ++failures;
    }

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " lookahead check\n";
    return failures == 0 ? 0 : 1;
}
