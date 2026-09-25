// hp_frequency_sweep_BassMaximiser: confirms the Frequency control (tooltip: "Crossover
// frequency -- bass below this point is boosted", BassMaximiserEditor.cpp) genuinely changes
// the output, i.e. that the crossover it drives is real and audible -- see the investigation
// note in BassMaximiserProcessor.cpp's processBassCompression() (the flagged defect was that
// function's own now-removed, unused `frequency` argument; the argument was redundant because
// the crossover is already applied earlier in the chain, in updateFilters(), which builds
// bassFilter/highPassFilter from the same Frequency parameter every block).
//
// Method: render a wideband probe (40 Hz, always below the 20-500 Hz Frequency range's crossover
// candidates when Frequency is high, close to it when Frequency is low; 2000 Hz, always well
// above every crossover setting) through the real BassMaximiserProcessor at two very different
// Frequency settings (30 Hz and 400 Hz) and compare output RMS. If moving Frequency across most
// of its range failed to move the output at all, Frequency would be provably dead; it is not.
// Exit 0 on pass.

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
        fail("unknown parameter " + id);
}

double renderOutputRms(float frequencyHz, double sampleRate, int numSamples)
{
    HP_CHECK_PROCESSOR_CLASS proc;
    proc.setPlayConfigDetails(2, 2, sampleRate, 512);
    proc.prepareToPlay(sampleRate, 512);

    auto& apvts = proc.getValueTreeState();
    setParam(apvts, "frequency", frequencyHz);
    setParam(apvts, "boost", 12.0f);
    setParam(apvts, "harmonics", 0.0f);   // isolate the boosted/compressed bass band's contribution
    setParam(apvts, "tightness", 100.0f); // maximise processBassCompression()'s contribution
    setParam(apvts, "outputGain", 0.0f);
    setParam(apvts, "phaseInvert", 0.0f);
    setParam(apvts, "bypass", 0.0f);

    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer midi;
    double sumSq = 0.0;
    int rendered = 0, sampleIndex = 0;

    while (rendered < numSamples)
    {
        const int n = juce::jmin(512, numSamples - rendered);
        buf.setSize(2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = buf.getWritePointer(ch);
            for (int i = 0; i < n; ++i)
            {
                const double t = (double) (sampleIndex + i) / sampleRate;
                d[i] = (float) (0.4 * std::sin(2.0 * juce::MathConstants<double>::pi * 40.0   * t)
                               + 0.4 * std::sin(2.0 * juce::MathConstants<double>::pi * 2000.0 * t));
            }
        }
        proc.processBlock(buf, midi);
        for (int i = 0; i < n; ++i)
            sumSq += (double) buf.getSample(0, i) * (double) buf.getSample(0, i);
        rendered += n;
        sampleIndex += n;
    }
    return std::sqrt(sumSq / (double) numSamples);
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000.0;
    const int numSamples = (int) (0.25 * fs); // 250ms: past filter/envelope settling

    const double rmsLow  = renderOutputRms(30.0f,  fs, numSamples);
    const double rmsHigh = renderOutputRms(400.0f, fs, numSamples);

    std::cout << "Frequency=30 Hz  -> output RMS = " << rmsLow  << "\n";
    std::cout << "Frequency=400 Hz -> output RMS = " << rmsHigh << "\n";

    const double diffPct = 100.0 * std::abs(rmsHigh - rmsLow) / juce::jmax(rmsLow, rmsHigh);
    std::cout << "difference = " << diffPct << " %\n";

    if (diffPct < 1.0)
        fail("sweeping Frequency from 30 Hz to 400 Hz changed output by less than 1% -- Frequency control appears dead");

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " bass maximiser frequency sweep check\n";
    return failures == 0 ? 0 : 1;
}
