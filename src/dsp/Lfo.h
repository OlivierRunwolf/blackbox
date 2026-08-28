#pragma once
#include "DspCommon.h"
#include <cstdint>

namespace bb {

enum class LfoShape { Sine = 0, Triangle, Saw, Square, SampleHold, Count };

class Lfo
{
public:
    void prepare (float sr) { sampleRate = sr; phase = 0.0f; held = 0.0f; }

    void setRate (float hz) { inc = clampf (hz, 0.01f, 50.0f) / sampleRate; }
    void setShape (LfoShape s) { shape = s; }
    void retrigger() { phase = 0.0f; }

    // Returns bipolar -1..1.
    float next()
    {
        float out = 0.0f;

        switch (shape)
        {
            case LfoShape::Sine:     out = std::sin (kTwoPi * phase); break;
            case LfoShape::Triangle: out = 4.0f * std::abs (phase - 0.5f) - 1.0f; break;
            case LfoShape::Saw:      out = 2.0f * phase - 1.0f; break;
            case LfoShape::Square:   out = phase < 0.5f ? 1.0f : -1.0f; break;
            case LfoShape::SampleHold: out = held; break;
            default: break;
        }

        phase += inc;
        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            held = (float) (int32_t) rng * (1.0f / 2147483648.0f);
        }

        return out;
    }

private:
    float sampleRate = 44100.0f, phase = 0.0f, inc = 0.0001f, held = 0.0f;
    LfoShape shape = LfoShape::Sine;
    uint32_t rng = 0x1234567u;
};

} // namespace bb
