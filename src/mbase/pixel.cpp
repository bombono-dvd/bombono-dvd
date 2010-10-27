
#include <mbase/_pc_.h>

#include "pixel.h"

namespace RGBA
{

Pixel::Pixel(const unsigned int rgba)
{
    FromUint(rgba);
}

Pixel& Pixel::FromUint(const unsigned int rgba)
{
    red   =  rgba >> 24;
    green = (rgba & 0x00ff0000) >> 16;
    blue  = (rgba & 0x0000ff00) >> 8;
    alpha = (rgba & 0x000000ff);

    return *this;
}

unsigned int Pixel::ToUint()
{
    return (red << 24) | (green << 16) | (blue << 8) | alpha;
}

} // namespace RGBA

