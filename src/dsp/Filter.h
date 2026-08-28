#pragma once
#include "DspCommon.h"

namespace bb {

enum class FilterMode { LowPass = 0, BandPass, HighPass, Notch, Count };

// Topology-preserving-transform state variable filter (Zavalishin). Chosen over
// a classic ladder because it stays stable when the cutoff is modulated fast,
// which matters once the mod matrix can slam it at audio rate.
class StateVariableFilter
{
public:
    void prepare (float sr)
    {
        sampleRate = sr;
        reset();
        setCutoff (1000.0f);
        setResonance (0.1f);
    }

    void reset() { ic1eq = ic2eq = 0.0f; }

    void setCutoff (float hz)
    {
        cutoff = clampf (hz, 20.0f, sampleRate * 0.45f);
        g = std::tan (kPi * cutoff / sampleRate);
        updateCoeffs();
    }

    // 0..1, where 1 is just short of self-oscillation.
    void setResonance (float r)
    {
        res = clampf (r, 0.0f, 1.0f);
        k = 2.0f - 1.98f * res;
        updateCoeffs();
    }

    void setMode (FilterMode m) { mode = m; }

    // Soft saturation in the feedback path is what stops high resonance from
    // blowing up and gives the filter its character when driven.
    void setDrive (float d) { drive = clampf (d, 1.0f, 10.0f); }

    float process (float in)
    {
        in = std::tanh (in * drive) / drive;

        const float v3 = in - ic2eq;
        const float v1 = a1 * ic1eq + a2 * v3;
        const float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        ic1eq = flushDenormal (2.0f * v1 - ic1eq);
        ic2eq = flushDenormal (2.0f * v2 - ic2eq);

        switch (mode)
        {
            case FilterMode::LowPass:  return v2;
            case FilterMode::BandPass: return v1;
            case FilterMode::HighPass: return in - k * v1 - v2;
            case FilterMode::Notch:    return in - k * v1;
            default:                   return v2;
        }
    }

private:
    void updateCoeffs()
    {
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    float sampleRate = 44100.0f;
    float cutoff = 1000.0f, res = 0.1f, drive = 1.0f;
    float g = 0.1f, k = 2.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1eq = 0.0f, ic2eq = 0.0f;
    FilterMode mode = FilterMode::LowPass;
};

} // namespace bb
