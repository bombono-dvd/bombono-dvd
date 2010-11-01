//
// mbase/pixel.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 

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

