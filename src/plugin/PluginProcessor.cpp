#include "PluginProcessor.h"
#include "PluginEditor.h"

BlackboxProcessor::BlackboxProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BLACKBOX", bb::createParameterLayout())
{
}

void BlackboxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.prepare ((float) sampleRate, samplesPerBlock);
    bb::applyParameters (apvts, synth.parameters());
}

bool BlackboxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo out only - the engine pans unison across a fixed stereo field, so
    // a mono layout would collapse the whole point of the spread control.
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void BlackboxProcessor::renderSegment (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (numSamples <= 0)
        return;

    synth.render (buffer.getWritePointer (0, startSample),
                  buffer.getWritePointer (1, startSample),
                  numSamples);
}

void BlackboxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();
    bb::applyParameters (apvts, synth.parameters());

    // Split the block at each MIDI event so note timing is sample-accurate
    // rather than quantised to the block size - audible on fast arpeggios.
    int position = 0;

    for (const auto meta : midi)
    {
        const int eventTime = juce::jlimit (0, buffer.getNumSamples(), meta.samplePosition);

        renderSegment (buffer, position, eventTime - position);
        position = eventTime;

        const auto m = meta.getMessage();

        if (m.isNoteOn())
            synth.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())
            synth.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            synth.allNotesOff();
        else if (m.isPitchWheel())
            synth.setPitchBend ((m.getPitchWheelValue() - 8192) / 8192.0f);
        else if (m.isController() && m.getControllerNumber() == 1)
            synth.setModWheel (m.getControllerValue() / 127.0f);
    }

    renderSegment (buffer, position, buffer.getNumSamples() - position);

    voiceCount.store (synth.activeVoiceCount());
}

juce::AudioProcessorEditor* BlackboxProcessor::createEditor()
{
    return new BlackboxEditor (*this);
}

void BlackboxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BlackboxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BlackboxProcessor();
}
