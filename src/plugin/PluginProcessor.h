#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterLayout.h"
#include "../dsp/SynthEngine.h"

class BlackboxProcessor : public juce::AudioProcessor
{
public:
    BlackboxProcessor();
    ~BlackboxProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BLACKBOX"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState& state() { return apvts; }

    // Read by the editor's voice indicator. Atomic because the editor polls it
    // from the message thread while the audio thread writes it.
    int activeVoices() const { return voiceCount.load(); }

private:
    void renderSegment (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    juce::AudioProcessorValueTreeState apvts;
    bb::SynthEngine synth;
    std::atomic<int> voiceCount { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlackboxProcessor)
};
