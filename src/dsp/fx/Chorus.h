#pragma once
#include "../DspCommon.h"
#include <vector>

namespace bb {

// Two-tap modulated delay. The taps run in quadrature so the left and right
// sides move against each other, which widens the image without comb-filtering
// the mono sum too badly.
class Chorus
{
public:
    void prepare (float sr)
    {
        sampleRate = sr;
        const int len = (int) (0.05f * sr) + 4;
        bufL.assign ((size_t) len, 0.0f);
        bufR.assign ((size_t) len, 0.0f);
        size = len;
        writePos = 0;
        phase = 0.0f;
    }

    void setParameters (float rateHz, float depth, float mix)
    {
        rate    = clampf (rateHz, 0.01f, 10.0f);
        depthMs = clampf (depth, 0.0f, 1.0f) * 8.0f;
        wet     = clampf (mix, 0.0f, 1.0f);
    }

    void process (float* l, float* r, int n)
    {
        if (wet <= 0.0f || size == 0) return;

        const float inc      = rate / sampleRate;
        const float baseMs   = 12.0f;

        for (int i = 0; i < n; ++i)
        {
            bufL[(size_t) writePos] = l[i];
            bufR[(size_t) writePos] = r[i];

            const float modL = std::sin (kTwoPi * phase);
            const float modR = std::sin (kTwoPi * phase + kPi * 0.5f);

            const float dL = (baseMs + depthMs * modL) * 0.001f * sampleRate;
            const float dR = (baseMs + depthMs * modR) * 0.001f * sampleRate;

            const float wetL = readInterp (bufL, dL);
            const float wetR = readInterp (bufR, dR);

            l[i] = lerp (l[i], wetL, wet);
            r[i] = lerp (r[i], wetR, wet);

            if (++writePos >= size) writePos = 0;
            phase += inc;
            if (phase >= 1.0f) phase -= 1.0f;
        }
    }

private:
    float readInterp (const std::vector<float>& buf, float delaySamples) const
    {
        float pos = (float) writePos - delaySamples;
        while (pos < 0.0f) pos += (float) size;

        const int   i0 = (int) pos;
        const int   i1 = (i0 + 1) % size;
        const float fr = pos - (float) i0;

        return lerp (buf[(size_t) i0], buf[(size_t) i1], fr);
    }

    std::vector<float> bufL, bufR;
    float sampleRate = 44100.0f, rate = 0.6f, depthMs = 0.0f, wet = 0.0f, phase = 0.0f;
    int size = 0, writePos = 0;
};

} // namespace bb
