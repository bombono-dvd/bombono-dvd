//
// myuvcontext.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007 Ilya Murav'jov
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

#ifndef __MYUVCONTEXT_H__
#define __MYUVCONTEXT_H__

#include <fcntl.h>

#include "mutils.h"
#include "mconst.h"


inline Point Divide(const Point& p1, const Point& p2)
{
    return Point(p1.x/p2.x, p1.y/p2.y);
}

struct MovieInfo;

namespace Planed {

struct YuvContext: public YuvInfo
{
    typedef YuvInfo MyParent;
    
   virtual       ~YuvContext();

   virtual  void  Clear();

                  // характеристики
             int  Width()  { return y4m_si_get_width(&streamInfo);  }
             int  Height() { return y4m_si_get_height(&streamInfo); }

                  // степени дискретизации
           Point  YQuant() { return Point(1, 1); }
           Point  UQuant() { return Divide(yPlane, uPlane); }
           Point  VQuant() { return Divide(yPlane, vPlane); }
                  // плоскости данных
           Plane& YPlane() { return yPlane; }
           Plane& UPlane() { return uPlane; }
           Plane& VPlane() { return vPlane; }


    protected:

                Plane  yPlane;
                Plane  uPlane;
                Plane  vPlane;
 
        unsigned char* yuv[3];


            bool  CheckRes(int yuv_res);
};

struct InYuvContext : public YuvContext
{
                  int  inFd;  // read from

                  InYuvContext(int in_fd) : inFd(in_fd) {}
                 ~InYuvContext()
                  {
//                       if( inFd != NO_HNDL )
//                           close(inFd);
                  }


                  // чтение данных
            bool  Begin();
            bool  GetFrame();

};

// Функции для чтения/записи MovieInfo
bool GetMovieInfo(MovieInfo& mi, const YuvContext& y_c);

struct OutYuvContext: public YuvContext
{
    private:
        typedef YuvContext MyParent;
    public:
                  int  outFd; // write to

                  OutYuvContext(int out_fd) : outFd(out_fd), isHdrWrit(false) {}

   virtual  void  Clear();
                  // инициализировать
            void  SetInfo(MovieInfo& mi);

                  // запись
            bool  PutFrame();


    protected:

        bool isHdrWrit; //
};

struct YuvContextIter
{
        PlaneIter  yIter;
        PlaneIter  uIter;
        PlaneIter  vIter;


            YuvContextIter(YuvContext& ycont) : 
                yIter(ycont.YPlane(), ycont.YQuant()),
                uIter(ycont.UPlane(), ycont.UQuant()),
                vIter(ycont.VPlane(), ycont.VQuant())
                {}

            YuvContextIter(Plane& y_p, Plane& u_p, Plane& v_p) : 
                yIter(y_p, Divide(y_p, y_p)),
                uIter(u_p, Divide(y_p, u_p)),
                vIter(v_p, Divide(y_p, v_p))
                {}


      void  XAdd()
            {
                ++yIter.xIter;
                ++uIter.xIter;
                ++vIter.xIter;
            }
      void  XSet(int x)
            {
                yIter.xIter.Set(x);
                uIter.xIter.Set(x);
                vIter.xIter.Set(x);
            }

      void  YAdd()
            {
                ++yIter.yIter;
                ++uIter.yIter;
                ++vIter.yIter;
            }
      void  YSet(int y)
            {
                yIter.yIter.Set(y);
                uIter.yIter.Set(y);
                vIter.yIter.Set(y);
            }
};

} // namespace Planed

#endif // #ifndef __MYUVCONTEXT_H__

