// hp_echo_timing_MultiDelay: regression check for the MultiDelay "scrambled taps" defect.
// processMultiDelay() used to call popSample(..., true) on every OTHER tap while building the
// global feedback sum, and popSample's updateReadPointer=true advances that line's read head,
// so each tap's read pointer moved several times per sample and its echoes landed at the
// wrong times (even with Global Feedback at 0). Every tap is now read exactly once per sample.
//
// Checks (48 kHz, where the default tap times 125/250/500/750 ms are whole sample counts):
//  1. One tap at a time (the others at level 0), no feedback: the single echo lands at exactly
//     the configured delay (+-1 sample) and nothing arrives before it.
//  2. All four taps at once, no feedback: the output is exactly the four delayed impulses
//     (this is the case the old code scrambled).
//  3. Default settings (local and global feedback on): each tap's first echo is still at its
//     configured delay.
//  4. Default settings: renders with 64- and 511-sample blocks match to within 1e-5.
// Exit 0 on pass.

#include HP_CHECK_PROCESSOR_HEADER
#include <iostream>
#include <vector>
#include <cmath>

namespace
{
int failures = 0;

void fail(const juce::String& msg)
{
    std::cerr << "FAIL " << msg << "\n";
    ++failures;
}

void setParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float raw)
{
    if (auto* p = apvts.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(raw));
    else
        fail("unknown parameter " + id);
}

constexpr double kSampleRate = 48000.0;
constexpr int kPreparedBlock = 512;
constexpr int kNumTaps = 4;
const float kTapTimesMs[kNumTaps] = { 125.0f, 250.0f, 500.0f, 750.0f };

juce::String tapId(int tap, const char* suffix)
{
    return "delay" + juce::String(tap + 1) + suffix;
}

