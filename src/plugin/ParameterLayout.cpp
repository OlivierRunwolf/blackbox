#include "ParameterLayout.h"

namespace bb {

namespace {

using APF  = juce::AudioParameterFloat;
using APC  = juce::AudioParameterChoice;
using APB  = juce::AudioParameterBool;
using APIn = juce::AudioParameterInt;

// Version hint 1 across the board - bump only for a parameter whose meaning
// changes, so hosts can migrate old sessions.
juce::ParameterID id (const juce::String& s) { return { s, 1 }; }

// Formats a value the way a hardware panel would print it, rather than as a
// raw float. The unit is taken from the suffix where there is one; otherwise a
// range that sits inside 0..1 (or -1..1) is shown as a percentage, which covers
// every level, mix and depth control without annotating each call site.
juce::String formatValue (float v, const juce::String& suffix,
                          const juce::NormalisableRange<float>& range)
{
    if (suffix == "Hz")
        return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz"
                            : juce::String (v, v < 10.0f ? 2 : 0) + " Hz";

    if (suffix == "ms")
        return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s"
                            : juce::String (v, v < 10.0f ? 1 : 0) + " ms";

    if (suffix == "cents")
        return (v > 0.0f ? "+" : "") + juce::String (v, 0) + " ct";

    const bool unipolar = range.start >= 0.0f  && range.end <= 1.0f;
    const bool bipolar  = range.start >= -1.0f && range.end <= 1.0f;

    if (unipolar)
        return juce::String (juce::roundToInt (v * 100.0f)) + "%";

    if (bipolar)
        return (v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v * 100.0f)) + "%";

