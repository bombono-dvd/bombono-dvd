
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

