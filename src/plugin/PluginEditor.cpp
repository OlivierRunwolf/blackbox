#include "PluginEditor.h"
#include "ParameterLayout.h"

namespace bb {

namespace {

// Populates a combo from the choices the parameter itself declares, so the box
// can never drift out of sync with the parameter definition.
void fillFromChoiceParameter (juce::ComboBox& box, APVTS& state, const juce::String& paramId)
{
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramId)))
        box.addItemList (choice->choices, 1);
    else
        jassertfalse;
}

void styleLegend (juce::Label& l, const juce::String& text)
{
    l.setText (text.toUpperCase(), juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centredTop);
    l.setColour (juce::Label::textColourId, colours::ink.withAlpha (0.85f));
    l.setInterceptsMouseClicks (false, false);
}

constexpr int kLegendHeight = 17;

} // namespace

// ---------------------------------------------------------------------------

LabeledKnob::LabeledKnob (APVTS& state, const juce::String& paramId, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 17);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
    // A slow-drag modifier matters on a 20 Hz - 20 kHz cutoff knob.
    slider.setVelocityBasedMode (false);
    addAndMakeVisible (slider);

    styleLegend (legend, text);
    addAndMakeVisible (legend);

    attachment = std::make_unique<APVTS::SliderAttachment> (state, paramId, slider);
}

void LabeledKnob::resized()
{
    // Keep every dial the same size regardless of how much height its panel
    // happens to have - otherwise the effects row draws knobs twice the size of
    // the oscillator row - and keep the legend tucked directly beneath it
    // rather than pinned to the bottom of a tall cell.
    auto r = getLocalBounds();
    const int d = juce::jmin (72, r.getWidth(), r.getHeight() - kLegendHeight);

    auto group = r.withSizeKeepingCentre (r.getWidth(), d + kLegendHeight);
    legend.setBounds (group.removeFromBottom (kLegendHeight));
    slider.setBounds (group.withSizeKeepingCentre (juce::jmax (d, group.getWidth() - 4), d));
}

LabeledCombo::LabeledCombo (APVTS& state, const juce::String& paramId, const juce::String& text)
{
    fillFromChoiceParameter (box, state, paramId);
    addAndMakeVisible (box);

    styleLegend (legend, text);
    addAndMakeVisible (legend);

    attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramId, box);
}

void LabeledCombo::resized()
{
    auto r = getLocalBounds();
    legend.setBounds (r.removeFromBottom (kLegendHeight));
    box.setBounds (r.withSizeKeepingCentre (r.getWidth() - 4, juce::jmin (24, r.getHeight())));
}

LabeledToggle::LabeledToggle (APVTS& state, const juce::String& paramId, const juce::String& text)
{
    button.setButtonText (text);
    addAndMakeVisible (button);
    attachment = std::make_unique<APVTS::ButtonAttachment> (state, paramId, button);
}

void LabeledToggle::resized()
{
    button.setBounds (getLocalBounds().withSizeKeepingCentre (getWidth(), 22));
}

// ---------------------------------------------------------------------------

Panel::Panel (juce::String panelTitle, int columnCount)
    : title (std::move (panelTitle)), columns (juce::jmax (1, columnCount))
{
}

void Panel::addControl (juce::Component* c)
{
    controls.add (c);
    addAndMakeVisible (c);
}

void Panel::paint (juce::Graphics& g)
{
    VintageLookAndFeel::drawEngravedPanel (g, getLocalBounds().toFloat(), title);
}

void Panel::resized()
{
    auto area = getLocalBounds().reduced (6);
    area.removeFromTop (16);   // title strip

    if (controls.isEmpty())
        return;

    const int rows = (controls.size() + columns - 1) / columns;
    const int rowH = juce::jmax (1, area.getHeight() / rows);
    const int colW = juce::jmax (1, area.getWidth() / columns);

    for (int i = 0; i < controls.size(); ++i)
    {
        const int row = i / columns;
        const int col = i % columns;
        controls[i]->setBounds (area.getX() + col * colW, area.getY() + row * rowH, colW, rowH);
    }
}

// ---------------------------------------------------------------------------

} // namespace bb

// ---------------------------------------------------------------------------

using namespace bb;

