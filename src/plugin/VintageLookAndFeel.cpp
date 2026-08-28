#include "VintageLookAndFeel.h"

namespace bb {

VintageLookAndFeel::VintageLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colours::panel);
    setColour (juce::Label::textColourId,                 colours::ink);
    setColour (juce::ComboBox::backgroundColourId,        colours::panelDark);
    setColour (juce::ComboBox::textColourId,              colours::screenGlow);
    setColour (juce::ComboBox::outlineColourId,           colours::bezel);
    setColour (juce::ComboBox::arrowColourId,             colours::screenGlow);
    setColour (juce::PopupMenu::backgroundColourId,       colours::panelDark);
    setColour (juce::PopupMenu::textColourId,             colours::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::ink);
    setColour (juce::PopupMenu::highlightedTextColourId,  colours::panel);
    setColour (juce::Slider::textBoxTextColourId,         colours::ink);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId,      juce::Colours::transparentBlack);
    setColour (juce::TextEditor::outlineColourId,         juce::Colours::transparentBlack);
    setColour (juce::TextEditor::focusedOutlineColourId,  colours::indicator);
    setColour (juce::TextEditor::textColourId,            colours::screenGlow);
    setColour (juce::TextEditor::highlightColourId,       colours::indicator.withAlpha (0.30f));
    setColour (juce::Label::backgroundColourId,           juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxTextColourId,         colours::screenGlow);
    setColour (juce::TextButton::buttonColourId,          colours::panelDark);
    setColour (juce::ScrollBar::thumbColourId,            colours::inkFaded);
    setColour (juce::ScrollBar::trackColourId,            colours::panelDark);
    setColour (juce::ScrollBar::backgroundColourId,       colours::panelDark);
    setColour (juce::TabbedComponent::backgroundColourId, colours::panel);
    setColour (juce::TabbedButtonBar::tabTextColourId,    colours::inkFaded);
    setColour (juce::TabbedButtonBar::frontTextColourId,  colours::ink);
}

void VintageLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (3, 1, box.getWidth() - 14, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}

juce::Font VintageLookAndFeel::getLabelFont (juce::Label& label)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultSerifFontName(),
                                          (float) label.getHeight() * 0.82f, juce::Font::plain));
}

juce::Font VintageLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultSerifFontName(),
                                          juce::jmin (14.0f, (float) box.getHeight() * 0.66f),
                                          juce::Font::plain));
}

void VintageLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre  = bounds.getCentre();
    const float angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Tick marks around the dial, in the style of a screen-printed scale.
    g.setColour (colours::inkFaded.withAlpha (0.55f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = rotaryStartAngle + (float) i / 10.0f * (rotaryEndAngle - rotaryStartAngle);
        const float inner = radius * 0.92f, outer = radius * (i % 5 == 0 ? 1.0f : 0.97f);
        const juce::Line<float> tick (centre.getPointOnCircumference (inner, a),
                                      centre.getPointOnCircumference (outer, a));
        g.drawLine (tick, i % 5 == 0 ? 1.4f : 0.8f);
    }

    // Value arc - the molten flow around the dial. On a dark panel the pointer
    // alone is hard to read; the arc gives the value a visible extent.
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius * 0.90f, radius * 0.90f,
                           0.0f, rotaryStartAngle, angle, true);

        juce::ColourGradient flow (colours::moltenCool,
                                   centre.getPointOnCircumference (radius, rotaryStartAngle),
                                   colours::moltenHot,
                                   centre.getPointOnCircumference (radius, angle), false);
        g.setGradientFill (flow);
        g.strokePath (arc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    const float knobR = radius * 0.80f;
    const auto knob = juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (centre);

    // Drop shadow under the cap.
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillEllipse (knob.translated (0.0f, 1.5f));

    // Brushed metal cap: a diagonal gradient reads as a machined surface.
    juce::ColourGradient metal (colours::metalHi, knob.getTopLeft(),
                                colours::metalLo, knob.getBottomRight(), false);
    metal.addColour (0.45, colours::metalHi.interpolatedWith (colours::metalLo, 0.35f));
    g.setGradientFill (metal);
    g.fillEllipse (knob);

    // Bezel ring.
    g.setColour (colours::bezel.withAlpha (0.85f));
    g.drawEllipse (knob.reduced (0.5f), 1.6f);

    // Inner bevel highlight along the top edge.
    g.setColour (colours::metalHi.withAlpha (0.7f));
    g.drawEllipse (knob.reduced (2.5f).translated (0.0f, 0.6f), 1.0f);

    // Pointer.
    juce::Path pointer;
    const float pw = juce::jmax (2.0f, knobR * 0.13f);
    pointer.addRoundedRectangle (-pw * 0.5f, -knobR * 0.92f, pw, knobR * 0.55f, pw * 0.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));

    if (slider.isEnabled())
    {
        g.setColour (colours::indicator.withAlpha (0.28f));
        g.strokePath (pointer, juce::PathStrokeType (3.0f));
    }

    g.setColour (slider.isEnabled() ? colours::indicator : colours::inkFaded);
    g.fillPath (pointer);

    // Centre screw slot - the small detail that sells the hardware look.
    g.setColour (colours::metalLo.withAlpha (0.6f));
    g.fillEllipse (juce::Rectangle<float> (knobR * 0.22f, knobR * 0.22f).withCentre (centre));
}

void VintageLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                       int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);

    // Recessed window, like a lens over a printed strip.
    g.setColour (colours::screen);
    g.fillRoundedRectangle (r, 3.0f);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 3.0f, 1.2f);

    g.setColour (colours::metalHi.withAlpha (0.25f));
    g.drawLine (r.getX() + 3.0f, r.getBottom() - 1.0f, r.getRight() - 3.0f, r.getBottom() - 1.0f, 1.0f);

    // Arrow.
    juce::Path arrow;
    const float cx = r.getRight() - 12.0f, cy = r.getCentreY();
    arrow.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour (colours::screenGlow.withAlpha (box.isEnabled() ? 0.8f : 0.3f));
    g.fillPath (arrow);
}

void VintageLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float, float,
                                           juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                          0.0f, 0.0f, style, slider);
        return;
    }

    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const float cy    = bounds.getCentreY();
    const float trackH = 5.0f;
    const auto track  = juce::Rectangle<float> (bounds.getX(), cy - trackH * 0.5f,
                                                bounds.getWidth(), trackH);

    // Recessed channel.
    g.setColour (colours::screen);
    g.fillRoundedRectangle (track, trackH * 0.5f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawRoundedRectangle (track, trackH * 0.5f, 1.0f);

    const double lo = slider.getMinimum(), hi = slider.getMaximum();
    const bool bipolar = lo < 0.0 && hi > 0.0;

    const float originX = bipolar
        ? (float) juce::jmap (0.0, lo, hi, (double) bounds.getX(), (double) bounds.getRight())
        : bounds.getX();

    // Fill between the origin and the thumb.
    const auto fill = juce::Rectangle<float> (juce::jmin (originX, sliderPos), track.getY(),
                                              std::abs (sliderPos - originX), trackH);
    g.setColour (colours::indicator.withAlpha (0.85f));
    g.fillRoundedRectangle (fill, trackH * 0.5f);

    if (bipolar)   // centre detent tick
    {
        g.setColour (colours::inkFaded.withAlpha (0.6f));
        g.drawLine (originX, cy - 7.0f, originX, cy + 7.0f, 1.0f);
    }

    // Thumb, matching the rotary caps.
    const float r = 7.0f;
    const auto thumb = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre ({ sliderPos, cy });

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillEllipse (thumb.translated (0.0f, 1.0f));

    juce::ColourGradient metal (colours::metalHi, thumb.getTopLeft(),
                                colours::metalLo, thumb.getBottomRight(), false);
    g.setGradientFill (metal);
    g.fillEllipse (thumb);
    g.setColour (colours::bezel.withAlpha (0.8f));
    g.drawEllipse (thumb.reduced (0.5f), 1.2f);
}

void VintageLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                           bool isHighlighted, bool isDown)
{
    const auto r = button.getLocalBounds().toFloat();
    const float lampSize = juce::jmin (12.0f, r.getHeight() - 2.0f);
    const auto lamp = juce::Rectangle<float> (lampSize, lampSize)
                          .withCentre ({ r.getX() + lampSize * 0.5f + 2.0f, r.getCentreY() });

    const bool on = button.getToggleState();

    // Bezel.
    g.setColour (colours::bezel);
    g.fillEllipse (lamp.expanded (1.5f));

    // Lens - lit lamps get a soft halo so the state reads at a glance.
    g.setColour (on ? colours::indicator : colours::indicator.withAlpha (0.18f));
    g.fillEllipse (lamp);

    if (on)
    {
        g.setColour (colours::indicator.withAlpha (0.30f));
        g.fillEllipse (lamp.expanded (3.0f));
    }

    g.setColour (colours::metalHi.withAlpha (on ? 0.55f : 0.25f));
    g.fillEllipse (lamp.reduced (lampSize * 0.30f).translated (-lampSize * 0.12f, -lampSize * 0.12f));

    if (isHighlighted || isDown)
    {
        g.setColour (colours::ink.withAlpha (0.12f));
        g.fillRoundedRectangle (r, 2.0f);
    }

    g.setColour (colours::ink);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSerifFontName(), 13.0f, juce::Font::plain)));
    g.drawText (button.getButtonText(),
                r.withTrimmedLeft (lampSize + 6.0f), juce::Justification::centredLeft, false);
}

void VintageLookAndFeel::drawEngravedPanel (juce::Graphics& g, juce::Rectangle<float> r,
                                            const juce::String& title)
{
    // Slightly darker inset plate with an engraved edge.
    g.setColour (colours::panelDark.withAlpha (0.55f));
    g.fillRoundedRectangle (r, 4.0f);

    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);

    g.setColour (colours::ember.withAlpha (0.28f));
    g.drawRoundedRectangle (r.reduced (0.5f).translated (0.0f, 1.0f), 4.0f, 0.8f);

    if (title.isNotEmpty())
    {
        auto header = r.removeFromTop (18.0f).reduced (8.0f, 2.0f);
        g.setColour (colours::ink.withAlpha (0.75f));
        g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSerifFontName(),
                                                  12.0f, juce::Font::bold)));
        // Letter-spaced caps, the way panel legends are actually printed.
        juce::String spaced;
        for (auto c : title) { spaced << c << ' '; }
        g.drawText (spaced.trim(), header, juce::Justification::centredLeft, false);
    }
}

} // namespace bb
