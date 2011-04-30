//
// mdemux/decoder.h
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

#ifndef __MDEMUX_DECODER_H__
#define __MDEMUX_DECODER_H__

#include <mlib/tech.h>

#include <mlib/stream.h>
#include "demuxconst.h"

#ifdef CONFIG_GPL

////////////////////////////////////////////////
// Включение libmpeg
// 
// так как libmpeg2 не озаботился наличием С++,
// то приходится обрамлять самим
#include <inttypes.h>
C_LINKAGE_BEGIN
#include <../libs/mpeg2dec/include/mpeg2.h>
#include <../libs/mpeg2dec/include/mpeg2convert.h>
C_LINKAGE_END
////////////////////////////////////////////////

typedef struct mpeg2dec_s mpeg2dec_t;

#else

typedef void mpeg2dec_t;

#endif // !CONFIG_GPL

namespace Mpeg {

struct Chunk
{
    io::pos  extPos;
        int  len;

        Chunk(io::pos pos, int l) : extPos(pos), len(l) {}
};

struct FrameData;
}


enum FrameDecType
{
    fdtCURRENT,
    fdtLEFT,
    fdtRIGHT
};

enum FrameOutputFrmt
{
    fofYCBCR,       // YCbCr - родной для MPEG2
    fofRGB,         // RGB   - 24bit, в формате Gdk::Pixbuf
    fofRGBA         // RGBA  - 32bit, --||--
};

class MpegDecodec
{
    public:

    typedef unsigned char* const* PlanesType;

                        MpegDecodec();
                       ~MpegDecodec();
        
                        // сама картинка - 3 массива - Y, Cb, Cr
            PlanesType  Planes();

                        /* для VideoLine */ 

                        // full_reset - mpeg2_reset(1)
                  void  Init(bool full_reset);
                  bool  IsInit();
                        // формат выходных буферов
                  void  SetOutputFormat(FrameOutputFrmt typ);
       FrameOutputFrmt  GetOutputFormat() { return fofTyp; }
                        //
                  void  ReadForInit(const Mpeg::Chunk& cnk, io::stream& strm);
                        // Сессия выглядит так:
                        //   SetOutputFormat(typ);
                        //   ReadBegin();
                        //   ReadFrame(); ReadFrame(); ...
                  void  ReadBegin();
                  void  ReadFrame(const Mpeg::FrameData& fram, io::stream& strm);
                        // Подробности в "Как mpeg2dec_t хранит результаты"
                        // ..PBBBBP..
                        //   l c  r
            PlanesType  FrameData(FrameDecType fdt) const;

    protected:

        mpeg2dec_t* mpeg2Dec;
   FrameOutputFrmt  fofTyp;

void ReadPrefix();

    private:
                bool seqFound;
                bool picFound;
                bool picEnd;
void ClearFlags();
void PlayMpeg();
char* DoReadForInit(char* buf, int buf_len);
char* DoReadFrame(char* buf, int buf_len);
void SetFormat();
};

//void operator << (MpegDecodec& m2d, std::pair<const Mpeg::Chunk, io::stream*> chk);


#endif // #ifndef __MDEMUX_DECODER_H__

