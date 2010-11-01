//
// mbase/pixel.h
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

#ifndef __MBASE_PIXEL_H__
#define __MBASE_PIXEL_H__

#include <mlib/tech.h>

inline int Round(double val) { return int(round(val)); }

namespace RGBA
{

#include PACK_ON
struct Pixel
{
    typedef unsigned char ClrType;
    static const ClrType MinClr = 0;
    static const ClrType MaxClr = 255;

    ClrType  red;
    ClrType  green;
    ClrType  blue;
    ClrType  alpha;

             Pixel(): red(MinClr), green(MinClr), blue(MinClr), alpha(MaxClr) {}
             Pixel(ClrType r, ClrType g, ClrType b, ClrType a = MaxClr):
                 red(r), green(g), blue(b), alpha(a) {}
             Pixel(const unsigned int rgba);
             //Pixel(const Gdk::Color& clr);

      Pixel& FromUint(const unsigned int rgba);
unsigned int ToUint();

    static  double  FromQuant(ClrType c) { return (double)c/MaxClr; }
    static ClrType  ToQuant(double c)    { return ClrType( Round(c*MaxClr) ); }
};
#include PACK_OFF

} // namespace RGBA

#endif // #ifndef __MBASE_PIXEL_H__

