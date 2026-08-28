#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../dsp/Params.h"

namespace bb {

// Parameter IDs live in one place because preset files and host automation both
// key off them - once shipped, an ID is permanent.
namespace pid {
    inline constexpr const char* osc1Wave  = "osc1_wave";
    inline constexpr const char* osc1Level = "osc1_level";
    inline constexpr const char* osc1Semi  = "osc1_semi";
    inline constexpr const char* osc1Fine  = "osc1_fine";
    inline constexpr const char* osc1Pw    = "osc1_pw";

    inline constexpr const char* osc2Wave  = "osc2_wave";
    inline constexpr const char* osc2Level = "osc2_level";
    inline constexpr const char* osc2Semi  = "osc2_semi";
    inline constexpr const char* osc2Fine  = "osc2_fine";
    inline constexpr const char* osc2Pw    = "osc2_pw";
    inline constexpr const char* osc2Sync  = "osc2_sync";

    inline constexpr const char* subLevel   = "sub_level";
    inline constexpr const char* noiseLevel = "noise_level";

    inline constexpr const char* filtMode  = "filt_mode";
    inline constexpr const char* filtCut   = "filt_cutoff";
    inline constexpr const char* filtRes   = "filt_res";
    inline constexpr const char* filtDrive = "filt_drive";
    inline constexpr const char* filtEnv   = "filt_env";
    inline constexpr const char* filtKey   = "filt_keytrack";

    inline constexpr const char* ampA = "amp_a";
    inline constexpr const char* ampD = "amp_d";
    inline constexpr const char* ampS = "amp_s";
    inline constexpr const char* ampR = "amp_r";

    inline constexpr const char* modA = "mod_a";
    inline constexpr const char* modD = "mod_d";
    inline constexpr const char* modS = "mod_s";
    inline constexpr const char* modR = "mod_r";

    inline constexpr const char* lfo1Shape = "lfo1_shape";
    inline constexpr const char* lfo1Rate  = "lfo1_rate";
    inline constexpr const char* lfo1Depth  = "lfo1_depth";
    inline constexpr const char* lfo1Retrig = "lfo1_retrig";
    inline constexpr const char* lfo2Shape = "lfo2_shape";
    inline constexpr const char* lfo2Rate  = "lfo2_rate";
    inline constexpr const char* lfo2Depth  = "lfo2_depth";
    inline constexpr const char* lfo2Retrig = "lfo2_retrig";

    inline constexpr const char* uniCount  = "uni_count";
    inline constexpr const char* uniDetune = "uni_detune";
    inline constexpr const char* uniSpread = "uni_spread";

    inline constexpr const char* chorusRate  = "fx_chorus_rate";
    inline constexpr const char* chorusDepth = "fx_chorus_depth";
    inline constexpr const char* chorusMix   = "fx_chorus_mix";
    inline constexpr const char* delayTime   = "fx_delay_time";
    inline constexpr const char* delayFb     = "fx_delay_fb";
    inline constexpr const char* delayMix    = "fx_delay_mix";
    inline constexpr const char* reverbSize  = "fx_reverb_size";
    inline constexpr const char* reverbDamp  = "fx_reverb_damp";
    inline constexpr const char* reverbMix   = "fx_reverb_mix";

    inline constexpr const char* voices = "voices";
    inline constexpr const char* glide  = "glide";
    inline constexpr const char* master = "master";
    inline constexpr const char* bend   = "bend_range";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Copies the current APVTS values into the plain DSP parameter struct. Called
// once per block - cheap, and it keeps the engine ignorant of JUCE.
void applyParameters (const juce::AudioProcessorValueTreeState& state, SynthParams& params);

} // namespace bb
