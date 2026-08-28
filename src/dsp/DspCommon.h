#pragma once
#include <cmath>
#include <algorithm>

namespace bb {

constexpr float kPi     = 3.14159265358979323846f;
constexpr float kTwoPi  = 2.0f * kPi;

inline float clampf (float v, float lo, float hi) { return std::min (std::max (v, lo), hi); }

inline float lerp (float a, float b, float t) { return a + (b - a) * t; }

// MIDI note -> Hz, with fractional notes allowed (detune, glide, pitch bend).
inline float noteToHz (float note) { return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f); }

// Equal-power pan: p in [-1, 1].
inline void panGains (float p, float& l, float& r)
{
    const float a = (clampf (p, -1.0f, 1.0f) + 1.0f) * 0.25f * kPi;
    l = std::cos (a);
    r = std::sin (a);
}

inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }

// Cheap denormal guard - denormals in the filter/reverb state are a real CPU
// cliff on Windows, and this costs nothing.
inline float flushDenormal (float v) { return std::abs (v) < 1.0e-20f ? 0.0f : v; }

// One-pole smoother for parameters that would otherwise zipper.
class Smoothed
{
public:
    void reset (float sampleRate, float timeMs, float initial = 0.0f)
    {
        coeff   = 1.0f - std::exp (-1.0f / (0.001f * timeMs * sampleRate));
        current = target = initial;
    }

    void setTarget (float v) { target = v; }
    void snap (float v)      { current = target = v; }

    float next()
    {
        current += coeff * (target - current);
        return current;
    }

    float value() const { return current; }

private:
    float coeff = 0.01f, current = 0.0f, target = 0.0f;
};

// One-pole DC blocker. The sub oscillator and the leaky triangle integrator
// both leave a small offset behind; left alone it accumulates in the delay and
// reverb feedback paths and eats headroom.
class DcBlocker
{
public:
    void reset (float sampleRate)
    {
        r = 1.0f - (kTwoPi * 5.0f / sampleRate);
        x1 = y1 = 0.0f;
    }

    float process (float x)
    {
        const float y = x - x1 + r * y1;
        x1 = x;
        y1 = flushDenormal (y);
        return y1;
    }

private:
    float r = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

} // namespace bb
