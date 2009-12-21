//
// mmpeg_demux.h
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

#ifndef __MMPEG_DEMUX_H__
#define __MMPEG_DEMUX_H__

#include <mdemux/mpeg.h>

#include "mdemux.h"

class MpegSeqDemuxer: public SeqDemuxer, public Mpeg::Service
{
    public:
                MpegSeqDemuxer(int track_num = 0);

                // SeqDemuxer
  virtual void  Init(io::stream* strm, TrackBuf* buf);
  virtual bool  Demux(MpegPlayer&) {  return implDmx.NextState(); }

                // Mpeg::Service
  virtual bool  Filter(uint32_t code) { return code == Mpeg::sidVIDEO_BEG + trkIdx; }
  virtual void  GetData(Mpeg::Demuxer& dmx, int len);

    protected:
                Mpeg::Demuxer  implDmx;
                          int  trkIdx;
                     TrackBuf* trkBuf;
};

class BufDemuxer: public SeqDemuxer
{
    public:

        virtual    bool  Demux(MpegPlayer& plyr);

    protected:

	    char rawBuf[STRM_BUF_SZ];

        virtual     int  Demux(uint8_t* beg, uint8_t* end, MpegPlayer& plyr) = 0;
};

// "тождественный" демиксер, т.е. вход=выход
// Пример: ".m2v"
class IDemuxer : public BufDemuxer
{
    using BufDemuxer::Demux;
    public:

        virtual     int  Demux(uint8_t* beg, uint8_t* end, MpegPlayer& plyr);
};

// старый код демиксера Mpeg1/2
namespace Mpeg_legacy 
{

///////////////////////////////////////////
// Стандарт MPEG(1,2)

// номера дорожек в потоке
const int mpegFIRST_TRACK = 0xe0;
const int mpegLAST_TRACK  = 0xef;

// номера программ в TS-потоке
const int mpegFIRST_PRG = 0x10;
const int mpegLAST_PRG  = 0x1ffe;

// размер транспортного (TS) пакета
const int mpegTS_PACKET_SZ = 188;

///////////////////////////////////////////

// MPEG-демиксер уровня программ (PS, ".mpg")
class MpegDemuxer: public BufDemuxer
{
    enum DemuxState
    {
        dsDEMUX_HEADER,
        dsDEMUX_DATA,
        dsDEMUX_SKIP
    };
    public:
    using BufDemuxer::Demux;


                       // для удобства номер дорожки отсчитываем с нуля
                       MpegDemuxer(int track_num = 0);

      virtual     int  Demux(uint8_t* beg, uint8_t* end, MpegPlayer& plyr);

    protected:

                // номер потока в MPEG PS (Program Stream), 
                // в пределах 0xe0-0xef
           int  trackNum;
                // номер "программы" в MPEG TS (Transport Stream)
                // в пределах 0x10-0x1ffe
                // :TODO: выделить (пока если ноль, то не учитываем)
           int  tsPid;
                // текущее состояние демиксера
    DemuxState  dmxState;

                // :TODO: узнать, что это такое и зачем нужно
           int  state_bytes;
       uint8_t  head_buf[264];


                       // реализация демиксирования
                       // flags - только для декодирования TS-потоков
                  int  DoDemux(uint8_t * beg, uint8_t * end, int flags, MpegPlayer& plyr);
};

// MPEG-демиксер транспортного уровня  (TS, ".dva")
class TSMpegDemuxer: public MpegDemuxer
{
    static const int BufSize = STRM_BUF_SZ+mpegTS_PACKET_SZ;
    public:
    using MpegDemuxer::Demux;

                       // первый попавшийся канал и будет использоваться
                       TSMpegDemuxer();
                       // в пределах 0x10-0x1ffe
                       TSMpegDemuxer(int ts_pid);

                       // flags - только для декодирования TS-потоков
      virtual     int  Demux(uint8_t* beg, uint8_t* end, MpegPlayer& plyr);


    protected:

                    
            uint8_t  tsBuf[BufSize]; // локальный буфер
            uint8_t* curPos;         // указатель на обрабатываемые данные в нем
               bool  isFirstIn;      // используем первый попавшийся канал

};

} // namespace Mpeg_legacy 

#endif // #ifndef __MMPEG_DEMUX_H__

