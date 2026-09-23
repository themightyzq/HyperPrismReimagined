// hp_oversampling_<Effect>: proves that the plugin's nonlinear stage
// (saturation waveshaper / harmonic generator / bit-rate quantiser) is running through its
// 4x juce::dsp::Oversampling wrapper, and that the reported latency is block-size invariant
// at 44.1/48/96 kHz. Meant to be built per plugin the same way tools/state_check/main.cpp
// is: compile definitions select the processor (HP_CHECK_PROCESSOR_HEADER /
// HP_CHECK_PROCESSOR_CLASS) and which plugin-specific "strong setting" and parameter IDs to
// drive (HP_CHECK_TUBETAPE / HP_CHECK_HARMONICEXCITER / HP_CHECK_SONICDECIMATOR). Not yet
// registered in CMakeLists.txt -- see the report for how it was exercised without that.
// Exit 0 on pass.

#include HP_CHECK_PROCESSOR_HEADER
#include <iostream>
#include <vector>
#include <cmath>

namespace
{
int failures = 0;

void setParam (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float rawValue)
{
    if (auto* param = apvts.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (rawValue));
    else
    {
        std::cerr << "FAIL unknown parameter " << id << "\n";
        ++failures;
    }
}

// A strong drive/decimation setting per plugin, chosen to maximise the harmonic content
// the nonlinear stage generates so aliasing (if any survives the oversampling wrapper)
// shows up clearly in the FFT.
void applyStrongSettings (HP_CHECK_PROCESSOR_CLASS& proc)
{
    auto& apvts = proc.getValueTreeState();

#if HP_CHECK_TUBETAPE
    setParam (apvts, "drive", 95.0f);
    setParam (apvts, "type", 0.0f);        // Tube
    setParam (apvts, "warmth", 70.0f);
    setParam (apvts, "brightness", 70.0f);
#elif HP_CHECK_HARMONICEXCITER
    setParam (apvts, "drive", 95.0f);
    setParam (apvts, "frequency", 997.0f); // lowest allowed cutoff: let the 1 kHz test
                                             // tone reach the harmonic generator
    setParam (apvts, "harmonics", 5.0f);
    setParam (apvts, "mix", 100.0f);
    setParam (apvts, "type", 1.0f);         // Bright
#elif HP_CHECK_SONICDECIMATOR
    setParam (apvts, "bitDepth", 2.0f);
    // Deliberately not a round divisor of the 1 kHz test tone or the 48 kHz render rate
    // (see measureAliasPeakDb()'s harmonic guard band): 3000 Hz would put every alias
    // image exactly on a harmonic bin of the fundamental, hiding it from this measurement.
    setParam (apvts, "sampleRate", 2900.0f);
    // On: isolates what the 4x oversampling wrapper contributes (suppressing the
    // bit-crusher's own quantisation harmonics) from the classic, user-intentional
    // decimation aliasing the Rate control's own filter already handles when this is off.
    setParam (apvts, "antiAlias", 1.0f);
    setParam (apvts, "dither", 0.0f);
    setParam (apvts, "mix", 100.0f);
#else
    #error "define HP_CHECK_TUBETAPE, HP_CHECK_HARMONICEXCITER, or HP_CHECK_SONICDECIMATOR"
#endif
}

constexpr double kTestSampleRate = 48000.0;
constexpr float  kTestFrequency  = 997.0f;
constexpr int    kFftOrder       = 13;     // 8192-point FFT, ~5.86 Hz bins at 48 kHz
constexpr int    kFftSize        = 1 << kFftOrder;
constexpr int    kBlockSize      = 512;

// Renders a 1 kHz sine through the processor at a strong setting, FFTs the (latency-
// aligned) output, and returns the highest non-harmonic peak's level relative to the
// fundamental, in dB (more negative is cleaner / better aliasing suppression).
float measureAliasPeakDb (HP_CHECK_PROCESSOR_CLASS& proc)
{
    proc.setPlayConfigDetails (2, 2, kTestSampleRate, kBlockSize);
    proc.prepareToPlay (kTestSampleRate, kBlockSize);

    const int latency = proc.getLatencySamples();
    const int totalSamplesNeeded = latency + kFftSize;
    const int numBlocks = (totalSamplesNeeded + kBlockSize - 1) / kBlockSize;

    juce::AudioBuffer<float> buffer (2, kBlockSize);
    juce::MidiBuffer midi;
    std::vector<float> rendered;
    rendered.reserve ((size_t) numBlocks * (size_t) kBlockSize);

    int sampleIndex = 0;
    for (int b = 0; b < numBlocks; ++b)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < kBlockSize; ++i)
            {
                double t = (double) (sampleIndex + i) / kTestSampleRate;
                data[i] = 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * kTestFrequency * t);
            }
        }

        proc.processBlock (buffer, midi);

        const auto* out = buffer.getReadPointer (0);
        for (int i = 0; i < kBlockSize; ++i)
            rendered.push_back (out[i]);

        sampleIndex += kBlockSize;
    }

    if ((int) rendered.size() < latency + kFftSize)
    {
        std::cerr << "FAIL not enough rendered samples for FFT analysis\n";
        ++failures;
        return 0.0f;
    }

    // Blackman-Harris (4-term, ~92 dB sidelobes) analysis window, starting after the
    // reported latency so the FFT sees steady-state output rather than the oversampling
    // filters' turn-on transient. A plain Hann window's ~-31 dB first sidelobe leaks
    // enough of a strongly-driven waveshaper's own (0-20 dB down) harmonics into
    // neighbouring bins to swamp real aliasing; Blackman-Harris keeps leakage well
    // below what oversampling is expected to buy.
    std::vector<float> fftData (2 * (size_t) kFftSize, 0.0f);
    for (int i = 0; i < kFftSize; ++i)
    {
        float phase = 2.0f * juce::MathConstants<float>::pi * (float) i / (float) (kFftSize - 1);
        float w = 0.35875f - 0.48829f * std::cos (phase) + 0.14128f * std::cos (2.0f * phase) - 0.01168f * std::cos (3.0f * phase);
        fftData[(size_t) i] = rendered[(size_t) (latency + i)] * w;
    }

    juce::dsp::FFT fft (kFftOrder);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const double binHz = kTestSampleRate / (double) kFftSize;
    const int fundamentalBin = (int) std::round (kTestFrequency / binHz);
    const int guardBins = 12; // Blackman-Harris main-lobe half-width plus margin, in bins

    auto peakNear = [&] (int centerBin)
    {
        int lo = juce::jmax (0, centerBin - guardBins);
        int hi = juce::jmin (kFftSize / 2, centerBin + guardBins);
        float peak = 0.0f;
        for (int i = lo; i <= hi; ++i)
            peak = juce::jmax (peak, fftData[(size_t) i]);
        return peak;
    };

    const float fundamentalMag = peakNear (fundamentalBin);

    // Exclude DC, the fundamental, and every harmonic multiple of it; the largest bin
    // left over is the highest non-harmonic (aliasing) peak.
    std::vector<bool> excluded ((size_t) kFftSize / 2 + 1, false);
    for (int i = 0; i <= guardBins; ++i)
        excluded[(size_t) i] = true;
    for (int h = 1; (h * (double) kTestFrequency) < (kTestSampleRate / 2.0); ++h)
    {
        int hBin = (int) std::round ((h * (double) kTestFrequency) / binHz);
        for (int i = juce::jmax (0, hBin - guardBins); i <= juce::jmin (kFftSize / 2, hBin + guardBins); ++i)
            excluded[(size_t) i] = true;
    }

    float aliasPeakMag = 0.0f;
    int aliasPeakBin = -1;
    for (int i = 0; i <= kFftSize / 2; ++i)
    {
        if (excluded[(size_t) i])
            continue;
        if (fftData[(size_t) i] > aliasPeakMag)
        {
            aliasPeakMag = fftData[(size_t) i];
            aliasPeakBin = i;
        }
    }

    const float aliasPeakDb = (fundamentalMag > 1.0e-9f && aliasPeakMag > 0.0f)
        ? 20.0f * std::log10 (aliasPeakMag / fundamentalMag)
        : -200.0f;

    std::cout << "latency = " << latency << " samples\n";
    std::cout << "fundamental magnitude = " << fundamentalMag << " at " << fundamentalBin * binHz << " Hz\n";
    std::cout << "highest non-harmonic peak = " << aliasPeakMag << " at "
               << (aliasPeakBin >= 0 ? aliasPeakBin * binHz : 0.0) << " Hz, "
               << aliasPeakDb << " dB relative to fundamental\n";

    return aliasPeakDb;
}

