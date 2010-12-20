//
// mlib/sdk/misc.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
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

#include "misc.h"

#include <mlib/format.h>
#include <mlib/tech.h>
#include <mlib/regex.h>
#include <mlib/string.h>

// = НОД
int GCD(int a, int b)
{
    a = (a >= 0) ? a : -a;
    b = (b >= 0) ? b : -b;
    
    for( int x; x=b, b>0; ) 
    {
        b = a % b;
        a = x;
    }
    return a;
}

void ReducePair(Point& p)
{
    int d = GCD(p.x, p.y);
    if( d )
    {
        p.x /= d;
        p.y /= d;
    }
}

MpegDP::MpegDP(bool is_pal)
    : aFrmt(af4_3), MyParent(is_pal ? Point(720, 576) : Point(720, 480))
{}

Point MpegDP::DisplayAspect() const
{
    Point res;
    switch( aFrmt )
    {
    case afSQUARE:
        res = sz;
        ReducePair(res);
        break;
    case af4_3:
        res = Point(4, 3);
        break;
    case af16_9:
        res = Point(16, 9);
        break;
    case af221_100:
        res = Point(221, 100);
        break;
    default:
        ASSERT(0);
    }
    return res;
}

Point MpegDP::PixelAspect() const
{
    if( aFrmt == afSQUARE )
        return Point(1, 1);

    Point asp = DisplayAspect();
    asp.x *= sz.y;
    asp.y *= sz.x;
    ReducePair(asp);
    return asp;
}

bool TryConvertParams(MpegDP& dst, const DisplayParams& src)
{
    bool res = true;
    AspectFormat af;
    if( src.PixelAspect() == Point(1, 1) )
        af = afSQUARE;
    else
    {
        Point dasp = src.DisplayAspect();
        if( dasp == Point(4, 3) )
            af = af4_3;
        else if( dasp == Point(16, 9) )
            af = af16_9;
        else if( dasp == Point(221, 100) )
            af = af221_100;
        else
            res = false;
    }

    if( res )
    {
        dst.GetSize() = src.Size();
        dst.GetAF()   = af;
    }
    return res;
}

namespace Str {

// считать целое из строки
bool GetLong(long& res, const char* str)
{
    // в стандарте POSIX записано, что нужно проверять 
    // errno, а это ни фига не дает
    // точно проверить можно по end == src - не число!
    //errno = 1;
    char* end;
    res = strtol(str, &end, 10);
    return end != str;
}

bool GetDouble(double& res, const char* str)
{
    char* end;
    res = strtod(str, &end);
    return end != str;
}

} // namespace Str

std::string PointToStr(const Point& pnt)
{
    return boost::format("%1% \303\227 %2%") % pnt.x % pnt.y % bf::stop;
}

std::string Double2Str(double val)
{
    //return boost::format("%1%") % val % bf::stop;
    return (str::stream() << val).str();
}

std::string Int2Str(int val)
{
    return (str::stream() << val).str();
}

bool ExtMatch(const char* display_name, const char* ext)
{
    // :TODO: ради ускорения сделать словарь (map) образцов сравнения по мере
    // использования
    re::pattern pat((std::string(".*\\.") + ext).c_str(), re::constants::icase);
    return re::match(display_name, pat);
}

std::string QuotedName(const std::string& str)
{
    return "\"" + str + "\"";
}