    return juce::String (v, 2);
}

std::unique_ptr<APF> makeFloat (const juce::String& pidStr, const juce::String& name,
                                juce::NormalisableRange<float> range, float def,
                                const juce::String& suffix = {})
{
    auto toText = [suffix, range] (float v, int) { return formatValue (v, suffix, range); };

    return std::make_unique<APF> (id (pidStr), name, range, def,
                                  juce::AudioParameterFloatAttributes()
                                      .withStringFromValueFunction (toText));
}

// Skewed so the useful part of the range sits in the middle of the knob - a
// linear cutoff knob spends most of its travel above 10 kHz where nothing
// interesting happens.
juce::NormalisableRange<float> freqRange (float lo, float hi)
{
    juce::NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (std::sqrt (lo * hi));
    return r;
}

// Modulation depth is most useful at the bottom of its travel - a few percent
// of vibrato is musical, 100% is an effect - so bias the knob accordingly.
juce::NormalisableRange<float> depthRange()
{
    juce::NormalisableRange<float> r (0.0f, 1.0f);
    r.setSkewForCentre (0.18f);
    return r;
}

juce::NormalisableRange<float> timeRange (float lo, float hi, float centre)
{
    juce::NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (centre);
    return r;
}

const juce::StringArray waveNames   { "Saw", "Sqr", "Tri", "Sine", "Noise" };
const juce::StringArray filterNames { "LP", "BP", "HP", "Notch" };
const juce::StringArray lfoNames    { "Sine", "Tri", "Saw", "Sqr", "S&H" };

void addOscillator (juce::AudioProcessorValueTreeState::ParameterLayout& l,
                    const char* wave, const char* level, const char* semi,
                    const char* fine, const char* pw, const juce::String& prefix,
                    int defaultWave, float defaultLevel)
{
    l.add (std::make_unique<APC> (id (wave),  prefix + " Wave", waveNames, defaultWave));
    l.add (makeFloat (level, prefix + " Level", { 0.0f, 1.0f }, defaultLevel));
    l.add (std::make_unique<APIn> (id (semi), prefix + " Semitones", -24, 24, 0));
    l.add (makeFloat (fine,  prefix + " Fine",  { -100.0f, 100.0f }, 0.0f, "cents"));
    l.add (makeFloat (pw,    prefix + " Pulse Width", { 0.02f, 0.98f }, 0.5f));
}

} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    addOscillator (l, pid::osc1Wave, pid::osc1Level, pid::osc1Semi, pid::osc1Fine, pid::osc1Pw,
                   "Osc 1", 0, 0.8f);
    addOscillator (l, pid::osc2Wave, pid::osc2Level, pid::osc2Semi, pid::osc2Fine, pid::osc2Pw,
                   "Osc 2", 1, 0.0f);
    l.add (std::make_unique<APB> (id (pid::osc2Sync), "Osc 2 Sync", false));

    l.add (makeFloat (pid::subLevel,   "Sub Level",   { 0.0f, 1.0f }, 0.0f));
    l.add (makeFloat (pid::noiseLevel, "Noise Level", { 0.0f, 1.0f }, 0.0f));

    l.add (std::make_unique<APC> (id (pid::filtMode), "Filter Mode", filterNames, 0));
    l.add (makeFloat (pid::filtCut,   "Cutoff",     freqRange (20.0f, 20000.0f), 12000.0f, "Hz"));
    l.add (makeFloat (pid::filtRes,   "Resonance",  { 0.0f, 1.0f }, 0.15f));
    l.add (makeFloat (pid::filtDrive, "Drive",      { 1.0f, 10.0f }, 1.0f));
    l.add (makeFloat (pid::filtEnv,   "Env Amount", { -1.0f, 1.0f }, 0.0f));
    l.add (makeFloat (pid::filtKey,   "Key Track",  { 0.0f, 1.0f }, 0.0f));

    l.add (makeFloat (pid::ampA, "Amp Attack",  timeRange (0.1f, 5000.0f, 50.0f), 5.0f,   "ms"));
    l.add (makeFloat (pid::ampD, "Amp Decay",   timeRange (1.0f, 8000.0f, 300.0f), 300.0f, "ms"));
    l.add (makeFloat (pid::ampS, "Amp Sustain", { 0.0f, 1.0f }, 0.7f));
    l.add (makeFloat (pid::ampR, "Amp Release", timeRange (1.0f, 10000.0f, 400.0f), 300.0f, "ms"));

    l.add (makeFloat (pid::modA, "Mod Attack",  timeRange (0.1f, 5000.0f, 50.0f), 2.0f,   "ms"));
    l.add (makeFloat (pid::modD, "Mod Decay",   timeRange (1.0f, 8000.0f, 300.0f), 400.0f, "ms"));
    l.add (makeFloat (pid::modS, "Mod Sustain", { 0.0f, 1.0f }, 0.3f));
    l.add (makeFloat (pid::modR, "Mod Release", timeRange (1.0f, 10000.0f, 400.0f), 300.0f, "ms"));

    l.add (std::make_unique<APC> (id (pid::lfo1Shape), "LFO 1 Shape", lfoNames, 0));
    l.add (makeFloat (pid::lfo1Rate, "LFO 1 Rate", timeRange (0.01f, 50.0f, 3.0f), 4.0f, "Hz"));
    l.add (makeFloat (pid::lfo1Depth, "LFO 1 Vibrato Depth", depthRange(), 0.0f));
    l.add (std::make_unique<APB> (id (pid::lfo1Retrig), "LFO 1 Retrigger", true));

    l.add (std::make_unique<APC> (id (pid::lfo2Shape), "LFO 2 Shape", lfoNames, 1));
    l.add (makeFloat (pid::lfo2Rate, "LFO 2 Rate", timeRange (0.01f, 50.0f, 3.0f), 0.5f, "Hz"));
    l.add (makeFloat (pid::lfo2Depth, "LFO 2 Filter Sweep Depth", depthRange(), 0.0f));
    l.add (std::make_unique<APB> (id (pid::lfo2Retrig), "LFO 2 Retrigger", false));

    l.add (std::make_unique<APIn> (id (pid::uniCount), "Unison Voices", 1, kMaxUnison, 1));
    l.add (makeFloat (pid::uniDetune, "Unison Detune", { 0.0f, 1.0f }, 0.2f));
    l.add (makeFloat (pid::uniSpread, "Unison Spread", { 0.0f, 1.0f }, 0.5f));

    l.add (makeFloat (pid::chorusRate,  "Chorus Rate",  timeRange (0.01f, 10.0f, 1.0f), 0.6f, "Hz"));
    l.add (makeFloat (pid::chorusDepth, "Chorus Depth", { 0.0f, 1.0f }, 0.5f));
    l.add (makeFloat (pid::chorusMix,   "Chorus Mix",   { 0.0f, 1.0f }, 0.0f));
    l.add (makeFloat (pid::delayTime,   "Delay Time",   timeRange (1.0f, 2000.0f, 300.0f), 375.0f, "ms"));
    l.add (makeFloat (pid::delayFb,     "Delay Feedback", { 0.0f, 0.95f }, 0.35f));
    l.add (makeFloat (pid::delayMix,    "Delay Mix",    { 0.0f, 1.0f }, 0.0f));
    l.add (makeFloat (pid::reverbSize,  "Reverb Size",  { 0.0f, 1.0f }, 0.6f));
    l.add (makeFloat (pid::reverbDamp,  "Reverb Damp",  { 0.0f, 1.0f }, 0.4f));
    l.add (makeFloat (pid::reverbMix,   "Reverb Mix",   { 0.0f, 1.0f }, 0.0f));

    l.add (std::make_unique<APIn> (id (pid::voices), "Max Voices", 1, kMaxVoices, 16));
    l.add (makeFloat (pid::glide,  "Glide",  timeRange (0.0f, 2000.0f, 100.0f), 0.0f, "ms"));
    l.add (makeFloat (pid::master, "Master", { 0.0f, 1.0f }, 0.7f));
    l.add (std::make_unique<APIn> (id (pid::bend), "Bend Range", 0, 24, 2));

    return l;
}

