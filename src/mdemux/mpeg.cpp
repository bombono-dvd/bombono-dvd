//
// mdemux/mpeg.cpp
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

#include "mpeg.h"

#include <mlib/sdk/logger.h>
#include <mlib/sdk/memmem.h>

#include <math.h>

#include <boost/static_assert.hpp>

namespace Mpeg
{

#include PACK_ON
struct PackHeader
{
    uint32_t  scrData1;   // system clock reference
    uint16_t  scrData2;

    uint32_t  prgMuxRate; // program mux rate
};
#include PACK_OFF

const int PH_SIZE = 10;
BOOST_STATIC_ASSERT( PH_SIZE == sizeof(PackHeader) );

#include PACK_ON
struct SystemHeader
{
    uint16_t  hdrLen;
    uint32_t  bounds;     // rate_bound, audio_bound, fixed_flag, CSPS_flag

     uint8_t  videoBound; // system_audio_lock_flag, system_video_lock_flag, video_bound 
     uint8_t  prrf; // packet_rate_restriction_flag 
};
#include PACK_OFF

const int SH_SIZE = 8;
BOOST_STATIC_ASSERT( SH_SIZE == sizeof(SystemHeader) );

#include PACK_ON
struct PESHeader
{
    uint16_t  pctLen; // PES_packet_length
     uint8_t  flags1; //
     uint8_t  flags2; //
     uint8_t  hdrLen; // PES_header_data_length
};
#include PACK_OFF

const int PES_H_SIZE = 5;
BOOST_STATIC_ASSERT( PES_H_SIZE == sizeof(PESHeader) );

//////////////////////////////////////////////////////////////

Demuxer::Demuxer(io::stream* strm, Service* svc)
    : objStrm(strm), dmxStt(0), objSvc(svc), 
      curPts(INV_TS), curScr(INV_TS), filterCode(sidFF), isStrictMode(false)
{ 
    Begin();
}

void Demuxer::ChangeState(DemuxState* stt)
{
    dmxStt = stt;
}

void Demuxer::Begin()
{
    SetError(0);
    ChangeState(&BeginDemux::Instance());
}

bool Demuxer::NextState()
{
    dmxStt->NextState(*this);
    return !dmxStt->IsEnd();
}

void Demuxer::SetError(const char* err)
{
    if( !err )
    {
        errReason.clear();
        return;
    }
    errReason.assign(err);
}

void DemuxState::ChangeState(Demuxer& dmx, DemuxState &stt)
{
    dmx.ChangeState(&stt);
}

bool DemuxState::Read(void* buf, int cnt, Demuxer& dmx, bool is_normal)
{
    int r_cnt = dmx.ObjStrm().raw_read((char*)buf, cnt);
    if( cnt != r_cnt )
    {
        if( r_cnt == 0 && is_normal )
        {
            ChangeStateByCode(sidEND, dmx);
            return false;
        }

        dmx.SetError("Read error!");
        ChangeState(dmx, EndDemux::Instance());
        return false;
    }
    return true;
}

bool DemuxState::ReadUint32(uint32_t& var, Demuxer& dmx, bool is_normal)
{
    bool res = Read(&var, 4, dmx, is_normal);
    if( res )
        var = htonl(var);
    return res;
}

bool DemuxState::ReadUint16(uint16_t& var, Demuxer& dmx)
{
    bool res = Read(&var, 2, dmx);
    if( res )
        var = htons(var);
    return res;
}

void DemuxState::ChangeStateByCode(Demuxer& dmx)
{
    uint32_t code;
    if( !ReadUint32(code, dmx, true) )
        return;

    ChangeStateByCode(code, dmx);
}

static void LogByStartCode(uint32_t code)
{
    if( !LogFilter->IsEnabled(::Log::Debug) )
        return;

    if( (code >= sidAUDIO_BEG && code < sidAUDIO_END) )
    {
        LOG_DBG2 << "\t" << "audio " << code-sidAUDIO_BEG << io::endl;
        return;
    }

    if( (code >= sidVIDEO_BEG && code < sidVIDEO_END) )
    {
        LOG_DBG2 << "\t" << "video " << code-sidVIDEO_BEG << io::endl;
        return;
    }

    switch( code )
    {
    case sidEND:
        LOG_DBG << "=============== End ===============" << io::endl;
        break;
    case sidPACK:
        LOG_DBG2 << "=============== Pack ===============" << io::endl;
        break;
    case sidSYSTEM:
        LOG_DBG << "system header" << io::endl;
        break;
    case sidPSM: 
        LOG_DBG << "\t" << "program stream map" << io::endl;
        break;
    case sidPAD: 
        LOG_DBG2 << "\t" << "padding stream" << io::endl;
        break;
    case sidPRIV1:
        LOG_DBG << "\t" << "private stream 1" << io::endl;
        break;
    case sidPRIV2:
        LOG_DBG << "\t" << "private stream 2 (nav)" << io::endl;
        break;
    case sidECM:
        LOG_DBG << "\t" << "ECM stream" << io::endl;
        break;
    case sidEMM:
        LOG_DBG << "\t" << "EMM stream" << io::endl;
        break;
    case sidPSD:
        LOG_DBG << "\t" << "program stream dir" << io::endl;
        break;
    case sidDSMCC:
        LOG_DBG << "\t" << "DSMCC stream" << io::endl;
        break;
    case sidH222_1_E:
        LOG_DBG << "\t" << "H222_1_E stream" << io::endl;
        break;
    default:
        LOG_DBG << "\t" << "packet with code: " << std::hex << code << std::dec << io::endl;
        break;
    }
}

bool IsAudioCode(uint32_t code)
{
    return (code >= sidAUDIO_BEG && code < sidAUDIO_END);
}

bool IsVideoCode(uint32_t code)
{
    return (code >= sidVIDEO_BEG && code < sidVIDEO_END);
}

void DemuxState::ChangeStateByCode(uint32_t code, Demuxer& dmx)
{
    LogByStartCode(code);

    // 0 проверка
    if( (code&~sidFF) != sidPREFIX )
    {
        LOG_INF << "Bad start code: " << code << "!" << io::endl;
        InvalidState(dmx);
        return;
    }

    // 1 фильтрация
    DemuxState* pes_ds = &PaddingDemux::Instance();
    if( Service* svc = dmx.GetService() )
    {
        if( svc->Filter(code) )
        {
            dmx.filterCode = code;
            pes_ds         = &PESDemux::Instance();
        }
    }

    // 2 переход
    
    // стандартное аудио/видео
    if( IsContentCode(code) )
    {
        ChangeState(dmx, *pes_ds);
        return;
    }

    // остальные случаи
    switch( code )
    {
    case sidEND:
        // в нестрогом режиме считаем, что конец наступает только когда мы в конце файла
        ChangeState(dmx, dmx.IsStrict() ? (DemuxState&)EndDemux::Instance() : InvalidDemux::Instance() );
        break;
    case sidPACK:
        ChangeState(dmx, PackDemux::Instance());
        break;
    case sidPSM: 
    case sidPAD: 
    case sidPRIV2:
    case sidECM:
    case sidEMM:
    case sidPSD:
    case sidDSMCC:
    case sidH222_1_E:
        ChangeState(dmx, PaddingDemux::Instance());
        break;
    case sidPRIV1:
        ChangeState(dmx, *pes_ds);
        break;
    //case sidSYSTEM: // только после sidPACK может быть
    default:
        InvalidState(dmx);
        break;
    }
}

void DemuxState::SeekRel(int len, Demuxer& dmx)
{
    dmx.ObjStrm().seekg(len, iof::cur);
}

void DemuxState::SetTS(double ts, Demuxer& dmx, bool is_pts)
{
    if( is_pts )
    {
        dmx.curPts = ts;
        if( ts != INV_TS )
            LOG_DBG << "\t pts: " << ts << " (s)" << io::endl;
    }
    else
    {
        dmx.curScr = ts;
        if( ts != INV_TS )
            LOG_DBG2 << "\t scr: " << ts << " (s)" << io::endl;
    }
}

void DemuxState::InvalidState(Demuxer& dmx, int reverse_cnt, const char* err_msg)
{
    if( dmx.IsStrict() )
    {
        dmx.SetError( err_msg ? err_msg : "Not MPEG file" );
        ChangeState(dmx, EndDemux::Instance());
    }
    else
    {
        if( reverse_cnt )
            SeekRel(-reverse_cnt, dmx);
        ChangeState(dmx, InvalidDemux::Instance());
    }
}

/////////////////////////////////////////////////////////////////////

void BeginDemux::NextState(Demuxer& dmx)
{
    SetTS(INV_TS, dmx, false);
    SetTS(INV_TS, dmx, true);

    uint32_t code;
    if( !ReadUint32(code, dmx) )
        return;
    if( code != sidPACK )
    {
        InvalidState(dmx);
        return;
    }

    ChangeState(dmx, PackDemux::Instance());
}

void InvalidDemux::NextState(Demuxer& dmx)
{
    uint8_t pat[4] = { 0, 0, 1, 0xba }; // = sidPACK

    char buf[1024];
    Data& dat = dmx;
    memcpy(buf, dat.rstBuf, dat.rstSz);
    char* cur = buf+dat.rstSz;

    int read_cnt = 2;
    for( int cnt, shift; shift=cur-buf, cnt = dmx.ObjStrm().raw_read(cur, ARR_SIZE(buf)-shift),
         cnt; ) // пока cnt не нуль
    {
        char* end = cur+cnt;
        if( char* p = (char*)MemMem((ucchar_t*)buf, shift+cnt, pat, 4) )
        {
            SeekRel(p-end+4, dmx);
            ChangeState(dmx, PackDemux::Instance());

            dat.rstSz = 0;
            return;
        }

        // старт-код может быть разделен границей чтения
        char new_sh = std::min(int(end-buf), 3);
        if( --read_cnt )
        {
            memmove(buf, end-new_sh, new_sh);
            cur = buf + new_sh;
        }
        else
        {
            dat.rstSz = new_sh;
            memcpy(dat.rstBuf, end-new_sh, new_sh);
            return;
        }
    }
    // все прочитали, но не нашли
    ChangeState(dmx, EndDemux::Instance());
}

inline double TimestampToSec(double ts)
{
    return ts/90000.;
}

void PackDemux::NextState(Demuxer& dmx)
{
    PackHeader ph;
    if( !Read(&ph, PH_SIZE, dmx) )
        return;

    uint32_t raw_scr = ntohl(ph.scrData1);
    // :TODO: реализовать MPEG1
    // в MPEG2 это маркерный бит; отсутствие - признак MPEG1 
    if( !(raw_scr & 0x04000000) )
    {
        InvalidState(dmx, PH_SIZE, "MPEG1 is not implemented yet");
        return;
    }

    uint8_t* p_scr = (uint8_t*)&ph.scrData1;
    uint64_t scr_bas = 0;
    scr_bas |= (p_scr[0] & 0x38) << 27;
    scr_bas |= (p_scr[0] & 0x03) << 28;
    scr_bas |=  p_scr[1]         << 20;
    scr_bas |= (p_scr[2] & 0xf8) << 12;
    scr_bas |= (p_scr[2] & 0x03) << 13;
    scr_bas |=  p_scr[3]         <<  5;
    scr_bas |= (p_scr[4] & 0xf8) >>  3;

    uint16_t scr_ext = 0;
    scr_ext |= (p_scr[4] & 0x03) << 7;
    scr_ext |= (p_scr[5] & 0xfe) >> 1;

    // проверка неиспорченности scr
    if( (p_scr[0] & 0xc0) != 0x40 || 
        !(p_scr[0] & 0x04) || !(p_scr[2] & 0x04) ||
        !(p_scr[4] & 0x04) || !(p_scr[5] & 0x01)    )
    {
        InvalidState(dmx, PH_SIZE);
        return;
    }

    double scr = TimestampToSec(scr_bas + scr_ext/300.);
    SetTS(scr, dmx, false);

    int var = ntohl(ph.prgMuxRate);
    int mux_rate = (var >> 10) * 50; // bitrate, b/s
    
    LOG_DBG2 << "Bitrate (in pack): " << mux_rate << " b/s" << io::endl;
    if( Service* svc = dmx.GetService() )
        svc->OnPack(dmx);

    var &= 7; // последние 3 бита
    SeekRel(var, dmx);

    uint32_t code;
    if( !ReadUint32(code, dmx) )
        return;
    if( code == sidSYSTEM )
    {
        LogByStartCode(code);
        ChangeState(dmx, SystemDemux::Instance());
        return;
    }

    ChangeStateByCode(code, dmx);
}

void SystemDemux::NextState(Demuxer& dmx)
{
    SystemHeader sh;
    if( !Read(&sh, SH_SIZE, dmx) )
        return;

    uint8_t* buf = (uint8_t*)&sh.bounds;
    if( !(buf[0] & 0x80) || !(buf[2] & 0x01) )
    {
        InvalidState(dmx, SH_SIZE);
        return;
    }

    // читаем сколько сказано в hdrLen
    int len = ntohs(sh.hdrLen) - (SH_SIZE - 2);
    SeekRel(len, dmx);
    
    ChangeStateByCode(dmx);
}

void PaddingDemux::NextState(Demuxer& dmx)
{
    uint16_t len;
    if( !ReadUint16(len, dmx) )
        return;
    SeekRel(len, dmx);

    ChangeStateByCode(dmx);
}

void PESDemux::NextState(Demuxer& dmx)
{
    PESHeader pes_h;
    if( !Read(&pes_h, PES_H_SIZE, dmx) )
        return;
    int at = PES_H_SIZE-2; // не включая само поле pes_h.pctLen

    // читаем заголовок пакета
    if( (pes_h.flags1 & 0xc0) != 0x80 )
    {
        InvalidState(dmx, PES_H_SIZE);
        return;
    }
    // проверка на зашифрованность
    if( (pes_h.flags1 & 0x30) != 0x00 )
    {
        InvalidState(dmx, PES_H_SIZE, "File is scrambled (protected)");
        return;
    }

    // PTS
    {
        int hdr_at = 0;
        if( pes_h.flags2 & 0x80 ) // есть PTS
        {
            uint8_t dat[5];
            if( !Read(dat, ARR_SIZE(dat), dmx) )
                return;
            hdr_at += ARR_SIZE(dat);

            uint64_t pts = 0;
            pts |= (dat[0] & 0x0E) << 29;
            pts |=  dat[1]         << 22;
            pts |= (dat[2] & 0xFE) << 14;
            pts |=  dat[3]         <<  7;
            pts |= (dat[4] & 0xFE) >>  1;

            // проверка неиспорченности pts
            uint8_t c;
            if( !(c = dat[0] & 0x31, (c == 0x21 || c == 0x31)) ||
                !(dat[2] & 1) || !(dat[4] & 1) )
            {
                InvalidState(dmx, PES_H_SIZE+hdr_at);
                return;
            }

            SetTS(TimestampToSec(pts), dmx, true);
        }

        SeekRel(pes_h.hdrLen - hdr_at, dmx);
    }

    at += pes_h.hdrLen;
    int len = htons(pes_h.pctLen) - at;
    std::streampos next_pos = dmx.ObjStrm().tellg() + std::streampos(len);

    // данные пакета
    if( Service* svc = dmx.GetService() ) 
        svc->GetData(dmx, len);

    dmx.ObjStrm().seekg(next_pos);
    ChangeStateByCode(dmx);
}

} // namespace Mpeg

