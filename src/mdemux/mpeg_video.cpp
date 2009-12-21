//
// mdemux/mpeg_video.cpp
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

#include <boost/static_assert.hpp>

#include <mlib/sdk/logger.h>
#include <mlib/sdk/memmem.h>
#include <mlib/sdk/misc.h>

#include "mpeg_video.h"
#include "decoder.h"

#include <stdexcept> // std::runtime_error()

namespace Mpeg {

Decoder::Decoder(): dcrStt(0), datPos(0), vdSvc(0)
{
    Init();
}

void Decoder::Begin(io::pos dat_pos)
{
    if( dat_pos != -1 )
        datPos = dat_pos;
    inpBuf.Clear();
    PicHeaderDecode::Instance().ClearState(*this, false);

    // начинаем с поиска заголовка HeaderDecode (все по-серьезному :) )
    NextDecode& nstt = NextDecode::Instance();
    nstt.SetState(HeaderDecode::Instance(), *this);

    ChangeState(&nstt);
}

void Decoder::Init()
{
    seqInf.Init();
    Begin(0);
}

void Decoder::ChangeState(DecodeState* stt)
{
    dcrStt = stt;
}

// сбросить данные с начала, размером skip_cnt
void Decoder::DataSkip(int skip_cnt)
{
    datPos += skip_cnt;
    inpBuf.CutStart(skip_cnt);
}

static char TFChar(bool b)
{
    return b ? 't' : 'f';
}

static void LogVideoTag(Decoder& dcr, VideoTag tag)
{
    if( !LogFilter->IsEnabled(Log::Debug) )
        return;

    switch( tag )
    {
    case vtGOP:
        LOG_DBG << io::endl;
        LOG_DBG << "GOP" << io::endl;
        break;
    case vtFRAME_FOUND:
        {
            Picture& pic = dcr.pic;
            std::stringstream log_ss;
            const char* type_names[] = { "Forbidden!", "I", "P", "B", "D", "Reserved!" };

            log_ss << type_names[pic.type] << " ";
            log_ss << char('a'+pic.structTyp);
            log_ss << TFChar(pic.tff) << TFChar(pic.rff) << TFChar(pic.progrFrame) << io::endl;

            LOG_DBG << log_ss.str();
        }
        break;
    default:
        break;
    }
}

// сбросить данные с указанием принадлежности
void Decoder::DataSkipTag(VideoTag tag, int skip_cnt)
{
    if( (tag == vtHEADER) || (tag == vtGOP) )
        PicHeaderDecode::Instance().ClearState(*this);

    LogVideoTag(*this, tag);

    if( tag != vtNONE && vdSvc )
        vdSvc->TagData(*this, tag);
    DataSkip(skip_cnt);
}

void Decoder::TagError()
{
    DataSkipTag(vtERROR, 0);
}

void Decoder::SetFrame(Picture& p)
{
    pic = p;
    DataSkipTag(vtFRAME_FOUND, 0);
}

io::pos Decoder::DatPosForTag(VideoTag tag)
{
    if( tag != vtFRAME_FOUND )
        return DatPos();

    ASSERT( pic.IsInit() );
    return pic.datPos;
}


void DecodeState::ChangeState(DecodeState& stt, Decoder& dcr)
{
    dcr.ChangeState(&stt);
}

bool DecodeState::HaveBytes(int cnt, Decoder& dcr)
{
    if( dcr.DataSize() >= cnt )
        return true;

    BufferDecode& stt = BufferDecode::Instance();
    stt.NeedBytes(cnt, this, dcr);

    ChangeState(stt, dcr);
    return false;
}

void DecodeState::ChangeWithNextState(DecodeState& stt, Decoder& dcr)
{
    NextDecode& nd_stt = NextDecode::Instance();
    nd_stt.SetState(stt, dcr);

    ChangeState(nd_stt, dcr);
}

void DecodeState::SkipState(DecodeState& after_stt, Decoder& dcr, int skip_bytes)
{
    if( skip_bytes )
        dcr.DataSkip(skip_bytes);

    SkipDecode& sk_stt = SkipDecode::Instance();
    sk_stt.SetState(after_stt, dcr);

    ChangeState(sk_stt, dcr);
}

void DecodeState::InvalidState(Decoder& dcr, int skip_bytes)
{
    dcr.DataSkipTag(vtERROR, std::min(skip_bytes, dcr.DataSize()));

    ChangeState(SeekDecode::Instance(), dcr);
}

/////////////////////////////////////////////////////////////////////////////
// Состояния

void EndDecode::NextState(Decoder& dcr)
{
    if( !HaveBytes(4, dcr) )
        return;
    dcr.DataSkipTag(vtEND, 4);
    ChangeWithNextState(HeaderDecode::Instance(), dcr); // заново
}

void BufferDecode::NeedBytes(int cnt, DecodeState* for_stt, Decoder& dcr)
{
    Data& dat = dcr;

    dat.bytNeed = cnt;
    dat.nxtStt  = for_stt;
}

void BufferDecode::NextState(Decoder& dcr)
{
    Data& dat = dcr;

    ASSERT( dat.nxtStt );
    if( dcr.DataSize() >= dat.bytNeed )
    {
        ChangeState(*dat.nxtStt, dcr);
        dat.nxtStt = 0;
    }
}

void CommonSkipDecode::Complete(Decoder& dcr)
{
    if( !HaveBytes(4, dcr) )
        return;
    uint8_t* beg = dcr.Data(0);

    Data& dat = dcr;
    uint32_t code = dat.nxtStt->Code();
    if( code != vidSEQ_ERR )
        if( beg[3] != (code&vidFF) )
        {
            InvalidState(dcr);
            return;
        }

    ChangeState(*dat.nxtStt, dcr);
    return;
}

void CommonSkipDecode::SetState(DecodeState& stt, Decoder& dcr)
{
    Data& dat = dcr;
    dat.nxtStt = &stt;
}

void NextDecode::NextState(Decoder& dcr)
{
    if( !HaveBytes(4, dcr) )
        return;

    int cnt = dcr.DataSize(); // >= 4
    uint8_t* beg = dcr.Data(0);
    for( uint8_t* cur = beg, *end = beg+cnt; cur<end; cur++ )
        if( *cur )
        {
            int shift = cur-beg;
            // проверяем на 0x000001
            if( (*cur != 1) || shift<2 )
            {
                dcr.DataSkip(shift+1);
                InvalidState(dcr, 0);
                return;
            }

            dcr.DataSkip(shift-2);
            Complete(dcr);
            return;
        }
    // все нули
    dcr.DataSkip(cnt-2);
}

void SkipDecode::NextState(Decoder& dcr)
{
    if( !HaveBytes(4, dcr) )
        return;
    //uint8_t pat[3] = { 0, 0, 1 }; // префикс

    int cnt = dcr.DataSize(); // >= 4
    uint8_t* beg = dcr.Data(0);
    if( uint8_t* p = (uint8_t*)MemPat3(beg, cnt, 0x100) )
    {
        dcr.DataSkip(p-beg);
        Complete(dcr);
        return;
    }
    else
        dcr.DataSkip(cnt-2);
}

void SeekDecode::NextState(Decoder& dcr)
{
    if( !HaveBytes(4, dcr) )
        return;
    uint8_t pat[4] = { 0, 0, 1, 0xb3 }; // = vidSEQ_HDR

    int cnt = dcr.DataSize(); // >= 4
    uint8_t* beg = dcr.Data(0);
    if( dcr.seqInf.IsInit() )
    {
        if( uint8_t* p = (uint8_t*)MemMem(beg, cnt, pat, 3) )
        {
            dcr.DataSkip( p-beg );
            p = dcr.Data(0);

            if( !HaveBytes(4, dcr) )
                return;
            switch( p[3] )
            {
            case (vidFF & vidPIC):
                ChangeState(PicHeaderDecode::Instance(), dcr);
                return;
            case (vidFF & vidSEQ_HDR):
                ChangeState(HeaderDecode::Instance(), dcr);
                return;
            case (vidFF & vidSEQ_END):
                ChangeState(EndDecode::Instance(), dcr);
                return;
            case (vidFF & vidGOP):
                ChangeState(GOPDecode::Instance(), dcr);
                return;
            }
            // неподходящий префикс
            dcr.DataSkip( 4 );
        }
        else
            dcr.DataSkip( cnt-2 );
    }
    else
    {
        // только заголовок подойдет
        if( uint8_t* p = (uint8_t*)MemMem(beg, cnt, pat, 4) )
        {
            dcr.DataSkip( p-beg );
            ChangeState(HeaderDecode::Instance(), dcr);
            return;
        }
        else
            dcr.DataSkip( cnt-3 );
    }
}

#include PACK_ON
struct SeqHeader
{
    uint32_t  seqStartCode; // = vidSEQ_HDR
     uint8_t  szDat[3];     // sizes

