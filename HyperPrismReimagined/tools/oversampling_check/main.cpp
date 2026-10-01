// hp_oversampling_<Effect>: proves that the plugin's nonlinear stage
// (saturation waveshaper / harmonic generator / bit-rate quantiser) is running through its
// 4x juce::dsp::Oversampling wrapper, and that the reported latency is block-size invariant
// at 44.1/48/96 kHz. Meant to be built per plugin the same way tools/state_check/main.cpp
// is: compile definitions select the processor (HP_CHECK_PROCESSOR_HEADER /
// HP_CHECK_PROCESSOR_CLASS) and which plugin-specific "strong setting" and parameter IDs to
// drive (HP_CHECK_TUBETAPE / HP_CHECK_HARMONICEXCITER / HP_CHECK_SONICDECIMATOR).
// Registered with CTest in CMakeLists.txt (add_hyperprism_console_check). Every plugin also
// gets the bypass-latency, block-size-invariance and oversized-host-block checks below;
// HarmonicExciter and SonicDecimator the dry/wet alignment check; HarmonicExciter its high-pass
// check; SonicDecimator its default-transparency check. Exit 0 on pass.

#include HP_CHECK_PROCESSOR_HEADER
#include <iostream>
#include <vector>
#include <cmath>
#include <functional>

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


//==============================================================================
// Latency, chunking and bypass checks shared by all three oversampled plugins (first written
// for HarmonicExciter's dry/wet alignment fix, generalised to SonicDecimator and TubeTape).
// HarmonicExciter and SonicDecimator output delayedDry * (1 - mix) + wet * mix (exciter:
// delayedDry + wet * mix), so at mix 0 the output is exactly the input delayed by the latency.

// Renders `input` (same signal on both channels) through a freshly prepared processor.
// `preparedBlock` goes to prepareToPlay; the host then sends blocks of `hostBlock` samples,
// which may be larger than preparedBlock (the processor must chunk internally).
std::vector<float> renderPlugin (const std::vector<float>& input, int preparedBlock, int hostBlock,
                                 std::function<void (juce::AudioProcessorValueTreeState&)> configure,
                                 int* latencyOut = nullptr, double sampleRate = kTestSampleRate)
{
    HP_CHECK_PROCESSOR_CLASS proc;
    if (configure)
        configure (proc.getValueTreeState());

    proc.setPlayConfigDetails (2, 2, sampleRate, preparedBlock);
    proc.prepareToPlay (sampleRate, preparedBlock);
    if (latencyOut != nullptr)
        *latencyOut = proc.getLatencySamples();

    std::vector<float> out (input.size(), 0.0f);
    juce::AudioBuffer<float> buffer (2, hostBlock);
    juce::MidiBuffer midi;

    for (size_t start = 0; start < input.size(); start += (size_t) hostBlock)
    {
        const int n = (int) std::min ((size_t) hostBlock, input.size() - start);
        buffer.setSize (2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                buffer.setSample (ch, i, input[start + (size_t) i]);

        proc.processBlock (buffer, midi);

        float chDiff = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            out[start + (size_t) i] = buffer.getSample (0, i);
            chDiff = juce::jmax (chDiff, std::abs (buffer.getSample (0, i) - buffer.getSample (1, i)));
        }
        if (chDiff > 1.0e-6f)
        {
            std::cerr << "FAIL left and right differ for identical input (" << chDiff << ")\n";
            ++failures;
        }
    }
    return out;
}

std::vector<float> testNoise (size_t length)
{
    std::vector<float> x (length);
    juce::Random rng (42);
    for (auto& v : x)
        v = rng.nextFloat() * 0.8f - 0.4f;
    return x;
}

// Largest |out[n] - in[n - latency]| over the whole render.
float maxDelayedDryError (const std::vector<float>& in, const std::vector<float>& out, int latency)
{
    float err = 0.0f;
    for (size_t n = 0; n < out.size(); ++n)
    {
        const float expected = (int) n >= latency ? in[n - (size_t) latency] : 0.0f;
        err = juce::jmax (err, std::abs (out[n] - expected));
    }
    return err;
}

float maxAbsDiff (const std::vector<float>& a, const std::vector<float>& b)
{
    float d = 0.0f;
    for (size_t i = 0; i < a.size(); ++i)
        d = juce::jmax (d, std::abs (a[i] - b[i]));
    return d;
}

using Configure = std::function<void (juce::AudioProcessorValueTreeState&)>;

// The reported latency, measured once at 48 kHz / 512.
int reportedLatency()
{
    int latency = -1;
    renderPlugin (std::vector<float> (16, 0.0f), 512, 512, nullptr, &latency);
    return latency;
}

