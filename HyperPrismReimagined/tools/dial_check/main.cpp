// hp_dial_<Effect>: every knob or slider that controls a parameter has the ZQ SFX house control
// behaviour (zqsfx_ui 0.5.0, components/Dial.h). Constructs the real editor headlessly and walks
// its component tree. A juce::Slider counts as bound to a parameter when moving it changes that
// parameter: this covers APVTS SliderAttachments and editors that drive a parameter pointer from
// onValueChange (Noise Gate) alike, and skips sliders that control nothing (a meter, a scrollbar).
// Every bound slider must:
//   - be a zqsfx::ui::Dial (Tab focus, focus ring, arrow and Shift+arrow steps);
//   - want keyboard focus and draw the focus outline of the house LookAndFeel (the ring);
//   - have double-click return enabled, returning to that parameter's own default.
// Also prints the editor's Tab order (the KeyboardFocusTraverser's order) for review.
// Built per plugin by add_hyperprism_light_check(dial_check ...) in CMakeLists.txt. Opens an
// editor, so Linux CI skips it with the label visibility check. Exit 0 on pass.

#include HP_CHECK_PROCESSOR_HEADER
#include <zqsfx_ui/zqsfx_ui.h>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <typeinfo>
#include <vector>

namespace
{
int failures = 0;

void fail (const juce::String& msg)
{
    std::cerr << "FAIL " << msg << "\n";
    ++failures;
}

void collectSliders (juce::Component& parent, std::vector<juce::Slider*>& out)
{
    for (auto* child : parent.getChildren())
    {
        if (auto* slider = dynamic_cast<juce::Slider*> (child))
            out.push_back (slider);
        collectSliders (*child, out);
    }
}

juce::String describe (const juce::Component& c)
{
    if (c.getTitle().isNotEmpty())
        return c.getTitle();
    if (auto* owner = dynamic_cast<const juce::Slider*> (c.getParentComponent()))
        return owner->getTitle() + " readout"; // the slider's editable value text box
    if (auto* button = dynamic_cast<const juce::Button*> (&c))
        if (button->getButtonText().isNotEmpty())
            return "button \"" + button->getButtonText() + "\"";
    return c.getName().isNotEmpty() ? c.getName() : juce::String (typeid (c).name());
}

// The parameter(s) that move when this slider moves, as indices into getParameters().
std::vector<int> boundParameters (juce::AudioProcessor& proc, juce::Slider& slider)
{
    const auto& params = proc.getParameters();
    std::vector<float> before;
    for (auto* p : params)
        before.push_back (p->getValue());

    const double original = slider.getValue();
    const double target = juce::approximatelyEqual (original, slider.getMaximum()) ? slider.getMinimum()
                                                                                   : slider.getMaximum();
    slider.setValue (target, juce::sendNotificationSync);

    std::vector<int> moved;
    for (int i = 0; i < params.size(); ++i)
        if (std::abs (params[i]->getValue() - before[(size_t) i]) > 1.0e-6f)
            moved.push_back (i);

    slider.setValue (original, juce::sendNotificationSync);
    return moved;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;

    // Processor declared before the editor, so the editor is destroyed first.
    HP_CHECK_PROCESSOR_CLASS proc;
    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    if (editor == nullptr)
    {
        std::cerr << "FAIL createEditor returned null\n";
        return 1;
    }

    std::vector<juce::Slider*> sliders;
    collectSliders (*editor, sliders);

    int bound = 0;
    for (auto* slider : sliders)
    {
        const auto name = describe (*slider);
        const auto moved = boundParameters (proc, *slider);
        if (moved.empty())
        {
            std::cout << "skip  " << name << " (moves no parameter)\n";
            continue;
        }
        ++bound;

        if (moved.size() > 1)
        {
            fail (name + " moves " + juce::String ((int) moved.size()) + " parameters");
            continue;
        }

        auto* param = dynamic_cast<juce::RangedAudioParameter*> (proc.getParameters()[moved.front()]);
        if (param == nullptr)
        {
            fail (name + " is bound to a parameter that is not a RangedAudioParameter");
            continue;
        }

        const double expected = param->convertFrom0to1 (param->getDefaultValue());
        const double tolerance = 1.0e-6 * juce::jmax (1.0, slider->getRange().getLength());
        std::cout << "check " << name << " -> " << param->getParameterID()
                  << " default " << expected << "\n";

        if (dynamic_cast<zqsfx::ui::Dial*> (slider) == nullptr)
            fail (name + " (" + param->getParameterID() + ") is not a zqsfx::ui::Dial");
        if (! slider->getWantsKeyboardFocus())
            fail (name + " (" + param->getParameterID() + ") does not want keyboard focus");
        if (! slider->hasFocusOutline())
            fail (name + " (" + param->getParameterID() + ") draws no focus outline");
        // The ring itself comes from zqsfx::ui::LookAndFeel::createFocusOutlineForComponent,
        // which HyperPrismLookAndFeel inherits; a slider on any other LookAndFeel gets JUCE's.
        if (dynamic_cast<zqsfx::ui::LookAndFeel*> (&slider->getLookAndFeel()) == nullptr)
            fail (name + " (" + param->getParameterID() + ") is not drawn by the house LookAndFeel");
        if (! slider->isDoubleClickReturnEnabled())
            fail (name + " (" + param->getParameterID() + ") has double-click return disabled");
        else if (std::abs (slider->getDoubleClickReturnValue() - expected) > tolerance)
            fail (name + " (" + param->getParameterID() + ") double-click returns "
                  + juce::String (slider->getDoubleClickReturnValue()) + ", default is "
                  + juce::String (expected));
    }

    if (bound == 0)
        fail ("no slider in the editor is bound to a parameter");

    std::cout << "Tab order:";
    if (auto traverser = editor->createKeyboardFocusTraverser())
        for (auto* c : traverser->getAllComponents (editor.get()))
            std::cout << " | " << describe (*c);
    std::cout << "\n";

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " dial check: " << bound << " bound sliders, "
              << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
