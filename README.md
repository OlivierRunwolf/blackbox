# BLACKBOX

A polyphonic subtractive synthesizer plugin (VST3) for FL Studio and other hosts.

## Architecture

The DSP core in `src/dsp/` is header-only, dependency-free C++17 with no JUCE
types in it. That is deliberate: it means the engine can be compiled and
regression-tested on any machine with a compiler, with no GUI libraries, no
audio device and no DAW. `src/plugin/` is the JUCE layer that wraps it.

    src/dsp/        engine - oscillators, filter, envelopes, LFOs, FX
    src/plugin/     JUCE wrapper - parameters, processor, editor, look and feel
    tools/render.cpp  offline renderer: drives the engine, writes a WAV

## Features

- Two oscillators (saw / square / triangle / sine / noise, PolyBLEP anti-aliased)
  plus a sub oscillator and a noise source, with optional osc 2 hard sync
- Up to 8-voice unison per note with detune and stereo spread
- TPT state-variable filter (LP / BP / HP / notch) with resonance, drive,
  envelope amount and key tracking
- Two ADSR envelopes and two LFOs (sine / tri / saw / square / S&H), each LFO
  with a fixed destination - LFO 1 vibrato, LFO 2 filter sweep - and a depth
  control; the mod wheel adds to vibrato depth on top of the panel setting
- Chorus, stereo delay and reverb, each its own section of the effects rack
- 16-voice polyphony with release-priority voice stealing, glide, pitch bend

## Building

**Windows VST3** is built by CI on every push - see `.github/workflows/build.yml`.
Download the `BLACKBOX-windows` artifact and copy `BLACKBOX.vst3` into
`C:\Program Files\Common Files\VST3`. The same artifact contains a standalone
`BLACKBOX.exe`, which needs no host at all.

**Locally**, the DSP core builds anywhere:

    cmake -B build -DBLACKBOX_BUILD_PLUGIN=OFF
    cmake --build build --target bb_render
    ./build/bb_render out.wav

The full plugin build additionally needs JUCE's dependencies. On Debian/Ubuntu:

    sudo apt install -y libasound2-dev libx11-dev libxext-dev libxrandr-dev \
      libxinerama-dev libxcursor-dev libfreetype6-dev libfontconfig1-dev \
      libgl1-mesa-dev pkg-config

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

## Licence

GPLv3, per the JUCE open-source licence this is built under.