BlackboxEditor::BlackboxEditor (BlackboxProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    auto& s = processor.state();

    oscA.addControl (new LabeledCombo (s, pid::osc1Wave,  "Wave"));
    oscA.addControl (new LabeledKnob  (s, pid::osc1Level, "Level"));
    oscA.addControl (new LabeledKnob  (s, pid::osc1Semi,  "Semi"));
    oscA.addControl (new LabeledKnob  (s, pid::osc1Fine,  "Fine"));
    oscA.addControl (new LabeledKnob  (s, pid::osc1Pw,    "Width"));

    oscB.addControl (new LabeledCombo (s, pid::osc2Wave,  "Wave"));
    oscB.addControl (new LabeledKnob  (s, pid::osc2Level, "Level"));
    oscB.addControl (new LabeledKnob  (s, pid::osc2Semi,  "Semi"));
    oscB.addControl (new LabeledKnob  (s, pid::osc2Fine,  "Fine"));
    oscB.addControl (new LabeledToggle (s, pid::osc2Sync, "Sync"));

    mixer.addControl (new LabeledKnob (s, pid::subLevel,   "Sub"));
    mixer.addControl (new LabeledKnob (s, pid::noiseLevel, "Noise"));

    filter.addControl (new LabeledCombo (s, pid::filtMode,  "Mode"));
    filter.addControl (new LabeledKnob  (s, pid::filtCut,   "Cutoff"));
    filter.addControl (new LabeledKnob  (s, pid::filtRes,   "Reso"));
    filter.addControl (new LabeledKnob  (s, pid::filtDrive, "Drive"));
    filter.addControl (new LabeledKnob  (s, pid::filtEnv,   "Env"));
    filter.addControl (new LabeledKnob  (s, pid::filtKey,   "Key"));

    ampEnv.addControl (new LabeledKnob (s, pid::ampA, "Attack"));
    ampEnv.addControl (new LabeledKnob (s, pid::ampD, "Decay"));
    ampEnv.addControl (new LabeledKnob (s, pid::ampS, "Sustain"));
    ampEnv.addControl (new LabeledKnob (s, pid::ampR, "Release"));

    modEnv.addControl (new LabeledKnob (s, pid::modA, "Attack"));
    modEnv.addControl (new LabeledKnob (s, pid::modD, "Decay"));
    modEnv.addControl (new LabeledKnob (s, pid::modS, "Sustain"));
    modEnv.addControl (new LabeledKnob (s, pid::modR, "Release"));

    unison.addControl (new LabeledKnob (s, pid::uniCount,  "Voices"));
    unison.addControl (new LabeledKnob (s, pid::uniDetune, "Detune"));
    unison.addControl (new LabeledKnob (s, pid::uniSpread, "Spread"));

    lfoA.addControl (new LabeledCombo  (s, pid::lfo1Shape,  "Shape"));
    lfoA.addControl (new LabeledKnob   (s, pid::lfo1Rate,   "Rate"));
    lfoA.addControl (new LabeledKnob   (s, pid::lfo1Depth,  "Vibrato"));
    lfoA.addControl (new LabeledToggle (s, pid::lfo1Retrig, "Retrig"));

    lfoB.addControl (new LabeledCombo  (s, pid::lfo2Shape,  "Shape"));
    lfoB.addControl (new LabeledKnob   (s, pid::lfo2Rate,   "Rate"));
    lfoB.addControl (new LabeledKnob   (s, pid::lfo2Depth,  "Sweep"));
    lfoB.addControl (new LabeledToggle (s, pid::lfo2Retrig, "Retrig"));

    global.addControl (new LabeledKnob (s, pid::voices, "Poly"));
    global.addControl (new LabeledKnob (s, pid::glide,  "Glide"));
    global.addControl (new LabeledKnob (s, pid::bend,   "Bend"));
    global.addControl (new LabeledKnob (s, pid::master, "Master"));

    // Each effect gets its own engraved plate inside the rack, so the three
    // units read as separate boxes rather than one row of nine knobs. The group
    // title carries the effect name, which lets the legends stay short.
    struct FxGroup { const char* title; const char* a; const char* b; const char* c;
                     const char* la; const char* lb; const char* lc; };

    const FxGroup groups[] = {
        { "CHORUS", pid::chorusRate, pid::chorusDepth, pid::chorusMix,  "Rate", "Depth",    "Mix" },
        { "DELAY",  pid::delayTime,  pid::delayFb,     pid::delayMix,   "Time", "Feedback", "Mix" },
        { "REVERB", pid::reverbSize, pid::reverbDamp,  pid::reverbMix,  "Size", "Damp",     "Mix" },
    };

    for (const auto& grp : groups)
    {
        auto* unit = new Panel (grp.title, 3);
        unit->addControl (new LabeledKnob (s, grp.a, grp.la));
        unit->addControl (new LabeledKnob (s, grp.b, grp.lb));
        unit->addControl (new LabeledKnob (s, grp.c, grp.lc));
        fx.addControl (unit);
    }

    for (auto* panel : { &oscA, &oscB, &mixer, &filter, &ampEnv, &modEnv,
                         &unison, &lfoA, &lfoB, &global, &fx })
        addAndMakeVisible (panel);

    setSize (1000, 700);
    startTimerHz (bb::BackgroundFilm::frameRateHz);
}

