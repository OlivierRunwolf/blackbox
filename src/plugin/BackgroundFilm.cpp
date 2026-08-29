#include "BackgroundFilm.h"
#include "BinaryData.h"

namespace bb {

namespace {

struct Stop { float pos; juce::uint32 argb; };

// Sampled from the reference photograph: cooled crust, through purple and
// magenta heat, into the orange-yellow core of the flow.
const Stop kMagma[] = {
    { 0.00f, 0xff050308 },
    { 0.18f, 0xff1a0a20 },
    { 0.38f, 0xff5c1840 },
    { 0.58f, 0xffa82a3e },
    { 0.74f, 0xffe0561a },
    { 0.88f, 0xffff9a24 },
    { 1.00f, 0xffffd86b },
};

} // namespace

juce::Colour BackgroundFilm::magma (float l)
{
    l = juce::jlimit (0.0f, 1.0f, l);

    const int count = (int) (sizeof (kMagma) / sizeof (Stop));

    for (int i = 1; i < count; ++i)
    {
        if (l <= kMagma[i].pos)
        {
            const auto& a = kMagma[i - 1];
            const auto& b = kMagma[i];
            const float t = (l - a.pos) / juce::jmax (1.0e-6f, b.pos - a.pos);
            return juce::Colour (a.argb).interpolatedWith (juce::Colour (b.argb), t);
        }
    }

    return juce::Colour (kMagma[count - 1].argb);
}

BackgroundFilm::BackgroundFilm()
{
    // Build the ramp once - a per-pixel gradient search over 2.6M pixels would
    // be needlessly slow.
    juce::Colour lut[256];
    for (int i = 0; i < 256; ++i)
        lut[i] = magma ((float) i / 255.0f);

    for (int i = 0; i < 64; ++i)   // stops at the first missing frame
    {
        const auto name = juce::String::formatted ("bg%02d_png", i);

        int size = 0;
        const char* data = BinaryData::getNamedResource (name.toRawUTF8(), size);
        if (data == nullptr || size == 0)
            break;

        const auto src = juce::ImageFileFormat::loadFrom (data, (size_t) size);
        if (! src.isValid())
            break;

        juce::Image dst (juce::Image::ARGB, src.getWidth(), src.getHeight(), false);
        {
            const juce::Image::BitmapData in  (src, juce::Image::BitmapData::readOnly);
            juce::Image::BitmapData       out (dst, juce::Image::BitmapData::writeOnly);

            for (int y = 0; y < src.getHeight(); ++y)
                for (int x = 0; x < src.getWidth(); ++x)
                    out.setPixelColour (x, y, lut[in.getPixelColour (x, y).getRed()]);
        }

        frames.add (dst);
    }

    jassert (! frames.isEmpty());
}

} // namespace bb