// Mix 0 (dry only): output must be the input delayed by exactly the reported latency (before
// the fix it was undelayed while latency was reported).
void checkDryAlignment (const std::vector<float>& noise)
{
    int latency = -1;
    auto out = renderPlugin (noise, 512, 512, [] (auto& apvts) { setParam (apvts, "mix", 0.0f); }, &latency);
    const float err = maxDelayedDryError (noise, out, latency);
    if (latency <= 0 || err > 1.0e-6f)
    {
        std::cerr << "FAIL mix 0: output is not the input delayed by latency " << latency
                   << " (max error " << err << ")\n";
        ++failures;
    }
    else
        std::cout << "ok   mix 0: output == input delayed by the reported " << latency << " samples\n";
}

// Bypass routes audio through the same delay, so timing does not jump when it is toggled.
void checkBypassLatency (const std::vector<float>& noise, int latency)
{
    auto out = renderPlugin (noise, 512, 512, [] (auto& apvts) { setParam (apvts, "bypass", 1.0f); });
    const float err = maxDelayedDryError (noise, out, latency);
    if (latency <= 0 || err > 1.0e-6f)
    {
        std::cerr << "FAIL bypass: output is not the input delayed by the reported latency " << latency
                   << " (max error " << err << ")\n";
        ++failures;
    }
    else
        std::cout << "ok   bypass: output == input delayed by " << latency << " samples\n";
}

// Prepared-and-driven at 64 vs 1000 (non-power-of-two) gives the same output, and a host block
// larger than the prepared size (4096 into a 512-sample preparation) is processed in chunks and
// matches the 512-block render (before the fix it overran the oversampler's buffers).
void checkBlockSizes (const std::vector<float>& noise, Configure configure)
{
    const auto ref = renderPlugin (noise, 512, 512, configure);
    {
        int latencyA = -1, latencyB = -1;
        auto a = renderPlugin (noise, 64, 64, configure, &latencyA);
        auto b = renderPlugin (noise, 1000, 1000, configure, &latencyB);
        const float d = maxAbsDiff (a, b);
        if (latencyA != latencyB || d > 1.0e-5f)
        {
            std::cerr << "FAIL block sizes 64 vs 1000: latency " << latencyA << "/" << latencyB
                       << ", max diff " << d << "\n";
            ++failures;
        }
        else
            std::cout << "ok   block sizes 64 vs 1000 match (max diff " << d << ")\n";
    }
    {
        auto big = renderPlugin (noise, 512, 4096, configure);
        const float d = maxAbsDiff (big, ref);
        if (d > 1.0e-5f)
        {
            std::cerr << "FAIL oversized host block (4096 > prepared 512) differs by " << d << "\n";
            ++failures;
        }
        else
            std::cout << "ok   oversized host block (4096 > prepared 512) matches (max diff " << d << ")\n";
    }
}

#if HP_CHECK_HARMONICEXCITER
void checkHarmonicExciter()
{
    const auto noise = testNoise (20000);
    const int latency = reportedLatency();

    checkDryAlignment (noise);
    checkBypassLatency (noise, latency);

    // Impulse at mix 50: one dominant peak at the latency and no second peak at sample 0
    // (before the fix the undelayed dry impulse sat at sample 0). The half-band IIR
    // filters are causal but not linear-phase, so some wet signal legitimately starts
    // before the nominal latency; it must stay well below the main peak.
    {
        std::vector<float> imp (4096, 0.0f);
        imp[0] = 1.0f;
        auto out = renderPlugin (imp, 512, 512,
                                 [] (auto& apvts) { setParam (apvts, "mix", 50.0f); });
        size_t peakIndex = 0;
        float peak = 0.0f;
        for (size_t i = 0; i < out.size(); ++i)
            if (std::abs (out[i]) > peak) { peak = std::abs (out[i]); peakIndex = i; }

        float early = 0.0f;
        for (int i = 0; i < latency; ++i)
            early = juce::jmax (early, std::abs (out[(size_t) i]));

        if ((int) peakIndex < latency || (int) peakIndex > latency + 2 || early > 0.3f * peak
            || std::abs (out[0]) > 0.3f * peak)
        {
            std::cerr << "FAIL mix 50 impulse: peak " << peak << " at " << peakIndex << ", latency "
                       << latency << ", largest value before latency " << early << "\n";
            ++failures;
        }
        else
            std::cout << "ok   mix 50 impulse: single peak " << peak << " at " << peakIndex
                       << " (latency " << latency << "), |out[0]| " << std::abs (out[0])
                       << ", max before latency " << early << "\n";
    }

    // The band filter is a high-pass: at Frequency 5 kHz a 100 Hz tone must generate far
    // less harmonic (wet) signal than a 10 kHz tone of the same level. With the old
    // low-pass it was the other way round.
    {
        auto wetRms = [&] (double freqHz)
        {
            std::vector<float> sine (24000);
            for (size_t i = 0; i < sine.size(); ++i)
                sine[i] = 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * freqHz * (double) i / kTestSampleRate);

            auto out = renderPlugin (sine, 512, 512, [] (auto& apvts)
            {
                setParam (apvts, "mix", 100.0f);
                setParam (apvts, "frequency", 5000.0f);
            });

            double sum = 0.0;
            int count = 0;
            for (size_t n = 8000; n < out.size(); ++n)
            {
                const double wet = out[n] - sine[n - (size_t) latency];
                sum += wet * wet;
                ++count;
            }
            return (float) std::sqrt (sum / count);
        };

        const float low = wetRms (100.0);
        const float high = wetRms (10000.0);
        if (! (high > 0.01f && low < 0.05f * high))
        {
            std::cerr << "FAIL high-pass: wet RMS at 100 Hz " << low << " vs 10 kHz " << high << "\n";
            ++failures;
        }
        else
            std::cout << "ok   high-pass: wet RMS at 100 Hz " << low << " vs 10 kHz " << high << "\n";
    }

    checkBlockSizes (noise, [] (auto& apvts) { setParam (apvts, "mix", 50.0f); });
}
#endif

