#pragma once
#include "Params.h"
#include "Envelope.h"

namespace bb {

// Full-scale depth of the two fixed LFO destinations.
constexpr float kMaxVibratoSemis    = 7.0f;
constexpr float kMaxFilterSweepOct  = 4.0f;

// One polyphonic voice. Unison is handled inside the voice rather than by
// stealing extra voices, so an 8-note chord at 8x unison still costs 8 voices
// of allocation and never runs the polyphony out.
class Voice
{
public:
    void prepare (float sr)
    {
        sampleRate = sr;

        for (int i = 0; i < kMaxUnison; ++i)
        {
            osc1[i].prepare (sr);
            osc2[i].prepare (sr);
            sub[i].prepare (sr);
            sub[i].setWaveform (Waveform::Square);
        }

        noise.prepare (sr);
        noise.setWaveform (Waveform::Noise);
        noise.setFrequency (1000.0f);   // unused by the noise path, kept sane

        ampEnv.prepare (sr);
        modEnv.prepare (sr);
        for (auto& l : lfo) l.prepare (sr);

        filterL.prepare (sr);
        filterR.prepare (sr);

        glide.reset (sr, 1.0f, 60.0f);
    }

    bool isActive() const { return ampEnv.isActive(); }
    int  currentNote() const { return note; }
    bool isReleasing() const { return ampEnv.currentStage() == Envelope::Stage::Release; }
    uint64_t age() const { return startStamp; }

    void noteOn (int midiNote, float vel, const SynthParams& p, uint64_t stamp, bool legato)
    {
        note       = midiNote;
        velocity   = vel;
        startStamp = stamp;

        const float target = (float) midiNote;

        if (legato && p.glideMs > 0.0f)
        {
            glide.reset (sampleRate, p.glideMs, glide.value());
        }
        else
        {
            glide.reset (sampleRate, std::max (0.1f, p.glideMs), p.glideMs > 0.0f ? glide.value() : target);
            if (p.glideMs <= 0.0f) glide.snap (target);
        }
        glide.setTarget (target);

        if (! legato)
        {
            for (int i = 0; i < kMaxUnison; ++i)
            {
                // Randomised start phases stop unison voices from summing into
                // a single loud transient click on every note.
                osc1[i].setPhase (randomUnit());
                osc2[i].setPhase (randomUnit());
                sub[i].setPhase (randomUnit());
            }

            filterL.reset();
            filterR.reset();

            for (int i = 0; i < kNumLfos; ++i)
                if (p.lfo[i].retrigger) lfo[i].retrigger();
        }

        ampEnv.noteOn (! legato);
        modEnv.noteOn (! legato);
    }

    void noteOff()
    {
        ampEnv.noteOff();
        modEnv.noteOff();
    }

    void kill()
    {
        ampEnv.reset();
        modEnv.reset();
        note = -1;
    }

    void setPitchBend (float semis) { bend = semis; }
    void setModWheel  (float v)     { modWheel = v; }

