#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VintageLookAndFeel.h"
#include "BackgroundFilm.h"

namespace bb {

using APVTS = juce::AudioProcessorValueTreeState;

// A knob with its printed legend underneath. Bundling the attachment here means
// a control cannot outlive the parameter binding it depends on.
class LabeledKnob : public juce::Component
{
public:
    LabeledKnob (APVTS& state, const juce::String& paramId, const juce::String& text);
    void resized() override;

    juce::Slider slider;
    juce::Label  legend;

private:
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

class LabeledCombo : public juce::Component
{
public:
    LabeledCombo (APVTS& state, const juce::String& paramId, const juce::String& text);
    void resized() override;

    juce::ComboBox box;
    juce::Label    legend;

private:
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

class LabeledToggle : public juce::Component
{
public:
    LabeledToggle (APVTS& state, const juce::String& paramId, const juce::String& text);
    void resized() override;

    juce::ToggleButton button;

private:
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

// An inset plate that arranges its controls on a fixed-column grid.
class Panel : public juce::Component
{
public:
    Panel (juce::String panelTitle, int columnCount);

    void paint (juce::Graphics&) override;
    void resized() override;

    void addControl (juce::Component* c);   // takes ownership

private:
    juce::String title;
    int columns;
    juce::OwnedArray<juce::Component> controls;
};

} // namespace bb

class BlackboxEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit BlackboxEditor (BlackboxProcessor&);
    ~BlackboxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawNameplate (juce::Graphics&, juce::Rectangle<float>);
    void drawVoiceScreen (juce::Graphics&, juce::Rectangle<float>);

    BlackboxProcessor& processor;
    bb::VintageLookAndFeel lookAndFeel;

    bb::Panel oscA { "OSC 1", 5 }, oscB { "OSC 2", 5 }, mixer { "MIX", 2 };
    bb::Panel filter { "FILTER", 6 }, ampEnv { "AMP ENV", 4 }, modEnv { "MOD ENV", 4 };
    bb::Panel unison { "UNISON", 3 }, lfoA { "LFO 1", 4 }, lfoB { "LFO 2", 4 };
    bb::Panel global { "GLOBAL", 4 }, fx { "EFFECTS", 3 };

    // Shared across editor instances - decoding and recolouring the frames is
    // not something to repeat every time a window opens.
    juce::SharedResourcePointer<bb::BackgroundFilm> film;
    int backdropFrame = 0;

    int displayedVoices = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlackboxEditor)
};
