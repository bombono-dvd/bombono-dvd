//
// mutils.h
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

#ifndef __MUTILS_H__
#define __MUTILS_H__

#include <mlib/sdk/misc.h>

// Planed - вещи, связанные с плоскостями/2D
namespace Planed {

// плоскость вместе с размерами
struct Plane: public Point
{
  unsigned char* data;

             Plane(): data(0) {}
             Plane(int wdh, int hgt, unsigned char* d)
                : Point(wdh, hgt), data(d) { }

        int  Size() { return x*y; }
};


// итератор по одной координате
struct CoordIter
{
      int  quant;  // размер дискретизации
      int  idx;    // 
      int  off;    // <quant

                CoordIter(int q): quant(q), idx(0), off(0) {}

            int Idx() { return idx; }

     CoordIter& operator++()
                {
                    off++;
                    if( off>=quant )
                    {
                        idx++;
                        off = 0;
                    }
                    return *this;
                }

     CoordIter& operator--()
                {
                    off--;
                    if( off<= 0 )
                    {
                        idx--;
                        off = quant-1;
                    }
                    return *this;
                }

          void  Set(int x)
                {
                    idx = x/quant;
                    off = x%quant;
                }
           int  Get() { return idx*quant+off; }

};

// итератор плоскости
struct PlaneIter
{
         Plane& plane;
     CoordIter  xIter;
     CoordIter  yIter;

                      PlaneIter( Plane& p, Point qp ): 
                          plane(p), xIter(qp.x), yIter(qp.y) {}
                      // получить данные по координатам (xIter, yIter)
      unsigned  char  constRes() { return plane.data[ xIter.Idx() + plane.x*yIter.Idx() ]; }
      unsigned  char& Res() { return plane.data[ xIter.Idx() + plane.x*yIter.Idx() ]; }
};

}


#endif // #ifndef __MUTILS_H__

