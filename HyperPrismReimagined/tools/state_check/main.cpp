// hp_state_<Effect>: proves that a session saved by the plugin's
// pre-APVTS getStateInformation (2026-09-23 migration of HarmonicExciter and NoiseGate)
// restores identically through the new setStateInformation, and that the new format
// round-trips into a fresh instance. Built per plugin by add_hyperprism_state_check() in
// CMakeLists.txt, which selects the processor through compile definitions (the same
// pattern as tools/ui_snapshot/main.cpp). Exit 0 on pass.

#include HP_CHECK_PROCESSOR_HEADER
#include <iostream>
#include <cmath>

namespace
{
int failures = 0;

void expectNear (const char* what, float actual, float expected, float tol = 1.0e-3f)
{
    if (std::abs (actual - expected) > tol)
    {
        std::cerr << "FAIL " << what << ": got " << actual << ", expected " << expected << "\n";
        ++failures;
    }
    else
        std::cout << "ok   " << what << " = " << actual << "\n";
}

struct Access : juce::AudioProcessor { using juce::AudioProcessor::copyXmlToBinary; };

// Exactly what the old getStateInformation produced: an XmlElement with the legacy tag
// and one attribute per parameter, through copyXmlToBinary.
juce::MemoryBlock legacyBlob (juce::AudioProcessor& p, const juce::XmlElement& xml)
{
    juce::MemoryBlock blob;
    static_cast<Access&> (p).copyXmlToBinary (xml, blob);
    return blob;
}

#if HP_CHECK_HARMONIC_EXCITER
juce::XmlElement makeLegacyXml()
{
    juce::XmlElement xml ("HarmonicExciter");           // old tag, HarmonicExciterProcessor.cpp
    xml.setAttribute ("drive", 72.5);
    xml.setAttribute ("frequency", 8000.0);
    xml.setAttribute ("harmonics", 3.5);
    xml.setAttribute ("mix", 25.0);
    xml.setAttribute ("type", 1);                         // index: Bright
    return xml;
}
void assertRestored (HP_CHECK_PROCESSOR_CLASS& p)
{
    expectNear ("drive",     p.driveParam->get(),     72.5f);
    expectNear ("frequency", p.frequencyParam->get(), 8000.0f, 1.0f);
    expectNear ("harmonics", p.harmonicsParam->get(), 3.5f);
    expectNear ("mix",       p.mixParam->get(),       25.0f);
    expectNear ("type",      (float) p.typeParam->getIndex(), 1.0f);
}
#elif HP_CHECK_NOISE_GATE
juce::XmlElement makeLegacyXml()
{
    juce::XmlElement xml ("NoiseGateState");             // old tag, NoiseGateProcessor.cpp
    xml.setAttribute ("threshold", -33.3);
    xml.setAttribute ("attack", 12.5);
    xml.setAttribute ("hold", 250.0);
    xml.setAttribute ("release", 750.0);
    xml.setAttribute ("range", -12.0);
    xml.setAttribute ("lookahead", 4.5);
    return xml;
}
void assertRestored (HP_CHECK_PROCESSOR_CLASS& p)
{
    expectNear ("threshold", p.threshold->get(), -33.3f);
    expectNear ("attack",    p.attack->get(),    12.5f);
    expectNear ("hold",      p.hold->get(),      250.0f);
    expectNear ("release",   p.release->get(),   750.0f, 1.0f);
    expectNear ("range",     p.range->get(),     -12.0f);
    expectNear ("lookahead", p.lookahead->get(), 4.5f);
}
#else
#error "define HP_CHECK_HARMONIC_EXCITER or HP_CHECK_NOISE_GATE"
#endif
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    HP_CHECK_PROCESSOR_CLASS proc;
    proc.setPlayConfigDetails (2, 2, 48000.0, 512);

    auto blob = legacyBlob (proc, makeLegacyXml());
    proc.setStateInformation (blob.getData(), (int) blob.getSize());
    std::cout << "-- legacy blob restored\n";
    assertRestored (proc);

    juce::MemoryBlock fresh;
    proc.getStateInformation (fresh);
    HP_CHECK_PROCESSOR_CLASS proc2;
    proc2.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc2.setStateInformation (fresh.getData(), (int) fresh.getSize());
    std::cout << "-- new blob round-tripped into a fresh instance\n";
    assertRestored (proc2);

    std::unique_ptr<juce::XmlElement> xml (juce::AudioProcessor::getXmlFromBinary (fresh.getData(), (int) fresh.getSize()));
    if (xml == nullptr || ! xml->hasTagName (HP_CHECK_PROCESSOR_CLASS::stateType))
    {
        std::cerr << "FAIL new blob is not typed " << HP_CHECK_PROCESSOR_CLASS::stateType << "\n";
        ++failures;
    }

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " state migration check\n";
    return failures == 0 ? 0 : 1;
}
