// Offline renderer for the BLACKBOX engine.
//
// The plugin build needs MSVC and a host; this does not. It drives the same
// SynthEngine with a scripted note sequence and writes a WAV, so the DSP can be
// developed and regression-checked without a DAW anywhere in the loop.

#include "../src/dsp/SynthEngine.h"

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <cmath>

namespace {

void writeWav (const std::string& path, const std::vector<float>& l, const std::vector<float>& r, int sampleRate)
{
    const uint32_t frames     = (uint32_t) l.size();
    const uint16_t channels   = 2;
    const uint16_t bits       = 16;
    const uint32_t byteRate   = (uint32_t) sampleRate * channels * bits / 8;
    const uint16_t blockAlign = channels * bits / 8;
    const uint32_t dataBytes  = frames * blockAlign;

    FILE* f = std::fopen (path.c_str(), "wb");
    if (f == nullptr) { std::fprintf (stderr, "cannot open %s\n", path.c_str()); return; }

    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };

    std::fwrite ("RIFF", 1, 4, f); u32 (36 + dataBytes); std::fwrite ("WAVE", 1, 4, f);
    std::fwrite ("fmt ", 1, 4, f); u32 (16); u16 (1); u16 (channels);
    u32 ((uint32_t) sampleRate); u32 (byteRate); u16 (blockAlign); u16 (bits);
    std::fwrite ("data", 1, 4, f); u32 (dataBytes);

    for (uint32_t i = 0; i < frames; ++i)
    {
        auto conv = [] (float v) -> int16_t
        {
            v = bb::clampf (v, -1.0f, 1.0f);
            return (int16_t) std::lrint (v * 32767.0f);
        };
        u16 ((uint16_t) conv (l[i]));
        u16 ((uint16_t) conv (r[i]));
    }

    std::fclose (f);
}

struct Event { double timeSec; bool on; int note; float vel; };

void report (const char* name, const std::vector<float>& l, const std::vector<float>& r)
{
    double peak = 0.0, sum = 0.0, dc = 0.0;
    int    bad  = 0;

    for (size_t i = 0; i < l.size(); ++i)
    {
        for (float v : { l[i], r[i] })
        {
            if (std::isnan (v) || std::isinf (v)) ++bad;
            peak = std::max (peak, (double) std::fabs (v));
            sum += (double) v * v;
            dc  += (double) v;
        }
    }

    const double n    = (double) l.size() * 2.0;
    const double rms  = std::sqrt (sum / n);
    const double rmsD = 20.0 * std::log10 (rms + 1e-12);

    std::printf ("%-14s peak %6.3f  rms %7.2f dB  dc %+8.5f  bad %d %s\n",
                 name, peak, rmsD, dc / n, bad,
                 (bad == 0 && peak > 0.001 && peak <= 1.0) ? "OK" : "<-- CHECK");
}

} // namespace

int main (int argc, char** argv)
{
    const int sampleRate = 48000;
    const int block      = 128;
    const double seconds = 6.0;
    const std::string out = argc > 1 ? argv[1] : "blackbox-test.wav";

    bb::SynthEngine synth;
    synth.prepare ((float) sampleRate, block);

    auto& p = synth.parameters();

    // A patch that exercises every subsystem at once: both oscillators, sub,
    // unison spread, a filter envelope, and all three effects.
    p.osc1.wave = bb::Waveform::Saw;    p.osc1.level = 0.8f;
    p.osc2.wave = bb::Waveform::Square; p.osc2.level = 0.5f; p.osc2.semitones = -12.0f; p.osc2.fine = 6.0f;
    p.subLevel   = 0.35f;
    p.noiseLevel = 0.04f;

    p.unison.count = 5; p.unison.detune = 0.35f; p.unison.spread = 0.8f;

    p.filter.mode = bb::FilterMode::LowPass;
    p.filter.cutoffHz = 500.0f;
    p.filter.resonance = 0.55f;
    p.filter.drive = 2.0f;
    p.filter.envAmount = 0.55f;
    p.filter.keyTrack = 0.4f;

    p.ampEnv = { 8.0f, 400.0f, 0.65f, 700.0f };
    p.modEnv = { 2.0f, 550.0f, 0.20f, 400.0f };

    // Fixed LFO destinations: a touch of vibrato from LFO 1, a slow filter
    // sweep from LFO 2.
    p.lfo[0].shape = bb::LfoShape::Sine;     p.lfo[0].rateHz = 4.5f;  p.lfo[0].depth = 0.04f;
    p.lfo[1].shape = bb::LfoShape::Triangle; p.lfo[1].rateHz = 0.25f; p.lfo[1].depth = 0.15f;

    p.fx.chorusRate = 0.5f; p.fx.chorusDepth = 0.6f; p.fx.chorusMix = 0.35f;
    p.fx.delayTimeMs = 330.0f; p.fx.delayFeedback = 0.38f; p.fx.delayMix = 0.22f;
    p.fx.reverbSize = 0.75f; p.fx.reverbDamp = 0.35f; p.fx.reverbMix = 0.25f;

    p.masterGain = 0.5f;
    p.maxVoices  = 16;

    // Two chords, the second played harder to prove velocity routing works.
    const std::vector<Event> events = {
        { 0.20, true,  48, 0.75f }, { 0.20, true,  55, 0.75f }, { 0.20, true,  60, 0.75f },
        { 0.20, true,  64, 0.75f }, { 0.20, true,  67, 0.75f },
        { 2.20, false, 48, 0.0f  }, { 2.20, false, 55, 0.0f  }, { 2.20, false, 60, 0.0f },
        { 2.20, false, 64, 0.0f  }, { 2.20, false, 67, 0.0f  },
        { 2.60, true,  45, 1.00f }, { 2.60, true,  52, 1.00f }, { 2.60, true,  57, 1.00f },
        { 2.60, true,  61, 1.00f }, { 2.60, true,  64, 1.00f },
        { 4.60, false, 45, 0.0f  }, { 4.60, false, 52, 0.0f  }, { 4.60, false, 57, 0.0f },
        { 4.60, false, 61, 0.0f  }, { 4.60, false, 64, 0.0f  },
    };

    const int total = (int) (seconds * sampleRate);
    std::vector<float> outL ((size_t) total, 0.0f), outR ((size_t) total, 0.0f);

    size_t nextEvent = 0;
    int    maxVoices = 0;

    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        const double t = (double) pos / sampleRate;

        while (nextEvent < events.size() && events[nextEvent].timeSec <= t)
        {
            const auto& e = events[nextEvent];
            if (e.on) synth.noteOn (e.note, e.vel);
            else      synth.noteOff (e.note);
            ++nextEvent;
        }

        synth.render (outL.data() + pos, outR.data() + pos, n);
        maxVoices = std::max (maxVoices, synth.activeVoiceCount());
    }

    report ("full patch", outL, outR);
    std::printf ("%-14s peak voices %d / %d\n", "", maxVoices, p.maxVoices);

    writeWav (out, outL, outR, sampleRate);
    std::printf ("%-14s wrote %s (%.1f s, %d Hz)\n", "", out.c_str(), seconds, sampleRate);

    return 0;
}