BlackboxEditor::~BlackboxEditor()
{
    setLookAndFeel (nullptr);
}

void BlackboxEditor::timerCallback()
{
    displayedVoices = processor.activeVoices();

    if (! film->isEmpty())
    {
        backdropFrame = (backdropFrame + 1) % film->numFrames();
        repaint();     // the backdrop moves, so the whole face is dirty anyway
    }
    else
    {
        repaint (getWidth() - 190, 8, 180, 48);
    }
}

void BlackboxEditor::drawNameplate (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (colours::bezel);
    g.fillRoundedRectangle (r, 3.0f);

    g.setColour (colours::ember.withAlpha (0.55f));
    g.drawRoundedRectangle (r.reduced (1.5f), 2.0f, 1.0f);

    // The lettering is the molten flow itself: hot at the top, cooling downward.
    juce::ColourGradient molten (colours::moltenHot, r.getCentreX(), r.getY() + 4.0f,
                                 colours::moltenCool, r.getCentreX(), r.getBottom() - 4.0f, false);
    molten.addColour (0.55, colours::indicator);
    g.setGradientFill (molten);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSerifFontName(),
                                              r.getHeight() * 0.46f, juce::Font::bold)));

    juce::String spaced;
    for (auto c : juce::String ("BLACKBOX")) { spaced << c << ' '; }
    g.drawText (spaced.trim(), r, juce::Justification::centred, false);
}

void BlackboxEditor::drawVoiceScreen (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (colours::bezel);
    g.fillRoundedRectangle (r.expanded (2.0f), 3.0f);

    g.setColour (colours::screen);
    g.fillRoundedRectangle (r, 2.0f);

    // A faint pool of heat behind the text, then the text itself.
    juce::ColourGradient heat (colours::indicator.withAlpha (0.22f), r.getCentre(),
                               juce::Colours::transparentBlack, r.getTopLeft(), true);
    g.setGradientFill (heat);
    g.fillRoundedRectangle (r, 2.0f);

    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                              r.getHeight() * 0.46f, juce::Font::plain)));
    g.setColour (colours::moltenHot);
    g.drawText ("VOICES " + juce::String (displayedVoices).paddedLeft ('0', 2),
                r, juce::Justification::centred, false);

    // Faint scanline wash, so it reads as a lit display rather than a label.
    g.setColour (juce::Colours::black.withAlpha (0.06f));
    for (float y = r.getY(); y < r.getBottom(); y += 3.0f)
        g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
}

void BlackboxEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::panel);

    if (! film->isEmpty())
    {
        g.drawImage (film->frame (backdropFrame), getLocalBounds().toFloat(),
                     juce::RectanglePlacement::fillDestination);

        // A backdrop has to stay a backdrop: without this the particle field
        // competes with the legends and the panel becomes hard to read.
        g.setColour (colours::panel.withAlpha (0.45f));
        g.fillAll();
    }

    auto header = getLocalBounds().removeFromTop (60).reduced (10, 8);
    drawVoiceScreen (g, header.removeFromRight (170).reduced (0, 6).toFloat());

    // Centred on the window, not on the space left over beside the readout, so
    // the plate sits on the panel's true centre line.
    const auto plate = juce::Rectangle<int> (250, header.getHeight())
                           .withCentre ({ getWidth() / 2, header.getCentreY() });
    drawNameplate (g, plate.toFloat());
}

void BlackboxEditor::resized()
{
    auto r = getLocalBounds().reduced (8);
    r.removeFromTop (56);   // header

    constexpr int gap = 6;

    auto row1 = r.removeFromTop (132);
    oscA .setBounds (row1.removeFromLeft (320).reduced (gap / 2));
    oscB .setBounds (row1.removeFromLeft (320).reduced (gap / 2));
    mixer.setBounds (row1.reduced (gap / 2));

    auto row2 = r.removeFromTop (132);
    filter.setBounds (row2.removeFromLeft (400).reduced (gap / 2));
    ampEnv.setBounds (row2.removeFromLeft (280).reduced (gap / 2));
    modEnv.setBounds (row2.reduced (gap / 2));

    auto row3 = r.removeFromTop (126);
    lfoA  .setBounds (row3.removeFromLeft (256).reduced (gap / 2));
    lfoB  .setBounds (row3.removeFromLeft (256).reduced (gap / 2));
    unison.setBounds (row3.removeFromLeft (190).reduced (gap / 2));
    global.setBounds (row3.reduced (gap / 2));

    fx.setBounds (r.reduced (gap / 2));
}
