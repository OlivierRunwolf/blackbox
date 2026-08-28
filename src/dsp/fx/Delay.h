#pragma once
#include "../DspCommon.h"
#include <vector>

namespace bb {

// Stereo delay with a gentle low-pass in the feedback path, so repeats darken
// as they decay instead of hanging around bright forever.
class Delay
{
public:
    void prepare (float sr)
    {
        sampleRate = sr;
        size = (int) (2.5f * sr) + 4;      // 2.5 s ceiling
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        writePos = 0;
        lpL = lpR = 0.0f;
        smoothedDelay.reset (sr, 50.0f, 0.375f * sr);
    }

    void setParameters (float timeMs, float feedback, float mix)
    {
        smoothedDelay.setTarget (clampf (timeMs, 1.0f, 2000.0f) * 0.001f * sampleRate);
        fb  = clampf (feedback, 0.0f, 0.95f);
        wet = clampf (mix, 0.0f, 1.0f);
    }

    void process (float* l, float* r, int n)
    {
        if (wet <= 0.0f || size == 0) return;

        for (int i = 0; i < n; ++i)
        {
            const float d = smoothedDelay.next();

            const float dl = read (bufL, d);
            // Offsetting the right tap slightly gives the repeats a natural
            // stereo drift without a full ping-pong topology.
            const float dr = read (bufR, d * 1.005f);

            lpL = flushDenormal (lpL + 0.35f * (dl - lpL));
            lpR = flushDenormal (lpR + 0.35f * (dr - lpR));

            bufL[(size_t) writePos] = flushDenormal (l[i] + lpL * fb);
            bufR[(size_t) writePos] = flushDenormal (r[i] + lpR * fb);

            l[i] = lerp (l[i], dl, wet);
            r[i] = lerp (r[i], dr, wet);

            if (++writePos >= size) writePos = 0;
        }
    }

private:
    float read (const std::vector<float>& buf, float delaySamples) const
    {
        float pos = (float) writePos - clampf (delaySamples, 1.0f, (float) size - 2.0f);
        while (pos < 0.0f) pos += (float) size;

        const int   i0 = (int) pos;
        const int   i1 = (i0 + 1) % size;
        return lerp (buf[(size_t) i0], buf[(size_t) i1], pos - (float) i0);
    }

    std::vector<float> bufL, bufR;
    Smoothed smoothedDelay;
    float sampleRate = 44100.0f, fb = 0.35f, wet = 0.0f, lpL = 0.0f, lpR = 0.0f;
    int size = 0, writePos = 0;
};

} // namespace bb
