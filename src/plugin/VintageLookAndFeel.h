#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace bb {

// Palette for the front panel. Kept as named constants rather than scattered
// literals so the whole unit can be re-skinned from one place.
namespace colours {
    const juce::Colour panel      { 0xffe6ddc8 };  // aged cream silkscreen
    const juce::Colour panelDark  { 0xffd4c9b0 };
    const juce::Colour metalHi    { 0xfff2efe9 };
    const juce::Colour metalLo    { 0xff8f8b82 };
    const juce::Colour ink        { 0xff2b2721 };  // screen-printed lettering
    const juce::Colour inkFaded   { 0xff6b6355 };
    const juce::Colour indicator  { 0xffb03a20 };
    const juce::Colour bezel      { 0xff1a1714 };
    const juce::Colour screen     { 0xff20281f };
    const juce::Colour screenGlow { 0xff8fd070 };
}

class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    // Bipolar controls (the mod matrix depths) need to fill outward from the
    // centre detent, not from the left end.
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    // JUCE reserves 30px for the arrow by default, which truncates text in a
    // narrow panel cell; the arrow here only needs about half that.
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;

    // Shared panel chrome, used by the editor for its section boxes.
    static void drawEngravedPanel (juce::Graphics&, juce::Rectangle<float>, const juce::String& title);
};

} // namespace bb
