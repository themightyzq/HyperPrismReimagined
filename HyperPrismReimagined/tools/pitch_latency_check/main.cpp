// hp_pitch_latency_PitchChanger: measures the delay of Pitch Changer's shifted (wet) path and
// checks that the plugin reports it and lines the dry path up with it.
//
// Method: a Hann-windowed 30 ms tone burst (1 kHz) is rendered through the plugin at Mix 100 %
// for pitch settings 0, +/-1, +/-7, +/-12 and +24 semitones, at 44.1, 48 and 96 kHz, and at
// four burst start offsets (so the burst lands at different points of the shifter's analysis
// hop). The delay is the shift of the burst's energy centroid (sum t*x^2 / sum x^2) from input
// to output. A pure delay moves the centroid by exactly that delay; transposition does not
// move it.
//   1. Every measured wet delay must be within 1 ms of getLatencySamples(), at every pitch,
//      offset and rate: the delay is a constant and the plugin reports it.
//   2. Mix 0 (dry only) and Bypass must each equal the input delayed by exactly
//      getLatencySamples(), so dry and wet stay aligned when mixed and the timing does not jump
//      when Bypass is toggled (before this fix the dry path and bypassed audio were undelayed and
//      the plugin reported 0 latency).
//   3. getLatencySamples() is identical for block sizes 64, 100, 512 and 1024.
// Exit 0 on pass. Registered with CTest in CMakeLists.txt (add_hyperprism_light_check).

#include HP_CHECK_PROCESSOR_HEADER
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
int failures = 0;

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

std::vector<float> render (const std::vector<float>& in, double sr, int block, float semitones, float mix,
                           int* latencyOut, bool bypass = false)
{
    HP_CHECK_PROCESSOR_CLASS proc;
    setParam (proc, "pitchShift", semitones);
    setParam (proc, "mix", mix);
    setParam (proc, "bypass", bypass ? 1.0f : 0.0f);
    proc.setPlayConfigDetails (2, 2, sr, block);
    proc.prepareToPlay (sr, block);
    if (latencyOut != nullptr)
        *latencyOut = proc.getLatencySamples();

    std::vector<float> out (in.size(), 0.0f);
    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;
    for (size_t start = 0; start < in.size(); start += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, in.size() - start);
        buffer.setSize (2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                buffer.setSample (ch, i, in[start + (size_t) i]);
        proc.processBlock (buffer, midi);
        for (int i = 0; i < n; ++i)
            out[start + (size_t) i] = buffer.getSample (0, i);
    }
    return out;
}

double centroid (const std::vector<float>& x)
{
    double num = 0.0, den = 0.0;
    for (size_t i = 0; i < x.size(); ++i)
    {
        const double e = (double) x[i] * x[i];
        num += e * (double) i;
        den += e;
    }
    return den > 0.0 ? num / den : -1.0;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    const float pitches[] = { 0.0f, 1.0f, -1.0f, 7.0f, -7.0f, 12.0f, -12.0f, 24.0f };

    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        int reported = -1;
        render (std::vector<float> (16, 0.0f), sr, 512, 0.0f, 100.0f, &reported);

        double worst = 0.0, minDelay = 1.0e9, maxDelay = -1.0e9;
        for (float semis : pitches)
            for (int offset : { 0, 331, 797, 1201 })
            {
                const int burstLen = (int) (0.030 * sr);
                const int start = (int) (0.5 * sr) + offset;
                std::vector<float> in ((size_t) (1.5 * sr), 0.0f);
                for (int i = 0; i < burstLen; ++i)
                {
                    const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (burstLen - 1));
                    in[(size_t) (start + i)] = (float) (0.5 * w * std::sin (juce::MathConstants<double>::twoPi * 1000.0 * i / sr));
                }
                const auto out = render (in, sr, 512, semis, 100.0f, nullptr);
                const double delayMs = (centroid (out) - centroid (in)) * 1000.0 / sr;
                minDelay = juce::jmin (minDelay, delayMs);
                maxDelay = juce::jmax (maxDelay, delayMs);
                worst = juce::jmax (worst, std::abs (delayMs - reported * 1000.0 / sr));
            }

        const bool pass = reported > 0 && worst <= 1.0;
        std::cout << (pass ? "ok   " : "FAIL ") << sr << " Hz: reported latency " << reported << " samples ("
                  << reported * 1000.0 / sr << " ms); measured wet delay " << minDelay << " .. " << maxDelay
                  << " ms over pitch 0, +/-1, +/-7, +/-12, +24 st; largest difference " << worst << " ms\n";
        if (! pass)
            ++failures;

        // 2. Dry path alignment at Mix 0, and Bypass.
        for (bool bypass : { false, true })
        {
            juce::Random rng (5);
            std::vector<float> noise ((size_t) (0.5 * sr));
            for (auto& v : noise)
                v = rng.nextFloat() * 0.8f - 0.4f;
            int latency = -1;
            const auto out = render (noise, sr, 512, 7.0f, bypass ? 100.0f : 0.0f, &latency, bypass);
            float err = 0.0f;
            for (size_t n = 0; n < out.size(); ++n)
            {
                const float expected = (int) n >= latency ? noise[n - (size_t) juce::jmax (0, latency)] : 0.0f;
                err = juce::jmax (err, std::abs (out[n] - expected));
            }
            const bool dryPass = latency > 0 && err <= 1.0e-6f;
            std::cout << (dryPass ? "ok   " : "FAIL ") << sr << " Hz: " << (bypass ? "Bypass" : "Mix 0") << " output vs input delayed by "
                      << latency << " samples, max error " << err << "\n";
            if (! dryPass)
                ++failures;
        }

        // 3. Block-size invariance of the reported latency.
        bool same = true;
        for (int bs : { 64, 100, 1024 })
        {
            int l = -1;
            render (std::vector<float> (16, 0.0f), sr, bs, 0.0f, 100.0f, &l);
            same = same && l == reported;
        }
        std::cout << (same ? "ok   " : "FAIL ") << sr << " Hz: reported latency "
                  << (same ? "identical" : "DIFFERS") << " for block sizes 64/100/512/1024\n";
        if (! same)
            ++failures;
    }

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " pitch latency check\n";
    return failures == 0 ? 0 : 1;
}
