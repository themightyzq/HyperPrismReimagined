// hp_smoothing_<Effect>: proves the listed continuous parameters are smoothed (no zipper noise
// or clicks when a host automates them). HP_SMOOTH_PARAMS (a compile definition set per plugin
// in CMakeLists.txt) is a comma-separated list of "paramID:mode[:delta[:offsetMs]]" entries.
// HP_SMOOTH_SETUP (optional) is a comma-separated list of "paramID=value" settings in real units
// applied to both renders first, for a parameter that only matters once another one is set
// (e.g. Frequency Shifter's Mix does nothing while the shift is 0 Hz). For each entry
// the plugin is rendered twice from the same input at 48 kHz in 256-sample blocks: A with the
// parameter held at its default, B identical except that the parameter steps by `delta`
// (normalised, default 0.3, applied downwards if it would leave 0..1) at the start of block 188
// (about 1 s in, so delay lines and feedback paths are already full).
//
//   mode r (ramp): input is independent white noise per channel. d = B - A after the step.
//      An unsmoothed parameter makes d jump to full size at once, so RMS(d) over the first
//      1 ms is close to RMS(d) over 50-100 ms later. A parameter smoothed over 20-50 ms ramps,
//      so the first-millisecond RMS is a small fraction of the later one. Pass: ratio < 0.3,
//      and the later RMS must be audible (> 1e-4), i.e. the parameter really does something.
//      offsetMs moves both windows later, for a parameter whose effect first reaches the output
//      one delay time after the change (feedback in a delay line).
//   mode c (click): for delay-time parameters, where any glide changes the waveform at once.
//      Input is a 110 Hz sine. An unsmoothed delay change jumps the read point, a step in
//      the output many times larger than the sine's own sample-to-sample slope. Pass: the
//      largest |B[n] - B[n-1]| in the 20 ms after the step is under 4x the larger of the
//      largest |A[n] - A[n-1]| over the same samples and the input sine's own largest step
//      (a smoothed glide only scales the slope by the small Doppler factor; use a small delta,
//      e.g. 0.02). The input's slope is the floor because the steady output can be nearly
//      silent when the dry and delayed signals happen to cancel at 110 Hz.
//   mode h (high frequency): for a delay time that feeds something that smears a click (Reverb's
//      Pre-Delay feeds the reverb). Input is the same 110 Hz sine. A jump in the read point puts
//      broadband energy into the output, which a 110 Hz sine through a linear reverb never has.
//      Pass: the RMS of the output's second difference (a high-pass that weights 2 kHz about
//      330x more than 110 Hz) over the 100 ms after the change is under 2x the same for A.
//
// Exit 0 on pass. Registered with CTest by add_hyperprism_smoothing_check().

#include HP_CHECK_PROCESSOR_HEADER
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlock = 256;
constexpr int kStepBlock = 188;   // about 1 s in, so delay lines and feedback paths are full
constexpr int kNumBlocks = 270;   // room for a 250 ms offset plus the 100 ms window
constexpr int kStepSample = kStepBlock * kBlock;

juce::RangedAudioParameter* findParam (juce::AudioProcessor& proc, const juce::String& id)
{
    for (auto* p : proc.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (r->paramID == id)
                return r;
    return nullptr;
}

std::vector<float> makeInput (char mode, int channel, int length)
{
    std::vector<float> x ((size_t) length);
    if (mode == 'c' || mode == 'h')
    {
        for (int i = 0; i < length; ++i)
            x[(size_t) i] = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 110.0 * i / kSampleRate);
    }
    else
    {
        juce::Random rng (1000 + channel);
        for (auto& v : x)
            v = 0.25f * (rng.nextFloat() * 2.0f - 1.0f);
    }
    return x;
}

