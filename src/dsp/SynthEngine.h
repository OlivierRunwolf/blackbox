#pragma once
#include "Voice.h"
#include "fx/Chorus.h"
#include "fx/Delay.h"
#include "fx/Reverb.h"
#include <vector>

namespace bb {

// Top-level synth. Owns the voice pool, note routing and the FX bus. Knows
// nothing about JUCE or VST3 - the plugin wrapper just feeds it note events
// and asks for audio.
class SynthEngine
{
public:
    void prepare (float sr, int maxBlockSize)
    {
        sampleRate = sr;

        for (auto& v : voices)
            v.prepare (sr);

        chorus.prepare (sr);
        delay.prepare (sr);
        reverb.prepare (sr);

        scratchL.assign ((size_t) std::max (1, maxBlockSize), 0.0f);
        scratchR.assign ((size_t) std::max (1, maxBlockSize), 0.0f);

        masterGain.reset (sr, 20.0f, params.masterGain);
        dcL.reset (sr);
        dcR.reset (sr);
        stamp = 0;
    }

    SynthParams& parameters() { return params; }
    const SynthParams& parameters() const { return params; }

    void reset()
    {
        for (auto& v : voices) v.kill();
    }

    void noteOn (int midiNote, float velocity)
    {
        const int limit = std::max (1, std::min (params.maxVoices, kMaxVoices));

        // Retrigger the same note if it is already sounding, rather than
        // stacking a second voice on it.
        for (int i = 0; i < limit; ++i)
        {
            if (voices[i].isActive() && voices[i].currentNote() == midiNote && ! voices[i].isReleasing())
            {
                voices[i].noteOn (midiNote, velocity, params, ++stamp, true);
                return;
            }
        }

        Voice* target = nullptr;

        for (int i = 0; i < limit; ++i)
            if (! voices[i].isActive()) { target = &voices[i]; break; }

        if (target == nullptr)
            target = &voices[(size_t) stealIndex (limit)];

        target->setPitchBend (bendSemis);
        target->setModWheel (modWheel);
        target->noteOn (midiNote, velocity, params, ++stamp, false);
    }

    void noteOff (int midiNote)
    {
        const int limit = std::max (1, std::min (params.maxVoices, kMaxVoices));

        for (int i = 0; i < limit; ++i)
            if (voices[i].isActive() && voices[i].currentNote() == midiNote && ! voices[i].isReleasing())
                voices[i].noteOff();
    }

    void allNotesOff()
    {
        for (auto& v : voices) v.noteOff();
    }

    void setPitchBend (float normalised)   // -1..1
    {
        bendSemis = normalised * params.bendRange;
        for (auto& v : voices) v.setPitchBend (bendSemis);
    }

    void setModWheel (float v)             // 0..1
    {
        modWheel = clampf (v, 0.0f, 1.0f);
        for (auto& voice : voices) voice.setModWheel (modWheel);
    }

    // Adds into outL/outR - the caller is expected to hand over cleared buffers.
    void render (float* outL, float* outR, int numSamples)
    {
        if ((int) scratchL.size() < numSamples)
        {
            scratchL.assign ((size_t) numSamples, 0.0f);
            scratchR.assign ((size_t) numSamples, 0.0f);
        }

        std::fill (scratchL.begin(), scratchL.begin() + numSamples, 0.0f);
        std::fill (scratchR.begin(), scratchR.begin() + numSamples, 0.0f);

        const int limit = std::max (1, std::min (params.maxVoices, kMaxVoices));

        for (int i = 0; i < limit; ++i)
            if (voices[i].isActive())
                voices[i].render (scratchL.data(), scratchR.data(), numSamples, params);

        chorus.setParameters (params.fx.chorusRate, params.fx.chorusDepth, params.fx.chorusMix);
        delay .setParameters (params.fx.delayTimeMs, params.fx.delayFeedback, params.fx.delayMix);
        reverb.setParameters (params.fx.reverbSize, params.fx.reverbDamp, params.fx.reverbMix);

        chorus.process (scratchL.data(), scratchR.data(), numSamples);
        delay .process (scratchL.data(), scratchR.data(), numSamples);
        reverb.process (scratchL.data(), scratchR.data(), numSamples);

        masterGain.setTarget (params.masterGain);

        for (int n = 0; n < numSamples; ++n)
        {
            const float g = masterGain.next();
            outL[n] += dcL.process (scratchL[(size_t) n]) * g;
            outR[n] += dcR.process (scratchR[(size_t) n]) * g;
        }
    }

    int activeVoiceCount() const
    {
        int c = 0;
        for (const auto& v : voices) if (v.isActive()) ++c;
        return c;
    }

private:
    // Prefer stealing something already releasing; otherwise take the oldest.
    int stealIndex (int limit) const
    {
        int best = 0;
        uint64_t bestAge = UINT64_MAX;
        bool foundReleasing = false;

        for (int i = 0; i < limit; ++i)
        {
            const bool rel = voices[i].isReleasing();

            if (rel && ! foundReleasing) { foundReleasing = true; bestAge = UINT64_MAX; }
            if (foundReleasing && ! rel) continue;

            if (voices[i].age() < bestAge) { bestAge = voices[i].age(); best = i; }
        }

        return best;
    }

    SynthParams params;
    Voice       voices[kMaxVoices];
    Chorus      chorus;
    Delay       delay;
    Reverb      reverb;
    Smoothed    masterGain;
    DcBlocker   dcL, dcR;

    std::vector<float> scratchL, scratchR;
    float sampleRate = 44100.0f, bendSemis = 0.0f, modWheel = 0.0f;
    uint64_t stamp = 0;
};

} // namespace bb
