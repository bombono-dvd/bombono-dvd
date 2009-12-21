//
// mdemux/mpeg.h
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

#ifndef __MDEMUX_MPEG_H__
#define __MDEMUX_MPEG_H__

#include <arpa/inet.h> // htonX()
#include <iosfwd>      // std::string

#include <mlib/mlib.h>
#include <mlib/patterns.h>

#include "demuxconst.h"

namespace Mpeg
{

const uint32_t sidEND    = 0x1b9; // end sign
const uint32_t sidPACK   = 0x1ba; // pack header
const uint32_t sidSYSTEM = 0x1bb; // system header

const uint32_t sidPSM    = 0x1bc; // program stream map - используется для получения доп. информации
                                  // об элементарных потоках, представленной в виде списка дескрипторов
                                  // (descriptors)
const uint32_t sidPRIV1  = 0x1bd; // private stream 1 - используется в DVD для всех аудио и субтитров
const uint32_t sidPAD    = 0x1be; // padding stream
const uint32_t sidPRIV2  = 0x1bf; // private stream 2 - используется в DVD как NAV-пакеты

const uint32_t sidAUDIO_BEG = 0x1c0; // audio stream number range
const uint32_t sidAUDIO_END = 0x1e0; // 
const uint32_t sidVIDEO_BEG = sidAUDIO_END; // video stream number range
const uint32_t sidVIDEO_END = 0x1f0;         // 
 
const uint32_t sidECM    = sidVIDEO_END; // ECM stream
const uint32_t sidEMM    = 0x1f1; // EMM stream
const uint32_t sidDSMCC  = 0x1f2; // DSM-CC stream
const uint32_t sidS13522 = 0x1f3; // 13522 stream
const uint32_t sidH222_1 = 0x1f4; // ITU-T Rec. H.222.1
const uint32_t sidH222_1_E = 0x1f8; // ITU-T Rec. H.222.1 Type E
const uint32_t sidAUX    = 0x1f9; // ancillary stream
const uint32_t sid1SL    = 0x1fa; // 1_SL-packetized stream
const uint32_t sidFM     = 0x1fb; // 1_FlexMux stream

const uint32_t sidRESERV = 0x1fc;     // reserved data stream
const uint32_t sidRESERV_END = 0x1ff;
const uint32_t sidPSD    = 0x1ff; // program stream directory

const uint32_t sidFF     = 0xff;
const uint32_t sidPREFIX = 0x100;

class Demuxer;
class Service;

class DemuxState
{
    public:
        virtual      ~DemuxState() {}

                      /* Управление состояниями */
                      // переход в следующее состояние
        virtual void  NextState(Demuxer& dmx) = 0;
                      // заключительное состояние
        virtual bool  IsEnd() { return false; }

    protected:
                void  ChangeState(Demuxer& dmx, DemuxState& stt);

void  ChangeStateByCode(Demuxer& dmx);
void  ChangeStateByCode(uint32_t code, Demuxer& dmx);

// is_normal - не ошибка если в конце файла
bool  Read(void* buf, int cnt, Demuxer& dmx,  bool is_normal = false);
bool  ReadUint32(uint32_t& var, Demuxer& dmx, bool is_normal = false);
bool  ReadUint16(uint16_t& var, Demuxer& dmx);
// передвинуться от текущей позиции на len
void  SeekRel(int len, Demuxer& dmx);
// is_pts - PTS или SCR
void  SetTS(double ts, Demuxer& dmx, bool is_pts);
// в случае ошибки в потоке, reverse_cnt - насколько вернуться назад
void  InvalidState(Demuxer& dmx, int reverse_cnt = 0, const char* err_msg = 0);
};

// Begin
class BeginDemux: public DemuxState, public Singleton<BeginDemux>
{
    public:
        virtual void  NextState(Demuxer& dmx);
};

// End
class EndDemux: public DemuxState, public Singleton<EndDemux>
{
    public:
        virtual void  NextState(Demuxer&) { }
        virtual bool  IsEnd() { return true; }
};

// в случае ошибок всегда начинаем с пачки, чтоб SCR всегда был верным
class InvalidDemux: public DemuxState, public Singleton<InvalidDemux>
{
    public:
            struct Data
            {
                uint8_t  rstBuf[4];
                   char  rstSz;

                   Data(): rstSz(0) {}
            };

        virtual void  NextState(Demuxer& dmx);
};

// Pack Header
class PackDemux: public DemuxState, public Singleton<PackDemux>
{
    public:

        virtual void  NextState(Demuxer& dmx);
};

// System Header
class SystemDemux: public DemuxState, public Singleton<SystemDemux>
{
    public:

        virtual void  NextState(Demuxer& dmx);
};

// пропускаем данные 
class PaddingDemux: public DemuxState, public Singleton<PaddingDemux>
{
    public:

        virtual void  NextState(Demuxer& dmx);
};

// PES Packet
class PESDemux: public DemuxState, public Singleton<PESDemux>
{
    public:

        virtual void  NextState(Demuxer& dmx);
};

//////////////////////////////////////////////////////////////

class Demuxer: public InvalidDemux::Data
{
    public:
                Demuxer(io::stream* strm, Service* svc = 0);

                // Стандартный цикл:
                // for( dmx.Begin(); dmx.NextState(); )
                //     ;
          void  Begin();
                //
          bool  NextState();
          bool  IsEnd() { return dmxStt->IsEnd(); }

                // в строгом режиме ошибке в потоке не допускаются -> EndDemux
                // иначе в InvalideDemux -> и заново
          bool  IsStrict() { return isStrictMode; }
          void  SetStrict(bool is_strict) { isStrictMode = is_strict; }
                // 
          void  SetError(const char* error);
   const  char* ErrorReason() { return !errReason.empty() ? errReason.c_str() : 0 ; }

    io::stream& ObjStrm() { return *objStrm; }
          void  SetStream(io::stream* strm) { objStrm = strm; }

       Service* GetService() { return objSvc; }
          void  SetService(Service* svc) { objSvc = svc; }

        double  CurPTS() { return curPts; }
        double  CurSCR() { return curScr; }
      uint32_t  FilterCode() { return filterCode; }

    protected:

        io::stream* objStrm;
        DemuxState* dmxStt;
           Service* objSvc;

            double  curPts;
            double  curScr;
          uint32_t  filterCode; // устанавливается в случае objSvc->Filter(code) == true
                                // используется для хранения состояния в течение objSvc->GetData()

              bool  isStrictMode;
       std::string  errReason;

          void  ChangeState(DemuxState* stt);

          friend class DemuxState;
};

//////////////////////////////////////////////////////////////

class Service
{
    public:
        virtual      ~Service() {}

                      // нужна ли данная дорожка
        virtual bool  Filter(uint32_t /*code*/) { return false; }
                      // данные
        virtual void  GetData(Demuxer& /*dmx*/, int /*len*/) {}
                      // пройдена пачка sidPACK
        virtual void  OnPack(Demuxer& /*dmx*/) {}
};


class FirstVideoSvc: public Service
{
    public:

        virtual bool Filter(uint32_t code) { return code == sidVIDEO_BEG; }
};

bool IsAudioCode(uint32_t code);
bool IsVideoCode(uint32_t code);
inline bool IsContentCode(uint32_t code) { return IsAudioCode(code) || IsVideoCode(code); }

} // namespace Mpeg

#endif // __MDEMUX_MPEG_H__

