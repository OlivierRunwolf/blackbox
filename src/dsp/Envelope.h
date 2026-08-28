#pragma once
#include "DspCommon.h"

namespace bb {

// Analog-style ADSR. The decay and release stages are exponential (they head
// toward a target below the one they stop at), which is what makes a plucked
// note sound plucked rather than linear-ramped.
class Envelope
{
public:
    enum class Stage { Idle = 0, Attack, Decay, Sustain, Release };

    void prepare (float sr) { sampleRate = sr; reset(); }

    void reset()
    {
        stage = Stage::Idle;
        level = 0.0f;
    }

    void setParameters (float attackMs, float decayMs, float sustainLevel, float releaseMs)
    {
        atkRate = rateFor (attackMs);
        decRate = rateFor (decayMs);
        sustain = clampf (sustainLevel, 0.0f, 1.0f);
        relRate = rateFor (releaseMs);
    }

    void noteOn (bool retrigger = true)
    {
        if (retrigger) level = 0.0f;
        stage = Stage::Attack;
    }

    void noteOff()
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    bool isActive() const { return stage != Stage::Idle; }

    float next()
    {
        switch (stage)
        {
            case Stage::Attack:
                // Overshoot target of 1.2 makes the approach curve convex,
                // so the attack reads as snappy instead of soft.
                level += atkRate * (1.2f - level);
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                break;

            case Stage::Decay:
                level += decRate * (sustain - 0.05f - level);
                if (level <= sustain) { level = sustain; stage = Stage::Sustain; }
                break;

            case Stage::Sustain:
                level = sustain;
                if (sustain <= 0.0f) stage = Stage::Idle;
                break;

            case Stage::Release:
                level += relRate * (-0.05f - level);
                if (level <= 0.0001f) { level = 0.0f; stage = Stage::Idle; }
                break;

            case Stage::Idle:
            default:
                level = 0.0f;
                break;
        }

        return level;
    }

    float value() const { return level; }
    Stage currentStage() const { return stage; }

private:
    float rateFor (float ms) const
    {
        const float samples = std::max (1.0f, 0.001f * ms * sampleRate);
        return 1.0f - std::exp (-1.0f / samples);
    }

    float sampleRate = 44100.0f;
    float atkRate = 0.01f, decRate = 0.001f, relRate = 0.001f;
    float sustain = 0.7f, level = 0.0f;
    Stage stage = Stage::Idle;
};

} // namespace bb