void applySetup (juce::AudioProcessor& proc)
{
   #ifdef HP_SMOOTH_SETUP
    for (const auto& item : juce::StringArray::fromTokens (HP_SMOOTH_SETUP, ",", ""))
    {
        const auto id = item.upToFirstOccurrenceOf ("=", false, false).trim();
        const auto value = item.fromFirstOccurrenceOf ("=", false, false).getFloatValue();
        if (auto* p = findParam (proc, id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
        else
            std::cerr << "FAIL unknown setup parameter " << id << "\n";
    }
   #else
    juce::ignoreUnused (proc);
   #endif
}

// Renders kNumBlocks blocks; if `stepTo` >= 0 the parameter is set to it before block kStepBlock.
// Returns channel 0 (and channel 1 appended, so both channels are compared).
std::vector<float> render (const juce::String& id, char mode, float startValue, float stepTo, bool& ok)
{
    HP_CHECK_PROCESSOR_CLASS proc;
    auto* param = findParam (proc, id);
    if (param == nullptr)
    {
        std::cerr << "FAIL unknown parameter " << id << "\n";
        ok = false;
        return {};
    }

    applySetup (proc);
    param->setValueNotifyingHost (startValue);
    proc.setPlayConfigDetails (2, 2, kSampleRate, kBlock);
    proc.prepareToPlay (kSampleRate, kBlock);

    const int length = kNumBlocks * kBlock;
    const auto in0 = makeInput (mode, 0, length);
    const auto in1 = makeInput (mode, 1, length);

    std::vector<float> out ((size_t) (2 * length), 0.0f);
    juce::AudioBuffer<float> buffer (2, kBlock);
    juce::MidiBuffer midi;

    for (int b = 0; b < kNumBlocks; ++b)
    {
        if (b == kStepBlock && stepTo >= 0.0f)
            param->setValueNotifyingHost (stepTo);

        for (int i = 0; i < kBlock; ++i)
        {
            buffer.setSample (0, i, in0[(size_t) (b * kBlock + i)]);
            buffer.setSample (1, i, in1[(size_t) (b * kBlock + i)]);
        }
        proc.processBlock (buffer, midi);
        for (int i = 0; i < kBlock; ++i)
        {
            out[(size_t) (b * kBlock + i)] = buffer.getSample (0, i);
            out[(size_t) (length + b * kBlock + i)] = buffer.getSample (1, i);
        }
    }
    return out;
}

double rms (const std::vector<float>& a, const std::vector<float>& b, int length, int from, int to)
{
    double sum = 0.0;
    int count = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (int n = from; n < to; ++n)
        {
            const double d = (double) b[(size_t) (ch * length + n)] - (double) a[(size_t) (ch * length + n)];
            sum += d * d;
            ++count;
        }
    return std::sqrt (sum / juce::jmax (1, count));
}

double maxSlope (const std::vector<float>& x, int length, int from, int to)
{
    double m = 0.0;
    for (int ch = 0; ch < 2; ++ch)
        for (int n = from; n < to; ++n)
            m = juce::jmax (m, (double) std::abs (x[(size_t) (ch * length + n)] - x[(size_t) (ch * length + n - 1)]));
    return m;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    int failures = 0;
    const int length = kNumBlocks * kBlock;
    const auto entries = juce::StringArray::fromTokens (HP_SMOOTH_PARAMS, ",", "");

    for (const auto& entry : entries)
    {
        const auto parts = juce::StringArray::fromTokens (entry, ":", "");
        if (parts.size() < 2)
            continue;

        const juce::String id = parts[0].trim();
        const char mode = (char) parts[1].trim()[0];
        const float delta = parts.size() > 2 ? parts[2].getFloatValue() : 0.3f;
        const int offset = parts.size() > 3 ? (int) (parts[3].getFloatValue() * 0.001 * kSampleRate) : 0;

        float startValue = 0.0f;
        {
            HP_CHECK_PROCESSOR_CLASS probe;
            if (auto* p = findParam (probe, id))
                startValue = p->getDefaultValue();
        }
        const float stepTo = (startValue + delta <= 1.0f) ? startValue + delta : startValue - delta;

        bool ok = true;
        const auto a = render (id, mode, startValue, -1.0f, ok);
        const auto b = render (id, mode, startValue, stepTo, ok);
        if (! ok)
        {
            ++failures;
            continue;
        }

        if (mode == 'h')
        {
            auto hfRms = [&] (const std::vector<float>& x)
            {
                double sum = 0.0;
                int count = 0;
                const int to = kStepSample + (int) (0.100 * kSampleRate);
                for (int ch = 0; ch < 2; ++ch)
                    for (int n = kStepSample; n < to; ++n)
                    {
                        const size_t i = (size_t) (ch * length + n);
                        const double d2 = (double) x[i] - 2.0 * x[i - 1] + x[i - 2];
                        sum += d2 * d2;
                        ++count;
                    }
                return std::sqrt (sum / juce::jmax (1, count));
            };
            const double hfA = hfRms (a);
            const double hfB = hfRms (b);
            const double ratio = hfB / juce::jmax (1.0e-12, hfA);
            const double effect = rms (a, b, length, kStepSample, length);
            const bool pass = ratio < 2.0 && effect > 1.0e-4;
            std::cout << (pass ? "ok   " : "FAIL ") << id << " (high-frequency): second-difference RMS after change "
                      << hfB << " vs steady " << hfA << " (ratio " << ratio << ")\n";
            if (! pass)
                ++failures;
        }
        else if (mode == 'c')
        {
            const int to = kStepSample + (int) (0.020 * kSampleRate);
            const double inputSlope = 0.5 * juce::MathConstants<double>::twoPi * 110.0 / kSampleRate;
            const double slopeA = juce::jmax (maxSlope (a, length, kStepSample, to), inputSlope);
            const double slopeB = maxSlope (b, length, kStepSample, to);
            const double ratio = slopeB / juce::jmax (1.0e-9, slopeA);
            const double effect = rms (a, b, length, kStepSample, length); // the change must be audible
            const bool pass = ratio < 4.0 && effect > 1.0e-4;
            std::cout << (pass ? "ok   " : "FAIL ") << id << " (click): max step after change "
                      << slopeB << " vs steady " << slopeA << " (ratio " << ratio << ")\n";
            if (! pass)
                ++failures;
        }
        else
        {
            const int from = kStepSample + offset;
            const double early = rms (a, b, length, from, from + 48);
            const double late = rms (a, b, length, from + 2400, juce::jmin (length, from + 4800));
            const double ratio = early / juce::jmax (1.0e-12, late);
            const bool pass = ratio < 0.3 && late > 1.0e-4;
            std::cout << (pass ? "ok   " : "FAIL ") << id << " (ramp): first-ms RMS change " << early
                      << " vs 50-100 ms " << late << " (ratio " << ratio << ")\n";
            if (! pass)
                ++failures;
        }
    }

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " smoothing check\n";
    return failures == 0 ? 0 : 1;
}