#if HP_CHECK_SONICDECIMATOR
// At its default settings (Rate 44100 Hz, Bit Depth 16) on a 44.1 kHz host, SonicDecimator was
// transparent apart from 16-bit quantisation before oversampling was added: the rate stage
// passed audio through whenever Rate >= the host rate. Oversampling made the rate stage compare
// Rate with the 4x rate instead, so it sample-and-held at 44.1 kHz behind a 19.8 kHz low-pass.
// The reference here is the same 4x oversampler doing up then down with nothing in between, so
// the only allowed difference is the 16-bit quantiser's (about 3e-5).
void checkDefaultTransparency()
{
    constexpr double hostRate = 44100.0;
    const auto noise = testNoise (22050);

    int latency = -1;
    auto out = renderPlugin (noise, 512, 512, nullptr, &latency, hostRate);

    juce::dsp::Oversampling<float> reference (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    reference.initProcessing (512);
    reference.reset();
    std::vector<float> ref (noise.size(), 0.0f);
    juce::AudioBuffer<float> buf (2, 512);
    for (size_t start = 0; start < noise.size(); start += 512)
    {
        const int n = (int) std::min ((size_t) 512, noise.size() - start);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                buf.setSample (ch, i, noise[start + (size_t) i]);
        juce::dsp::AudioBlock<float> block (buf.getArrayOfWritePointers(), 2, (size_t) n);
        reference.processSamplesUp (block);
        reference.processSamplesDown (block);
        for (int i = 0; i < n; ++i)
            ref[start + (size_t) i] = buf.getSample (0, i);
    }

    const float d = maxAbsDiff (out, ref);
    if (d > 2.0e-4f || latency != (int) reference.getLatencyInSamples())
    {
        std::cerr << "FAIL defaults at 44.1 kHz are not transparent: max difference from the bare "
                     "oversampler " << d << " (latency " << latency << ")\n";
        ++failures;
    }
    else
        std::cout << "ok   defaults at 44.1 kHz: output matches the bare 4x oversampler within " << d
                  << " (16-bit quantisation only)\n";
}

void checkSonicDecimator()
{
    const auto noise = testNoise (20000);
    const int latency = reportedLatency();

    checkDefaultTransparency();
    checkDryAlignment (noise);
    checkBypassLatency (noise, latency);
    checkBlockSizes (noise, [] (auto& apvts)
    {
        setParam (apvts, "mix", 50.0f);
        setParam (apvts, "bitDepth", 6.0f);
        setParam (apvts, "sampleRate", 8000.0f);
    });
}
#endif

#if HP_CHECK_TUBETAPE
void checkTubeTape()
{
    const auto noise = testNoise (20000);
    const int latency = reportedLatency();

    checkBypassLatency (noise, latency);
    checkBlockSizes (noise, [] (auto& apvts) { setParam (apvts, "drive", 70.0f); });
}
#endif
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    checkLatencyInvariance();

    HP_CHECK_PROCESSOR_CLASS proc;
    applyStrongSettings (proc);
    measureAliasPeakDb (proc);

   #if HP_CHECK_HARMONICEXCITER
    checkHarmonicExciter();
   #elif HP_CHECK_SONICDECIMATOR
    checkSonicDecimator();
   #elif HP_CHECK_TUBETAPE
    checkTubeTape();
   #endif

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " oversampling check\n";
    return failures == 0 ? 0 : 1;
}