namespace {
    inline float raw (const juce::AudioProcessorValueTreeState& s, const juce::String& p)
    {
        if (auto* a = s.getRawParameterValue (p)) return a->load();
        jassertfalse;   // an ID typo would otherwise silently read as zero
        return 0.0f;
    }
}

void applyParameters (const juce::AudioProcessorValueTreeState& s, SynthParams& p)
{
    p.osc1.wave       = (Waveform) (int) raw (s, pid::osc1Wave);
    p.osc1.level      = raw (s, pid::osc1Level);
    p.osc1.semitones  = raw (s, pid::osc1Semi);
    p.osc1.fine       = raw (s, pid::osc1Fine);
    p.osc1.pulseWidth = raw (s, pid::osc1Pw);

    p.osc2.wave       = (Waveform) (int) raw (s, pid::osc2Wave);
    p.osc2.level      = raw (s, pid::osc2Level);
    p.osc2.semitones  = raw (s, pid::osc2Semi);
    p.osc2.fine       = raw (s, pid::osc2Fine);
    p.osc2.pulseWidth = raw (s, pid::osc2Pw);
    p.osc2Sync        = raw (s, pid::osc2Sync) > 0.5f;

    p.subLevel   = raw (s, pid::subLevel);
    p.noiseLevel = raw (s, pid::noiseLevel);

    p.filter.mode      = (FilterMode) (int) raw (s, pid::filtMode);
    p.filter.cutoffHz  = raw (s, pid::filtCut);
    p.filter.resonance = raw (s, pid::filtRes);
    p.filter.drive     = raw (s, pid::filtDrive);
    p.filter.envAmount = raw (s, pid::filtEnv);
    p.filter.keyTrack  = raw (s, pid::filtKey);

    p.ampEnv = { raw (s, pid::ampA), raw (s, pid::ampD), raw (s, pid::ampS), raw (s, pid::ampR) };
    p.modEnv = { raw (s, pid::modA), raw (s, pid::modD), raw (s, pid::modS), raw (s, pid::modR) };

    p.lfo[0].shape     = (LfoShape) (int) raw (s, pid::lfo1Shape);
    p.lfo[0].rateHz    = raw (s, pid::lfo1Rate);
    p.lfo[0].depth     = raw (s, pid::lfo1Depth);
    p.lfo[0].retrigger = raw (s, pid::lfo1Retrig) > 0.5f;
    p.lfo[1].shape     = (LfoShape) (int) raw (s, pid::lfo2Shape);
    p.lfo[1].rateHz    = raw (s, pid::lfo2Rate);
    p.lfo[1].depth     = raw (s, pid::lfo2Depth);
    p.lfo[1].retrigger = raw (s, pid::lfo2Retrig) > 0.5f;

    p.unison.count  = (int) raw (s, pid::uniCount);
    p.unison.detune = raw (s, pid::uniDetune);
    p.unison.spread = raw (s, pid::uniSpread);

    p.fx.chorusRate    = raw (s, pid::chorusRate);
    p.fx.chorusDepth   = raw (s, pid::chorusDepth);
    p.fx.chorusMix     = raw (s, pid::chorusMix);
    p.fx.delayTimeMs   = raw (s, pid::delayTime);
    p.fx.delayFeedback = raw (s, pid::delayFb);
    p.fx.delayMix      = raw (s, pid::delayMix);
    p.fx.reverbSize    = raw (s, pid::reverbSize);
    p.fx.reverbDamp    = raw (s, pid::reverbDamp);
    p.fx.reverbMix     = raw (s, pid::reverbMix);

    p.maxVoices  = (int) raw (s, pid::voices);
    p.glideMs    = raw (s, pid::glide);
    p.masterGain = raw (s, pid::master);
    p.bendRange  = raw (s, pid::bend);
}

} // namespace bb
