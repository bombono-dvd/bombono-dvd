//
// mdemux/trackbuf.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2009 Ilya Murav'jov
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

#ifndef __MDEMUX_TRACKBUF_H__
#define __MDEMUX_TRACKBUF_H__

#include <mlib/stream.h>

#include "demuxconst.h"

// буфер одной дорожки
class TrackBuf
{
    static const int optimalSize = STRM_BUF_SZ;
    public:

                TrackBuf();
               ~TrackBuf();

          char* Beg() { return begDat; }
          char* End() { return endDat; }

          void  Clear() { endDat = begDat; }
           int  Size() { return endDat - begDat; }
                // заполнен как только больше
          bool  IsFull() { return Size() >= optimalSize; }
          bool  IsEmpty() { return Size() <= 0; }

                // добавить в конец
          void  Append(const char* beg, const char* end);
          void  AppendFromStream(io::stream& strm, int len);

                // вырезать до текущей позиции и остаток сместить в начало
          void  CutStart(int off);

          // пока не нужны
//                 // установить новые данные
//           void  Set(const char* beg, const char* end);
//                 // отрезать с позиции
//           void  CutEnd(const char* pos);

          void  Reserve(int sz) { Extend(sz, false); }

    protected:

                    char* begDat; // начало и конец данных
                    char* endDat; //
                     int  bufSz;  //

          void  Extend(int new_sz, bool is_add);
};

#endif // __MDEMUX_TRACKBUF_H__

