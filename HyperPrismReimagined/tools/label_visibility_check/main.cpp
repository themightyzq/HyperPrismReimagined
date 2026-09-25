// hp_label_visibility_SonicDecimator: regression check for the Anti-Alias/Dither ParameterLabel
// visibility defect (SonicDecimatorEditor::setupToggleButton() never called addAndMakeVisible()
// on the label or gave it bounds in resized(), even though each label had a working onClick
// handler wired up in the constructor -- right-click-to-assign-to-XY-pad on those two toggles
// silently did nothing; see the FIX comment in SonicDecimatorEditor.cpp).
//
// Constructs the real editor (not a mock), finds the two labels by their text via the same
// public-API-only traversal a host or accessibility tool would have to use (they are private
// SonicDecimatorEditor members), and asserts each is visible with a non-empty bounds at least
// 22px tall (the house minimum hit target, ../../CLAUDE.md section 6) at both the default
// (700x550) and minimum (600x520, the setResizeLimits() floor) editor size. Also exercises the
// right-click assign path by invoking each label's onClick() handler directly: ParameterLabel::
// onClick is a public std::function, and showParameterMenu() (which it calls) posts via
// juce::PopupMenu::showMenuAsync() and returns immediately without pumping the message loop, so
// this is safe to call from a headless console app that never runs a dispatch loop.
//
// Before the fix, collectLabels() below would find these labels with isVisible() == false and
// an empty (default-constructed) bounds, since addAndMakeVisible() and setBounds() were never
// called on them. Exit 0 on pass.

#include "SonicDecimatorEditor.h"
#include <iostream>
#include <vector>

namespace
{
int failures = 0;

void fail(const juce::String& msg)
{
    std::cerr << "FAIL " << msg << "\n";
    ++failures;
}

// antiAliasLabel/ditherLabel are private SonicDecimatorEditor members, so this walks the
// component tree the way an external tool (or a screen reader) would have to.
void collectLabels(juce::Component& parent, const juce::String& text, std::vector<ParameterLabel*>& out)
{
    for (int i = 0; i < parent.getNumChildComponents(); ++i)
    {
        auto* child = parent.getChildComponent(i);
        if (child == nullptr)
            continue;
        if (auto* label = dynamic_cast<ParameterLabel*>(child))
            if (label->getText() == text)
                out.push_back(label);
        collectLabels(*child, text, out);
    }
}

void checkAtSize(SonicDecimatorProcessor& proc, int w, int h)
{
    std::unique_ptr<juce::AudioProcessorEditor> editor(proc.createEditor());
    if (editor == nullptr)
    {
        fail("createEditor returned null");
        return;
    }
    editor->setSize(w, h); // triggers resized(); within setResizeLimits(600, 520, 900, 750)

    for (const char* name : { "Anti-Alias", "Dither" })
    {
        std::vector<ParameterLabel*> found;
        collectLabels(*editor, name, found);

        if (found.empty())
        {
            fail(juce::String("no ParameterLabel found with text \"") + name + "\" at "
                 + juce::String(w) + "x" + juce::String(h));
            continue;
        }

        for (auto* label : found)
        {
            const auto bounds = label->getBounds();
            std::cout << name << " at " << w << "x" << h << ": visible="
                       << (label->isVisible() ? "true" : "false")
                       << " bounds=" << bounds.toString() << "\n";

            if (! label->isVisible())
                fail(juce::String(name) + " is not visible at " + juce::String(w) + "x" + juce::String(h));
            if (bounds.isEmpty())
                fail(juce::String(name) + " has empty bounds at " + juce::String(w) + "x" + juce::String(h));
            if (bounds.getHeight() < 22)
                fail(juce::String(name) + " hit target is " + juce::String(bounds.getHeight())
                     + "px tall, below the 22px house minimum, at " + juce::String(w) + "x" + juce::String(h));

            if (label->onClick)
            {
                label->onClick(); // exercise the right-click assign path -- see file header
                std::cout << name << " onClick (right-click assign) exercised successfully\n";
            }
            else
            {
                fail(juce::String(name) + " has no onClick handler bound");
            }
        }
    }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;

    SonicDecimatorProcessor proc;
    checkAtSize(proc, 700, 550); // default
    checkAtSize(proc, 600, 520); // minimum

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " label visibility check\n";
    return failures == 0 ? 0 : 1;
}
