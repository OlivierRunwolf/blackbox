#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace bb {

// Palette for the front panel. Kept as named constants rather than scattered
// literals so the whole unit can be re-skinned from one place.
namespace colours {
    // Lava palette: cooled basalt ground, molten core, purple heat at the edges.
    const juce::Colour panel      { 0xff0a0810 };  // near-black basalt
    const juce::Colour panelDark  { 0xff1b1526 };  // inset plate, still lifted off the ground
    const juce::Colour metalHi    { 0xff6b5a7a };  // lit stone edge
    const juce::Colour metalLo    { 0xff241c31 };  // stone in shadow
    const juce::Colour ink        { 0xfff5e3c0 };  // warm bone lettering
    const juce::Colour inkFaded   { 0xffa08ca8 };  // cooled purple-grey
    const juce::Colour indicator  { 0xffff9a24 };  // molten pointer
    const juce::Colour ember      { 0xffc0246a };  // magenta heat at the crust edge
    const juce::Colour bezel      { 0xff040309 };  // deepest shadow
    const juce::Colour screen     { 0xff130a0d };  // ember well
    const juce::Colour screenGlow { 0xffffb43a };  // glowing readout
    const juce::Colour moltenHot  { 0xffffd86b };  // brightest part of the flow
    const juce::Colour moltenCool { 0xffff6a18 };  // where the flow darkens
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
