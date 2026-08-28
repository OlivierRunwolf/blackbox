#pragma once
#include "DspCommon.h"
#include <cstdint>

namespace bb {

enum class Waveform { Saw = 0, Square, Triangle, Sine, Noise, Count };

// PolyBLEP-corrected oscillator. The naive versions of saw/square alias badly
// at the top of the keyboard; the BLEP residual cleans up the discontinuities
// for a fraction of the cost of oversampling.
class Oscillator
{
public:
    void prepare (float sr)
    {
        sampleRate = sr;
        phase = 0.0f;
        triState = 0.0f;
    }

    void setPhase (float p) { phase = p - std::floor (p); }

    void setFrequency (float hz)
    {
        freq = hz;
        inc  = clampf (hz / sampleRate, 0.0f, 0.49f);
    }

    void setWaveform (Waveform w) { wave = w; }

    // Pulse width for the square wave, 0.02..0.98.
    void setPulseWidth (float pw) { width = clampf (pw, 0.02f, 0.98f); }

    float next()
    {
        float out = 0.0f;

        switch (wave)
        {
            case Waveform::Saw:
                out = 2.0f * phase - 1.0f;
                out -= polyBlep (phase);
                break;

            case Waveform::Square:
            {
                out = phase < width ? 1.0f : -1.0f;
                out += polyBlep (phase);
                float p2 = phase - width;
                if (p2 < 0.0f) p2 += 1.0f;
                out -= polyBlep (p2);
                break;
            }

            case Waveform::Triangle:
            {
                // Leaky-integrated square gives a band-limited triangle.
                float sq = phase < 0.5f ? 1.0f : -1.0f;
                sq += polyBlep (phase);
                float p2 = phase - 0.5f;
                if (p2 < 0.0f) p2 += 1.0f;
                sq -= polyBlep (p2);
                triState = flushDenormal (triState + 4.0f * inc * sq - 0.0001f * triState);
                out = triState;
                break;
            }

            case Waveform::Sine:
                out = std::sin (kTwoPi * phase);
                break;

            case Waveform::Noise:
                out = whiteNoise();
                break;

            default: break;
        }

        phase += inc;
        if (phase >= 1.0f) phase -= 1.0f;

        return out;
    }

    // Hard sync: restart this oscillator's phase (used by osc2 <- osc1).
    void hardSync() { phase = 0.0f; triState = 0.0f; }

    float phasePos() const { return phase; }

private:
    float polyBlep (float t) const
    {
        if (inc <= 0.0f) return 0.0f;

        if (t < inc)                  // just after a discontinuity
        {
            t /= inc;
            return t + t - t * t - 1.0f;
        }
        if (t > 1.0f - inc)           // just before one
        {
            t = (t - 1.0f) / inc;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    float whiteNoise()
    {
        // xorshift32 - fast, and good enough for an audio noise source.
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float) (int32_t) rng * (1.0f / 2147483648.0f);
    }

    float sampleRate = 44100.0f;
    float freq = 440.0f, inc = 0.01f, phase = 0.0f, width = 0.5f, triState = 0.0f;
    Waveform wave = Waveform::Saw;
    uint32_t rng = 0x9E3779B9u;
};

} // namespace bb
