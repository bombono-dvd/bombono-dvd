//
// mconst.h
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

#ifndef __MCONST_H__
#define __MCONST_H__

// Различные константы

#include "mjpeg.h"
#include "mutils.h"

#include <mlib/const.h>


// myuvcontext.h

struct YuvInfo
{
    y4m_stream_info_t  streamInfo;
     y4m_frame_info_t  frameInfo;

                  YuvInfo() : isInit(false) {}
                  YuvInfo(const YuvInfo& info): isInit(false)
                  { Copy(info); }
      virtual    ~YuvInfo();

         YuvInfo& operator =(const YuvInfo& info)
                  {
                      Copy(info);
                      return *this;
                  }

            bool  IsInit() const { return isInit; }
   virtual  void  Clear();

            void  Copy(const YuvInfo& info);

   protected:

                bool  isInit; // установлены ли данные
};


// mmedia.h

// параметры видео
struct MovieInfo : public YuvInfo
{
    Point  ySz;
    Point  uSz;
    Point  vSz;

            void  Init() 
                  { isInit = true; }

    const  Point& Size() const { return ySz; }
                  // отношение (реальных) ширины к высоте
           Point  AspectRadio() const;
                  // соотношение сторон пикселя
           Point  PixelAspect() const;

};

#endif // #ifndef __MCONST_H__

