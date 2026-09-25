// hp_release_time_Limiter: regression check for the "Release parameter has no audible effect"
// defect (LimiterProcessor::processBlock() used to read the Release parameter and then ignore
// it, applying hard-coded 0.999f/0.01f coefficients instead -- see CHANGELOG and the FIX
// comment in LimiterProcessor.cpp).
//
// Method: run a -20 dB step (a loud "burst" sustained to steady-state gain reduction, then an
// instant 20 dB drop) through the real LimiterProcessor at two very different Release settings
// (50 ms and 300 ms), and measure how many samples it takes the recovering gain to reach 90% of
// the way back to unity. Because both the envelope follower's release and the gain smoother's
// release share the exact same Release-derived coefficient
// (coeff = exp(-1000 / (release_ms * sampleRate))), and the whole post-step recurrence only
// depends on sample count through that coefficient (never engaging the fixed attack branch
// during a release, since gain is monotonically recovering after a level drop), the measured
// 90%-recovery sample count is an exactly linear function of release_ms for fixed sample rate,
// ceiling and burst/post levels. So measuredSamples(300ms) / measuredSamples(50ms) must equal
// 300/50 = 6.0, regardless of the compound (envelope + gain-smoothing) shape of the response --
// this is what actually proves Release drives real, proportional, release-time control, rather
// than requiring a closed-form derivation of the two-stage step response.
//
// Before the fix this ratio was ~1.0 (both settings produced the same hard-coded ~20.8ms-
// equivalent release, see LimiterProcessor.cpp's Release parameter default comment). Exit 0 on
// pass.

#include HP_CHECK_PROCESSOR_HEADER
#include <iostream>
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
    {
        fail("unknown parameter " + id);
    }
}

// Returns the sample index (0-based, counted from the instant of the -20dB step) at which the
// recovering gain first reaches 90% of the way from the burst's steady-state gain reduction
// back to unity gain. Returns -1 if it never gets there within the timeout.
long measureRecoverySamples(float releaseMs, double sampleRate)
{
    HP_CHECK_PROCESSOR_CLASS proc;
    proc.setPlayConfigDetails(2, 2, sampleRate, 1);
    proc.prepareToPlay(sampleRate, 1);

    auto& apvts = proc.getValueTreeState();
    setParam(apvts, "ceiling", -6.0f);
    setParam(apvts, "release", releaseMs);
    setParam(apvts, "lookahead", 0.0f);
    setParam(apvts, "softclip", 0.0f);
    setParam(apvts, "inputgain", 0.0f);
    setParam(apvts, "bypass", 0.0f);

    const float burstLevel = 1.0f;                                             // 0 dBFS
    const float postLevel  = burstLevel * juce::Decibels::decibelsToGain(-20.0f); // -20 dB step

    juce::AudioBuffer<float> buf(2, 1);
    juce::MidiBuffer midi;

    // Sustain the burst well past the (fixed, fast, ~0.1ms-equivalent) attack's settling time
    // so the limiter reaches true steady-state gain reduction before the step.
    const int burstSamples = static_cast<int>(0.5 * sampleRate);
    for (int i = 0; i < burstSamples; ++i)
    {
        buf.setSample(0, 0, burstLevel);
        buf.setSample(1, 0, burstLevel);
        proc.processBlock(buf, midi);
    }

    const float steadyGain = buf.getSample(0, 0) / burstLevel; // ~= ceiling linear
    const float threshold  = steadyGain + 0.9f * (1.0f - steadyGain);

    const long maxSamples = static_cast<long>(5.0 * releaseMs / 1000.0 * sampleRate) + 10000;
    for (long n = 0; n < maxSamples; ++n)
    {
        buf.setSample(0, 0, postLevel);
        buf.setSample(1, 0, postLevel);
        proc.processBlock(buf, midi);

        const float gainNow = buf.getSample(0, 0) / postLevel;
        if (gainNow >= threshold)
            return n;
    }
    return -1;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000.0;
    constexpr float releaseA = 50.0f;
    constexpr float releaseB = 300.0f;

    const long samplesA = measureRecoverySamples(releaseA, fs);
    const long samplesB = measureRecoverySamples(releaseB, fs);

    if (samplesA < 0 || samplesB < 0)
    {
        fail("gain reduction never recovered to 90% within the timeout window");
    }
    else
    {
        const double msA = 1000.0 * (double) samplesA / fs;
        const double msB = 1000.0 * (double) samplesB / fs;
        const double ratio = msB / msA;
        const double expectedRatio = (double) releaseB / (double) releaseA; // 6.0

        std::cout << "Release=" << releaseA << "ms  -> measured 90% recovery = " << msA
                   << " ms (" << samplesA << " samples)\n";
        std::cout << "Release=" << releaseB << "ms -> measured 90% recovery = " << msB
                   << " ms (" << samplesB << " samples)\n";
        std::cout << "measured ratio = " << ratio << ", expected " << expectedRatio
                   << " (" << releaseB << "/" << releaseA << ")\n";

        const double tolerance = 0.10;
        if (ratio < expectedRatio * (1.0 - tolerance) || ratio > expectedRatio * (1.0 + tolerance))
            fail("measured release-time ratio is not within 10% of the expected " + juce::String(expectedRatio) + "x");
    }

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " limiter release-time check\n";
    return failures == 0 ? 0 : 1;
}