// Renders `input` (per-channel, same signal on both) through `proc` in blocks of `blockSize`
// and returns channel 0 and channel 1 of the output.
std::vector<std::vector<float>> render(HP_CHECK_PROCESSOR_CLASS& proc,
                                       const std::vector<float>& input, int blockSize)
{
    proc.setPlayConfigDetails(2, 2, kSampleRate, kPreparedBlock);
    proc.prepareToPlay(kSampleRate, kPreparedBlock);

    std::vector<std::vector<float>> out(2, std::vector<float>(input.size(), 0.0f));
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;

    for (size_t start = 0; start < input.size(); start += (size_t) blockSize)
    {
        const int n = (int) std::min((size_t) blockSize, input.size() - start);
        buffer.setSize(2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                buffer.setSample(ch, i, input[start + (size_t) i]);

        proc.processBlock(buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                out[(size_t) ch][start + (size_t) i] = buffer.getSample(ch, i);
    }
    return out;
}

int expectedDelaySamples(int tap)
{
    return (int) std::lround(kTapTimesMs[tap] / 1000.0 * kSampleRate);
}

void setupIsolated(juce::AudioProcessorValueTreeState& apvts, int activeTap, bool allActive)
{
    setParam(apvts, "masterMix", 100.0f);
    setParam(apvts, "globalFeedback", 0.0f);
    for (int t = 0; t < kNumTaps; ++t)
    {
        setParam(apvts, tapId(t, "Time"), kTapTimesMs[t]);
        setParam(apvts, tapId(t, "Level"), (allActive || t == activeTap) ? 100.0f : 0.0f);
        setParam(apvts, tapId(t, "Pan"), 0.0f);
        setParam(apvts, tapId(t, "Feedback"), 0.0f);
    }
}

std::vector<float> impulse(size_t length)
{
    std::vector<float> x(length, 0.0f);
    x[0] = 1.0f;
    return x;
}

void checkSingleTaps()
{
    const auto input = impulse(40000);

    for (int tap = 0; tap < kNumTaps; ++tap)
    {
        HP_CHECK_PROCESSOR_CLASS proc;
        setupIsolated(proc.getValueTreeState(), tap, false);
        const auto out = render(proc, input, kPreparedBlock);
        const int expected = expectedDelaySamples(tap);

        for (int ch = 0; ch < 2; ++ch)
        {
            const auto& y = out[(size_t) ch];
            size_t peakIndex = 0;
            float peak = 0.0f;
            for (size_t i = 0; i < y.size(); ++i)
                if (std::abs(y[i]) > peak) { peak = std::abs(y[i]); peakIndex = i; }

            float early = 0.0f;
            for (int i = 0; i < expected - 1; ++i)
                early = std::max(early, std::abs(y[(size_t) i]));

            if (std::abs((int) peakIndex - expected) > 1 || peak < 0.9f || early > 1.0e-4f)
                fail("tap " + juce::String(tap + 1) + " ch " + juce::String(ch) + ": peak "
                     + juce::String(peak) + " at " + juce::String((int) peakIndex) + ", expected ~1 at "
                     + juce::String(expected) + ", max before it " + juce::String(early));
            else
                std::cout << "ok   tap " << tap + 1 << " ch " << ch << " echo at " << peakIndex
                          << " samples (expected " << expected << ")\n";
        }
    }
}

void checkAllTapsNoFeedback()
{
    HP_CHECK_PROCESSOR_CLASS proc;
    setupIsolated(proc.getValueTreeState(), -1, true);
    const auto out = render(proc, impulse(40000), kPreparedBlock);

    std::vector<float> expected(40000, 0.0f);
    for (int tap = 0; tap < kNumTaps; ++tap)
        expected[(size_t) expectedDelaySamples(tap)] += 1.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        float maxErr = 0.0f;
        size_t worst = 0;
        for (size_t i = 0; i < expected.size(); ++i)
        {
            const float err = std::abs(out[(size_t) ch][i] - expected[i]);
            if (err > maxErr) { maxErr = err; worst = i; }
        }

        // Tolerance covers the linear interpolation of a tap time that the parameter's 0.1 ms
        // snapping leaves a hair off a whole sample; a scrambled tap is off by ~1.
        if (maxErr > 1.0e-2f)
            fail("four taps, no feedback, ch " + juce::String(ch) + ": output differs from four clean echoes by "
                 + juce::String(maxErr) + " at sample " + juce::String((int) worst));
        else
            std::cout << "ok   four taps, no feedback, ch " << ch
                      << ": exactly four echoes at 6000/12000/24000/36000 (max error " << maxErr << ")\n";
    }
}

void checkFirstEchoesWithFeedback()
{
    HP_CHECK_PROCESSOR_CLASS proc;   // default settings: local + global feedback on
    const auto out = render(proc, impulse(40000), kPreparedBlock);
    const auto& y = out[0];

    // Nothing but the dry impulse (sample 0) before the shortest tap; each tap's first echo
    // shows up at its delay.
    float early = 0.0f;
    for (int i = 1; i < expectedDelaySamples(0) - 1; ++i)
        early = std::max(early, std::abs(y[(size_t) i]));
    if (early > 1.0e-4f)
        fail("default settings: output before the first tap: " + juce::String(early));

    for (int tap = 0; tap < kNumTaps; ++tap)
    {
        const int expected = expectedDelaySamples(tap);
        float nearPeak = 0.0f;
        for (int i = expected - 1; i <= expected + 1; ++i)
            nearPeak = std::max(nearPeak, std::abs(y[(size_t) i]));

        // Mix 50 %, the lowest default level (30 %) and its pan gain (0.75) leave ~0.11.
        if (nearPeak < 0.05f)
            fail("default settings: tap " + juce::String(tap + 1) + " first echo missing at "
                 + juce::String(expected) + " (|y| = " + juce::String(nearPeak) + ")");
        else
            std::cout << "ok   default settings: tap " << tap + 1 << " first echo at " << expected
                      << " (|y| = " << nearPeak << ")\n";
    }
}

void checkBlockSizeInvariance()
{
    // Impulse plus a short noise burst, long enough for several feedback round trips.
    std::vector<float> input(96000, 0.0f);
    juce::Random rng(1234);
    input[0] = 1.0f;
    for (size_t i = 1000; i < 3000; ++i)
        input[i] = rng.nextFloat() * 0.5f - 0.25f;

    HP_CHECK_PROCESSOR_CLASS a, b;
    const auto outA = render(a, input, 64);
    const auto outB = render(b, input, 511);

    float maxDiff = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (size_t i = 0; i < input.size(); ++i)
            maxDiff = std::max(maxDiff, std::abs(outA[(size_t) ch][i] - outB[(size_t) ch][i]));

    if (maxDiff > 1.0e-5f)
        fail("block sizes 64 vs 511 differ by " + juce::String(maxDiff));
    else
        std::cout << "ok   block sizes 64 vs 511 match (max diff " << maxDiff << ")\n";
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    checkSingleTaps();
    checkAllTapsNoFeedback();
    checkFirstEchoesWithFeedback();
    checkBlockSizeInvariance();

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " echo timing check\n";
    return failures == 0 ? 0 : 1;
}
