#pragma once
#include "../DspCommon.h"
#include <vector>
#include <array>

namespace bb {

// Freeverb-style: 8 damped comb filters into 4 allpasses per channel. Cheap,
// well understood, and forgiving on a synth bus.
class Reverb
{
public:
    void prepare (float sr)
    {
        const float scale = sr / 44100.0f;

        static const int combTune[8]    = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int allpassTune[4] = { 556, 441, 341, 225 };
        constexpr int stereoSpread = 23;

        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < 8; ++i)
                combs[ch][i].prepare ((int) ((combTune[i] + ch * stereoSpread) * scale));

            for (int i = 0; i < 4; ++i)
                allpasses[ch][i].prepare ((int) ((allpassTune[i] + ch * stereoSpread) * scale));
        }
    }

    void setParameters (float size, float damp, float mix)
    {
        roomSize = 0.7f + 0.28f * clampf (size, 0.0f, 1.0f);
        damping  = clampf (damp, 0.0f, 1.0f) * 0.4f;
        wet      = clampf (mix, 0.0f, 1.0f);
    }

    void process (float* l, float* r, int n)
    {
        if (wet <= 0.0f) return;

        for (int i = 0; i < n; ++i)
        {
            const float in = (l[i] + r[i]) * 0.015f;
            float out[2] = { 0.0f, 0.0f };

            for (int ch = 0; ch < 2; ++ch)
            {
                for (auto& c : combs[ch])
                    out[ch] += c.process (in, roomSize, damping);

                for (auto& a : allpasses[ch])
                    out[ch] = a.process (out[ch]);
            }

            l[i] = lerp (l[i], out[0], wet);
            r[i] = lerp (r[i], out[1], wet);
        }
    }

private:
    struct Comb
    {
        void prepare (int n) { buf.assign ((size_t) std::max (1, n), 0.0f); pos = 0; store = 0.0f; }

        float process (float in, float feedback, float damp)
        {
            const float out = buf[(size_t) pos];
            store = flushDenormal (out * (1.0f - damp) + store * damp);
            buf[(size_t) pos] = flushDenormal (in + store * feedback);
            if (++pos >= (int) buf.size()) pos = 0;
            return out;
        }

        std::vector<float> buf;
        int pos = 0;
        float store = 0.0f;
    };

    struct Allpass
    {
        void prepare (int n) { buf.assign ((size_t) std::max (1, n), 0.0f); pos = 0; }

        float process (float in)
        {
            const float bufout = buf[(size_t) pos];
            const float out    = -in + bufout;
            buf[(size_t) pos]  = flushDenormal (in + bufout * 0.5f);
            if (++pos >= (int) buf.size()) pos = 0;
            return out;
        }

        std::vector<float> buf;
        int pos = 0;
    };

    std::array<std::array<Comb, 8>, 2>    combs;
    std::array<std::array<Allpass, 4>, 2> allpasses;
    float roomSize = 0.9f, damping = 0.2f, wet = 0.0f;
};

} // namespace bb
