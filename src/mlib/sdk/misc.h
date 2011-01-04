//
// mlib/sdk/misc.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008 Ilya Murav'jov
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

#ifndef __MLIB_MISC_H__
#define __MLIB_MISC_H__

#include <mlib/geom2d.h>

// = НОД
int GCD(int a, int b);
// сделать пару чисел взаимно простыми
void ReducePair(Point& p);

// 
// Работа с размерами и соотношением сторон изображения
// 

class DisplayParams
{
    public:
                   DisplayParams(const Point& sz_): sz(sz_) {}
    virtual       ~DisplayParams() {}

                   // размеры
            Point  Size() const { return sz; }
                   // соотношение сторон всего изображения
    virtual Point  DisplayAspect() const = 0;
                   // соотношение сторон пикселя
    virtual Point  PixelAspect() const = 0;

            Point& GetSize() { return sz; }

    protected:
               Point  sz;
};

enum AspectFormat
{
    afUNKNOWN = 0, // ошибка
    afSQUARE  = 1,  // пикселы квадратные
    af4_3     = 2,     // 4:3
    af16_9    = 3,    // 16:9
    af221_100 = 4  // 221:100
};

// 
// MpegDP - формат хранения параметров, используемый
// в MPEG(2)
// 

class MpegDP: public DisplayParams
{
    typedef DisplayParams MyParent;
    public:
                   MpegDP(bool is_pal);
                   MpegDP(AspectFormat af, const Point& sz_)
                        :MyParent(sz_), aFrmt(af) {}

            Point  DisplayAspect() const;
            Point  PixelAspect() const;

     AspectFormat& GetAF() { return aFrmt; }

    protected:
        AspectFormat  aFrmt;

                   MpegDP();
};

inline bool operator ==(MpegDP& dp, MpegDP& dp2)
{
    return (dp.GetSize() == dp2.GetSize()) && (dp.GetAF() == dp2.GetAF());
}

bool TryConvertParams(MpegDP& dst, const DisplayParams& src);
typedef MpegDP MenuParams;

//
class FullDP: public DisplayParams
{
    typedef DisplayParams MyParent;
    public:
                    FullDP(const Point& pasp, const Point& sz_)
                        :MyParent(sz_), pAsp(pasp) {}
            
    virtual  Point  DisplayAspect() const 
                    {
                        Point res(sz.x*pAsp.x, sz.y*pAsp.y);
                        ReducePair(res);
                        return res; 
                    }
    virtual  Point  PixelAspect() const { return pAsp; }

             Point& GetPixelAspect() { return pAsp; }
    protected:
             Point  pAsp;

                    FullDP();
};

// сравнение одной компоненты, если неравенство (!) не достигается,
// то ворачивает false
template<typename T>
bool CompareComponent(const T& e1, const T& e2, bool& cmp_res)
{
    bool res = true;
    if( e1 < e2 )
        cmp_res = true;
    else if( e2 < e1 )
        cmp_res = false;
    else
        res = false;

    return res;
}

#endif // #ifndef __MLIB_MISC_H__

