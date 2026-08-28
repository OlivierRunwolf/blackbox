#pragma once
#include "Oscillator.h"
#include "Filter.h"
#include "Lfo.h"

namespace bb {

constexpr int kMaxVoices      = 16;
constexpr int kMaxUnison      = 8;
constexpr int kNumEnvelopes   = 2;
constexpr int kNumLfos        = 2;

struct EnvParams { float attackMs = 5.0f, decayMs = 200.0f, sustain = 0.7f, releaseMs = 300.0f; };

struct OscParams
{
    Waveform wave      = Waveform::Saw;
    float    level     = 1.0f;     // 0..1
    float    semitones = 0.0f;     // coarse, -24..24
    float    fine      = 0.0f;     // cents, -100..100
    float    pulseWidth = 0.5f;
};

struct FilterParams
{
    FilterMode mode      = FilterMode::LowPass;
    float      cutoffHz  = 8000.0f;
    float      resonance = 0.15f;
    float      drive     = 1.0f;
    float      envAmount = 0.0f;   // -1..1, Env2 -> cutoff, in octaves * 6
    float      keyTrack  = 0.0f;   // 0..1
};

struct UnisonParams
{
    int   count  = 1;      // 1..kMaxUnison
    float detune = 0.15f;  // 0..1 -> up to ~50 cents spread
    float spread = 0.5f;   // 0..1 stereo width
};

struct FxParams
{
    float chorusRate = 0.6f, chorusDepth = 0.0f, chorusMix = 0.0f;
    float delayTimeMs = 375.0f, delayFeedback = 0.35f, delayMix = 0.0f;
    float reverbSize = 0.6f, reverbDamp = 0.4f, reverbMix = 0.0f;
};

// The complete synth state. Deliberately plain data with no JUCE types, so the
// engine can be compiled and rendered offline for testing.
struct SynthParams
{
    OscParams    osc1, osc2;
    float        subLevel   = 0.0f;
    float        noiseLevel = 0.0f;
    bool         osc2Sync   = false;

    FilterParams filter;
    EnvParams    ampEnv;
    EnvParams    modEnv;      // Env2

    // Each LFO has one fixed destination - LFO 1 vibrato, LFO 2 filter sweep -
    // so depth is all that needs routing.
    struct LfoParams
    {
        LfoShape shape = LfoShape::Sine;
        float    rateHz = 2.0f;
        float    depth = 0.0f;      // 0..1
        bool     retrigger = true;
    };
    LfoParams    lfo[kNumLfos];

    UnisonParams unison;
    FxParams     fx;

    int   maxVoices  = 16;
    float glideMs    = 0.0f;
    float masterGain = 0.7f;   // linear
    float bendRange  = 2.0f;   // semitones
};

} // namespace bb