     uint8_t  inf[5]; // frame rate, bit rate, ...
};
#include PACK_OFF

const int SH_SIZE = 12;
BOOST_STATIC_ASSERT( SH_SIZE == sizeof(SeqHeader) );

#include PACK_ON
struct ExtSeqHeader
{
    uint32_t  extStartCode; // = vidEXT
     uint8_t  inf[6];
};
#include PACK_OFF

const int EXTH_SIZE = 10;
BOOST_STATIC_ASSERT( EXTH_SIZE == sizeof(ExtSeqHeader) );

void HeaderDecode::NextState(Decoder& dcr)
{
    int hdr_size = SH_SIZE;
    if( !HaveBytes(hdr_size, dcr) )
        return;

    SeqHeader& hdr = *(SeqHeader*)dcr.Data(0);
    Data& dat = dcr;

    // 1 параметры
    int t = (hdr.szDat[0] << 16) | (hdr.szDat[1] << 8) | hdr.szDat[2];
    dat.wdh = t >> 12;
    dat.hgt = t & 0xfff;
    if( !(dat.wdh && dat.hgt) )
    {
        InvalidState(dcr);
        return;
    }

    dat.sarCode = hdr.inf[0] >> 4;
    dat.framRatCode = hdr.inf[0] & 0xf;

    dat.bytRat = (hdr.inf[1] << 10) | (hdr.inf[2] << 2) | (hdr.inf[3] >> 6);
    if( !(hdr.inf[3] & 0x20) ) // marker bit
    {
        InvalidState(dcr);
        return;
    }
    dat.vbvBufSz  = (hdr.inf[3] && 0x1f) << 5;
    dat.vbvBufSz |=  hdr.inf[4]          >> 3;

    // 2 матрицы
    if( hdr.inf[4] & 2 ) //  intra quantiser matrix
    {
        hdr_size += 64;
        if( !HaveBytes(hdr_size, dcr) )
            return;
    }

    if( *dcr.Data(hdr_size-1) & 1 ) // non-intra quantiser matrix
    {
        hdr_size += 64;
        if( !HaveBytes(hdr_size, dcr) )
            return;
    }

    dcr.DataSkipTag(vtHEADER, hdr_size);
    ChangeWithNextState(ExtHeaderDecode::Instance(), dcr);
}

static uint32_t GetStartCode(Decoder& dcr)
{
    return htonl(*(uint32_t*)dcr.Data(0));
}

// получить идентификатор расширения
static int GetExtId(Decoder& dcr)
{
    return *dcr.Data(4) >> 4;
}

void ExtHeaderDecode::NextState(Decoder& dcr)
{
    if( !HaveBytes(EXTH_SIZE, dcr) )
        return;

    ExtSeqHeader& hdr = *(ExtSeqHeader*)dcr.Data(0);
    // :TODO: пока только MPEG2 парсим
    //ASSERT_RTL( vidEXT == htonl(hdr.extStartCode) );
    if( vidEXT != htonl(hdr.extStartCode) )
        throw std::runtime_error("MPEG1 Video is not implemented yet!");

    if( extSEQ != GetExtId(dcr) )
    {
        InvalidState(dcr);
        return;
    }

    // запись параметров и проверка того, что данные совпадают с пред.заголовком
    HeaderDecode::Data& dat = (HeaderDecode::Data&)dcr;
    dat.plId  = (hdr.inf[0] & 0x0f) << 4;
    dat.plId |=  hdr.inf[1]         >> 4;

    dat.isProgr = hdr.inf[1] & 0x08;

    dat.chromaFrmt = (hdr.inf[1] & 0x07) >> 1;

    int wdh_ext = (hdr.inf[1] & 0x01) << 1;
    wdh_ext    |=  hdr.inf[2]         >> 7;
    dat.wdh += (wdh_ext << 12);

    int hgt_ext = (hdr.inf[2] & 0x60) << (12-5);
    dat.hgt += hgt_ext;

    dat.bytRat |= ((hdr.inf[2] << 25) | (hdr.inf[3] << 17)) & 0x3ffc0000; // 30 бит
    if( !(hdr.inf[3] & 1) ) // marker bit
    {
        InvalidState(dcr);
        return;
    }

    dat.vbvBufSz |= hdr.inf[4] << 10;
    dat.lowDelay  = hdr.inf[5] & 0x80;

    dat.framRatN = (hdr.inf[5] & 0x60) >> 5;
    dat.framRatD =  hdr.inf[5] & 0x1f;

    dcr.DataSkip(EXTH_SIZE);
    // установка/проверка последовательности
    if( !SetSeqData(dcr) )
      return;

    ExtUserDecode& eu_stt = ExtUserDecode::Instance();
    eu_stt.SetEUType(0, dcr);

    ChangeWithNextState(eu_stt, dcr);
}

bool SequenceData::operator ==(const SequenceData& sd)
{
    bool res = true;

    res = res && (wdh == sd.wdh);
    res = res && (hgt == sd.hgt);
    res = res && (sarCode == sd.sarCode);
    res = res && (framRat == sd.framRat);
    // судя по mpeg2dec, не все DVD придерживаются стандарта
    // в отношении пропускн. способности
    //res = res && (bytRat == sd.bytRat);
    res = res && (vbvBufSz == sd.vbvBufSz);

    res = res && (plId == sd.plId);
    res = res && (isProgr == sd.isProgr);
    res = res && (chromaFrmt == sd.chromaFrmt);
    res = res && (lowDelay == sd.lowDelay);

    return res;
}

Point FrameRates[10] =
{
    Point(0, 0),        // forbidden
    Point(24000, 1001), // NTSC FILM
    Point(24,    1   ), // FILM
    Point(25,    1   ), // PAL/SECAM FILM
    Point(30000, 1001), // NTSC VIDEO
    Point(30,    1   ), // 
    Point(50,    1   ), // PAL FIELD RATE
    Point(60000, 1001), // NTSC FIELD RATE
    Point(60,    1   ), // 
    Point(0, 0)         // reserved
};

Point SequenceData::PixelAspect() const
{
    // :TODO: надо парсить extSEQ_DISP ради там display_*_size, которые
    // могут и не совпадать с (wdh, hgt)
    return MpegDP((AspectFormat)sarCode, Point(wdh, hgt)).PixelAspect();
}

Point SequenceData::SizeAspect() const
{
    return MpegDP((AspectFormat)sarCode, Point(wdh, hgt)).DisplayAspect();
}

static bool CalcSeqData(HeaderDecode::Data& dat)
{
    // закодированные данные кратны размеру макроблока - 16
    dat.rawWdh = (dat.wdh+0xf) & ~0xf;
    dat.rawHgt = (dat.hgt+0xf) & ~0xf;

    if( !dat.sarCode || dat.sarCode>4 )
        return false;

    if( !dat.framRatCode || dat.framRatCode>8 )
        return false;
    dat.framRat = FrameRates[dat.framRatCode];
    dat.framRat.x *= dat.framRatN+1;
    dat.framRat.y *= dat.framRatD+1;
    if( dat.framRat.x == 0 || dat.framRat.y == 0 )
      return false;

    // переводим из единиц 400 бит/с
    ASSERT( dat.bytRat < 0x00100000 );
    dat.bytRat *= 50;

    dat.vbvBufSz *= 2*1024;

    if( !dat.chromaFrmt || dat.chromaFrmt>3 )
        return false;

    return true;
}

bool ExtHeaderDecode::SetSeqData(Decoder& dcr)
{
    SequenceData& seq = dcr.seqInf;
    HeaderDecode::Data& dat = (HeaderDecode::Data&)dcr;

    if( !CalcSeqData(dat) )
    {
        InvalidState(dcr, 0);
        return false;
    }

//     // согласно 6.1.1.6
//     if( seq.IsInit() && !(seq == (SequenceData&)dat) )
//     {
//         ChangeState(EndDecode::Instance(), dcr);
//         return false;
//     }
//     else
//         seq = (SequenceData&)dat;
    if( !seq.IsInit() || !(seq == (SequenceData&)dat) ) 
    {
        seq = (SequenceData&)dat;
        // согласно Doc: 6.1.1.6
        dcr.DataSkipTag(vtHEADER_COMPLETE, 0);
    }

    return true;
}

#define POW2( deg ) (1 << deg)

void ExtUserDecode::SetEUType(unsigned char type, Decoder& dcr)
{
    Data& dat = dcr;
    dat.type = type;

    switch( type )
    {
    case 0:
        dat.allowExt = POW2(extSEQ_DISP) | POW2(extSCAL_EXT);
        break;
    case 1:
        dat.allowExt = 0;
        break;
    case 2:
        dat.allowExt = POW2(extQUANT_MATR) | POW2(extCOPYRIGHT) |
                       POW2(extPIC_SPT)    | POW2(extPIC_TMP)   ;
        break;
    default:
        ASSERT(0);
    }
}

void ExtUserDecode::NextState(Decoder& dcr)
{
    if( !HaveBytes(5, dcr) )
        return;

    Data& dat = dcr;
    uint32_t code = GetStartCode(dcr);
    switch( code )
    {
    case vidEXT:
        {
            uint8_t id = GetExtId(dcr);
            int val = POW2(id) & dat.allowExt;
            if( !val )
            {
                // пропускаем несоответствия
                SkipState(*this, dcr, 4);
                return;
            }
            dat.allowExt &= ~val;

            switch( id )
            {
            case extSEQ:
            case extSEQ_DISP:
            case extQUANT_MATR:
            case extCOPYRIGHT:
            case extSCAL_EXT:
            case extPIC_DISP:
            case extPIC_CODING:
            case extPIC_SPT:
            case extPIC_TMP:
                SkipState(*this, dcr, 4);
                break;
            default:
                ASSERT_RTL(0);
            }
            return;
        }
        break;
    case vidUSR_DAT:
        SkipState(*this, dcr, 4);
        return;
    default:
        {
            // конец секции extension_and_user_data(i)
            switch( dat.type )
            {
            case 0:
                ChangeState(GOPDecode::Instance(), dcr);
                return;
            case 1:
                ChangeState(PicHeaderDecode::Instance(), dcr);
                return;
            case 2:
                ChangeState(SliceDecode::Instance(), dcr);
                return;
            default:
                ASSERT(0);
            }
        }
        break;
    }
}

#include PACK_ON
struct GOPHeader
{
    uint32_t  gopStartCode; // = vidGOP
     uint8_t  inf[4];       // time_code, closed_gop, broken_link
};
#include PACK_OFF

const int GOP_SIZE = 8;
BOOST_STATIC_ASSERT( GOP_SIZE == sizeof(GOPHeader) );

void GOPDecode::NextState(Decoder& dcr)
{
    if( !HaveBytes(4, dcr) )
        return;
    uint32_t code = GetStartCode(dcr);
    if( code == vidGOP )
    {
        if( !HaveBytes(GOP_SIZE, dcr) )
            return;

        GOPHeader& hdr = *(GOPHeader*)dcr.Data(0);
        if( !(hdr.inf[1] & 0x08) ) // marker bit
        {
            InvalidState(dcr);
            return;
        }
        dcr.isGOPClosed = hdr.inf[3] & 0x40;
        //bool is_broken  = hdr.inf[3] & 0x20;
        dcr.DataSkipTag(vtGOP, GOP_SIZE);

        ExtUserDecode& eu_stt = ExtUserDecode::Instance();
        eu_stt.SetEUType(1, dcr);
        ChangeWithNextState(eu_stt, dcr);
        return;
    }

    ChangeState(PicHeaderDecode::Instance(), dcr);
}

#include PACK_ON
struct PicHeader
{
    uint32_t  picStartCode; // = vidPIC
     uint8_t  inf[4];       //
};
#include PACK_OFF

const int PIC_SIZE = 8;
BOOST_STATIC_ASSERT( PIC_SIZE == sizeof(PicHeader) );

void PicHeaderDecode::NextState(Decoder& dcr)
{
    if( !HaveBytes(PIC_SIZE, dcr) )
        return;

    if( GetStartCode(dcr) != vidPIC )
    {
        InvalidState(dcr);
        return;
    }
    PicHeader& hdr = *(PicHeader*)dcr.Data(0);

    int tmp_ref =  hdr.inf[0]         << 2;
    tmp_ref    |= (hdr.inf[1] & 0xc0) >> 6;

    int type = (hdr.inf[1] & 0x38) >> 3;
    if( !StartPicture(dcr, tmp_ref, type) )
        return;

    //dcr.curPicTyp = (PicType)type;
    //dcr.DataSkipTag(vtPIC, PIC_SIZE);
    SkipState(PicCodingExtDecode::Instance(), dcr, PIC_SIZE);
}

void PicHeaderDecode::ClearState(Decoder& dcr, bool tag_error)
{
    Data& dat = dcr;
    bool err = dat.firstPic.IsInit();
    dat.firstPic.Init();
    err      = dat.secndPic.IsInit() || err;
    dat.secndPic.Init();

    if( tag_error && err )
        dcr.TagError();
}

bool PicHeaderDecode::StartPicture(Decoder& dcr, int tmp_ref, int type)
{
    if( !(type>=1 && type<=4) ||              // тип может быть только I,P,B,D
        (dcr.seqInf.lowDelay && type == ptB_FRAME) ) // Doc: 6.3.9
    {
        InvalidState(dcr);
        return false;
    }

    Data& dat = dcr;
    ASSERT( !dat.secndPic.IsInit() );

    Picture& pic = dat.firstPic.IsInit() ? dat.secndPic : dat.firstPic ;
    pic.type   = (PicType)type;
    pic.tmpRef = tmp_ref;
    pic.len    = 2; // 1 такт, по умолчанию
    pic.datPos = dcr.DatPos();

    return true;
}

#include PACK_ON
struct PicCodingHeader
{
    uint32_t  picStartCode; // = vidEXT
     uint8_t  inf[5];       // extPIC_CODING, ...
};
#include PACK_OFF

const int PIC_CODING_SIZE = 9;
BOOST_STATIC_ASSERT( PIC_CODING_SIZE == sizeof(PicCodingHeader) );

void PicCodingExtDecode::NextState(Decoder& dcr)
{
    int hdr_size = PIC_CODING_SIZE;
    if( !HaveBytes(hdr_size, dcr) )
        return;

    if( GetExtId(dcr) != extPIC_CODING )
    {
        InvalidState(dcr);
        return;
    }
    PicCodingHeader& hdr = *(PicCodingHeader*)dcr.Data(0);

    int pic_struct   = hdr.inf[2] & 0x03;
    bool tff         = hdr.inf[3] & 0x80;
    bool rff         = hdr.inf[3] & 0x02;
    bool progr_frame = hdr.inf[4] & 0x80;

    if( hdr.inf[4] & 0x40 )
    {
        hdr_size += 2;
        if( !HaveBytes(hdr_size, dcr) )
            return;
    }
    if( !CompletePicture(dcr, pic_struct, tff, rff, progr_frame) )
        return;

    ExtUserDecode& eu_stt = ExtUserDecode::Instance();
    eu_stt.SetEUType(2, dcr);
    //dcr.DataSkip(4);
    //SkipState(eu_stt, dcr);
    dcr.DataSkip(hdr_size);
    ChangeWithNextState(eu_stt, dcr);
}

static void FillPicture2(Picture& pic, PicStructType pic_struct,
                        bool tff, bool rff, bool progr_frame)
{
    pic.structTyp  = pic_struct;
    pic.tff        = tff;
    pic.rff        = rff;
    pic.progrFrame = progr_frame;
}

static void ShiftPicture(Decoder& dcr, Picture& first_pic, Picture& secnd_pic )
{
    // начали со второй половины пред. кадра :(
    first_pic = secnd_pic;
    dcr.TagError();
    secnd_pic.Init();
}

bool PicCodingExtDecode::CompletePicture(Decoder& dcr, int pt, 
                                         bool tff, bool rff, bool progr_frame)
{
    PicHeaderDecode::Data& dat = dcr;
    SequenceData& seq          = dcr.seqInf;
    PicStructType pic_struct   = (PicStructType)pt;
    if( pic_struct == pstRESERVED )
    {
        InvalidState(dcr);
        return false;
    }
    Picture& pic = dat.firstPic;
    ASSERT( pic.IsInit() );

    if( seq.isProgr )
    {
        // Doc: 6.3.5
        if( (pic_struct != pstFRAM_PICTURE) || !progr_frame )
        {
            InvalidState(dcr);
            return false;
        }
        ASSERT( !dat.secndPic.IsInit() );
        
        // Doc: 6.3.10, 7.12
        if( !rff )
        {
            if( tff )
            {
                InvalidState(dcr);
                return false;
            }
        }
        else
            pic.len = tff ? 6 : 4 ;
        FillPicture2(pic, pic_struct, tff, rff, progr_frame);
    }
    else // !seq.isProgr, все кадры составлены из двух - top_field + bottom_field
    {
        if( pic_struct == pstFRAM_PICTURE )
        {
            // картинка содержит обе половинки кадра сразу
            if( dat.secndPic.IsInit() )
                ShiftPicture(dcr, pic, dat.secndPic);

            if( !progr_frame )
            {
                if( rff )
                {
                    InvalidState(dcr);
                    return false;
                }
            }
            else
            {
                // хотя последовательность и чересстрочная, данные кадры - прогрессивные (вроде исключений),
                // формально же кадр запакован в 2 половинки
                if( rff )
                    pic.len = 3;
            }
            FillPicture2(pic, pic_struct, tff, rff, progr_frame);
        }
        else
        {
            // кадр разложен по двум картинкам
            if( tff || rff ||
                progr_frame    ) // не может быть =1 согласно 6.3.10
            {
                InvalidState(dcr);
                return false;
            }

            if( !dat.secndPic.IsInit() )
            {
                // первая часть пришла
                FillPicture2(pic, pic_struct, tff, rff, progr_frame);
                return true;
            }
            else
            {
                Picture& sec_pic = dat.secndPic;
                FillPicture2(sec_pic, pic_struct, tff, rff, progr_frame);
                ASSERT( pic.structTyp != pstRESERVED && pic.structTyp != pstFRAM_PICTURE );

                bool is_pair = (pic.tmpRef == sec_pic.tmpRef) || 
                               (seq.lowDelay && (pic.tmpRef+1 < sec_pic.tmpRef));
                if( !is_pair || 
                    (pic.structTyp == sec_pic.structTyp) ) // должны быть разными
                {
                    ShiftPicture(dcr, pic, sec_pic);
                    return true;
                }

                sec_pic.Init();
            }
        }
    }
    dcr.SetFrame(pic);
    pic.Init();

    return true;
}


void SliceDecode::NextState(Decoder& dcr)
{
    uint32_t code = GetStartCode(dcr);
    switch( code )
    {
    case vidGOP:
    case vidPIC:
        ChangeState(GOPDecode::Instance(), dcr);
        return;
    case vidSEQ_HDR:
        ChangeState(HeaderDecode::Instance(), dcr);
        return;
    case vidSEQ_END:
        ChangeState(EndDecode::Instance(), dcr);
        return;
    }

    if( !((code >= vidSLICE_BEG) && (code < vidSLICE_END)) )
    {
        InvalidState(dcr);
        return;
    }

    // :TODO: вместо того, чтобы просто пропускать,
    // надо приделать декодер срезов от mpeg2dec
    SkipState(*this, dcr, 4);
}

} // namespace Mpeg