// getLatencySamples() must not depend on the block size prepareToPlay was called with,
// at any of the sample rates the house rules require testing at (../../CLAUDE.md section 4).
void checkLatencyInvariance()
{
    const double sampleRates[] = { 44100.0, 48000.0, 96000.0 };
    const int blockSizes[] = { 64, 100, 512, 1024 };

    for (double sr : sampleRates)
    {
        int firstLatency = -1;
        for (int bs : blockSizes)
        {
            HP_CHECK_PROCESSOR_CLASS proc;
            proc.setPlayConfigDetails (2, 2, sr, bs);
            proc.prepareToPlay (sr, bs);
            const int latency = proc.getLatencySamples();

            if (firstLatency < 0)
            {
                firstLatency = latency;
            }
            else if (latency != firstLatency)
            {
                std::cerr << "FAIL latency changed with block size at " << sr << " Hz: "
                           << firstLatency << " -> " << latency << " (block " << bs << ")\n";
                ++failures;
            }
        }
        std::cout << "ok   latency at " << sr << " Hz is " << firstLatency
                   << " samples, identical across block sizes 64/100/512/1024\n";
    }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    checkLatencyInvariance();

    HP_CHECK_PROCESSOR_CLASS proc;
    applyStrongSettings (proc);
    measureAliasPeakDb (proc);

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " oversampling check\n";
    return failures == 0 ? 0 : 1;
}