    void render (float* outL, float* outR, int numSamples, const SynthParams& p)
    {
        if (! ampEnv.isActive())
            return;

        const int   uCount = std::max (1, std::min (p.unison.count, kMaxUnison));
        const float uGain  = 1.0f / std::sqrt ((float) uCount);
        const float maxDetuneCents = 50.0f * p.unison.detune;

        ampEnv.setParameters (p.ampEnv.attackMs, p.ampEnv.decayMs, p.ampEnv.sustain, p.ampEnv.releaseMs);
        modEnv.setParameters (p.modEnv.attackMs, p.modEnv.decayMs, p.modEnv.sustain, p.modEnv.releaseMs);

        for (int i = 0; i < kNumLfos; ++i)
            lfo[i].setShape (p.lfo[i].shape);

        for (int n = 0; n < numSamples; ++n)
        {
            // --- modulation ----------------------------------------------
            const float e1 = ampEnv.next();
            const float e2 = modEnv.next();

            for (int i = 0; i < kNumLfos; ++i)
                lfo[i].setRate (p.lfo[i].rateHz);

            const float l1 = lfo[0].next();
            const float l2 = lfo[1].next();

            // The mod wheel adds vibrato on top of the panel setting, which is
            // what a player expects the wheel to do on a synth like this.
            const float vibrato = clampf (p.lfo[0].depth + modWheel, 0.0f, 1.0f) * l1 * kMaxVibratoSemis;

            // --- pitch ---------------------------------------------------
            const float basePitch = glide.next() + bend;
            const float p1 = basePitch + p.osc1.semitones + p.osc1.fine * 0.01f + vibrato;
            const float p2 = basePitch + p.osc2.semitones + p.osc2.fine * 0.01f + vibrato;

            const float detuneMod = clampf (maxDetuneCents, 0.0f, 100.0f);
            const float pw = p.osc1.pulseWidth;

            const float lvl1 = clampf (p.osc1.level, 0.0f, 1.0f);
            const float lvl2 = clampf (p.osc2.level, 0.0f, 1.0f);
            const float lvlS = clampf (p.subLevel,   0.0f, 1.0f);
            const float lvlN = clampf (p.noiseLevel, 0.0f, 1.0f);

            float sumL = 0.0f, sumR = 0.0f;

            for (int u = 0; u < uCount; ++u)
            {
                const float spreadPos = uCount > 1 ? ((float) u / (float) (uCount - 1)) * 2.0f - 1.0f : 0.0f;
                const float cents     = spreadPos * detuneMod;
                const float ratio     = std::pow (2.0f, cents / 1200.0f);

                osc1[u].setWaveform (p.osc1.wave);
                osc2[u].setWaveform (p.osc2.wave);
                osc1[u].setPulseWidth (pw);
                osc2[u].setPulseWidth (p.osc2.pulseWidth);

                osc1[u].setFrequency (noteToHz (p1) * ratio);
                osc2[u].setFrequency (noteToHz (p2) * ratio);
                sub [u].setFrequency (noteToHz (p1 - 12.0f) * ratio);

                const float prevPhase = osc1[u].phasePos();
                float s = osc1[u].next() * lvl1;

                // Osc1 wrapping is the sync trigger for osc2.
                if (p.osc2Sync && osc1[u].phasePos() < prevPhase)
                    osc2[u].hardSync();

                s += osc2[u].next() * lvl2;
                s += sub [u].next() * lvlS * 0.7f;

                float gl, gr;
                panGains (spreadPos * p.unison.spread, gl, gr);
                sumL += s * gl;
                sumR += s * gr;
            }

            sumL *= uGain;
            sumR *= uGain;

            if (lvlN > 0.0f)
            {
                const float nz = noise.next() * lvlN * 0.5f;
                sumL += nz;
                sumR += nz;
            }

            // --- filter ---------------------------------------------------
            const float keyOct  = p.filter.keyTrack * ((float) note - 60.0f) / 12.0f;
            const float envOct  = p.filter.envAmount * e2 * 6.0f;
            const float modOct  = p.lfo[1].depth * l2 * kMaxFilterSweepOct;
            const float cutoff  = clampf (p.filter.cutoffHz * std::pow (2.0f, envOct + keyOct + modOct),
                                          20.0f, sampleRate * 0.45f);
            const float reso    = clampf (p.filter.resonance, 0.0f, 1.0f);
            const float drv     = clampf (p.filter.drive, 1.0f, 10.0f);

            for (auto* f : { &filterL, &filterR })
            {
                f->setMode (p.filter.mode);
                f->setCutoff (cutoff);
                f->setResonance (reso);
                f->setDrive (drv);
            }

            sumL = filterL.process (sumL);
            sumR = filterR.process (sumR);

            // --- amp ------------------------------------------------------
            const float amp = clampf (e1, 0.0f, 2.0f);

            outL[n] += sumL * amp;
            outR[n] += sumR * amp;
        }
    }

private:
    float randomUnit()
    {
        rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
        return (float) (rngState >> 8) * (1.0f / 16777216.0f);
    }

    float sampleRate = 44100.0f;

    Oscillator osc1[kMaxUnison], osc2[kMaxUnison], sub[kMaxUnison], noise;
    Envelope   ampEnv, modEnv;
    Lfo        lfo[kNumLfos];
    StateVariableFilter filterL, filterR;
    Smoothed   glide;

    int      note = -1;
    float    velocity = 1.0f, bend = 0.0f, modWheel = 0.0f;
    uint64_t startStamp = 0;
    uint32_t rngState = 0xA5A5F00Du;
};

} // namespace bb
