#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace bb {

// The animated panel backdrop. The frames ship as greyscale - they were
// monochrome to begin with, so storing one channel rather than three keeps the
// binary small - and the magma colouring is applied here at load time, which
// means the palette can be retuned without re-encoding any image.
class BackgroundFilm
{
public:
    BackgroundFilm();

    int numFrames() const                       { return frames.size(); }
    const juce::Image& frame (int index) const  { return frames.getReference (index % juce::jmax (1, frames.size())); }
    bool isEmpty() const                        { return frames.isEmpty(); }

    static constexpr int frameRateHz = 25;   // matches the source animation

private:
    static juce::Colour magma (float luminance);

    juce::Array<juce::Image> frames;
};

} // namespace bb
