//
// mdemux/decoder.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2010 Ilya Murav'jov
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

#include <mlib/read_stream.h>
#include <mlib/lambda.h>

#include "decoder.h"
#include "videoline.h"

C_LINKAGE_BEGIN
#include <../libs/mpeg2dec/include/config.h>
#include <../libs/mpeg2dec/include/attributes.h>
#include <../libs/mpeg2dec/libmpeg2/mpeg2_internal.h>
C_LINKAGE_END


MpegDecodec::MpegDecodec(): mpeg2Dec(0), fofTyp(fofYCBCR)
{
    mpeg2Dec = mpeg2_init();
    if( !mpeg2Dec )
        Error("Could not allocate a decoder object.\n");
}

MpegDecodec::~MpegDecodec()
{
    mpeg2_close(mpeg2Dec);
}

MpegDecodec::PlanesType MpegDecodec::Planes()
{
    const mpeg2_info_t * info = mpeg2_info(mpeg2Dec);
    ASSERT( info && info->display_fbuf );

    return info->display_fbuf->buf;
}

void MpegDecodec::Init(bool full_reset)
{
    mpeg2_reset(mpeg2Dec, full_reset ? 1 : 0);
    ASSERT( !(full_reset && IsInit()) );
}

bool MpegDecodec::IsInit()
{
    // sequence == 0, когда встречаем b7 (= vidSEQ_END), поэтому
    // проверяем напрямую
    //return mpeg2_info(mpeg2Dec)->sequence;
    return mpeg2Dec->sequence.width != (unsigned)-1;
}

void MpegDecodec::ClearFlags()
{
    picFound = false;
}

void MpegDecodec::SetFormat()
{
    switch( fofTyp )
    {
    case fofYCBCR:
        break; // ничего
    case fofRGB:
        mpeg2_convert(mpeg2Dec, mpeg2convert_rgb24, NULL);
        break;
    case fofRGBA:
        // похоже, что конвертор учитывает порядок байтов
#ifdef HAS_LITTLE_ENDIAN
        mpeg2_convert(mpeg2Dec, mpeg2convert_bgr32, NULL);
#else
        mpeg2_convert(mpeg2Dec, mpeg2convert_rgb32, NULL);
#endif
        break;
    default:
        ASSERT(0);
    }
}

char* MpegDecodec::DoReadForInit(char* buf, int buf_len)
{
    mpeg2_buffer(mpeg2Dec, (uint8_t*)buf, (uint8_t*)buf+buf_len);
    for( mpeg2_state_t stt; stt = mpeg2_parse(mpeg2Dec), stt != STATE_BUFFER;  )
        if( stt == STATE_SEQUENCE )
        {
            ASSERT( IsInit() );
            SetFormat();
            break;
        }

    return !IsInit() ? buf : 0 ;
}

void MpegDecodec::ReadForInit(const Mpeg::Chunk& chk, io::stream& strm)
{
    if( IsInit() )
        return;

    ReadFunctor read_fnr = bl::bind(&MpegDecodec::DoReadForInit, this, bl::_1, bl::_2);

    strm.seekg(chk.extPos);
    ReadStream(read_fnr, strm, chk.len);
}

void MpegDecodec::PlayMpeg()
{
    for( mpeg2_state_t stt; stt = mpeg2_parse(mpeg2Dec), stt != STATE_BUFFER; )
    {
        // пока используем только для отладки эти флаги
        switch( stt )
        {
        case STATE_SEQUENCE:
            // :KLUDGE: а где false устанавливается?
            ASSERT( !seqFound );
            seqFound = true;

            SetFormat();
            break;
        case STATE_PICTURE:
            ASSERT( !picFound );
            picFound = true;
            break;
        case STATE_SLICE:
            picEnd = mpeg2_info(mpeg2Dec)->display_fbuf;
            break;
        case STATE_END:
        case STATE_INVALID_END:
            ASSERT(0);
            break;
        default:
            ;
        }
    }
}

uint8_t FrameSplit[4] = { 0, 0, 1, 0xb3 }; // = vidSEQ_HDR

void MpegDecodec::ReadPrefix()
{
    mpeg2_buffer(mpeg2Dec, FrameSplit, FrameSplit+4);
    PlayMpeg();
}

void MpegDecodec::SetOutputFormat(FrameOutputFrmt typ)
{
    ASSERT( !IsInit() );
    fofTyp = typ;
}

void MpegDecodec::ReadBegin()
{
    ASSERT( IsInit() );
    Init(false);

    ReadPrefix();
}

char* MpegDecodec::DoReadFrame(char* buf, int buf_len)
{
    mpeg2_buffer(mpeg2Dec, (uint8_t*)buf, (uint8_t*)buf+buf_len);
    PlayMpeg();
    return buf;
}

void MpegDecodec::ReadFrame(const Mpeg::FrameData& fram, io::stream& strm)
{
    using namespace Mpeg;
    ASSERT( IsInit() && fram.IsInit() );
    ClearFlags();

    // 0 заменяем значащий байт префикса на тот, что у тек. картинки
    uint32_t prefix = fram.opt&fdHEADER 
        ? Mpeg::vidSEQ_HDR
        : (fram.opt&fdGOP ? Mpeg::vidGOP : Mpeg::vidPIC) ;
    mpeg2Dec->code = VSignByte(prefix);

    // 1 прогоняем данные картинки
    for( FrameData::ChunkList::const_iterator it = fram.dat.begin(), end = fram.dat.end(); it != end; ++it )
    {
        const Chunk& chk = *it;
        ReadFunctor read_fnr = bl::bind(&MpegDecodec::DoReadFrame, this, bl::_1, bl::_2);

        strm.seekg(chk.extPos);
        ReadStream(read_fnr, strm, chk.len);
    }

    // 2 в конце начало нового кадра - разделитель для mpeg2dec_t
    ReadPrefix();
    // всегда находим начало кадра, а в случае B-кадров должен быть готов и результат
    ASSERT( picFound && ((fram.typ != ptB_FRAME) || picEnd) );
}

MpegDecodec::PlanesType MpegDecodec::FrameData(FrameDecType fdt) const
{
    PlanesType dat = NULL;
    if( fdt == fdtCURRENT )
    {
        dat = mpeg2Dec->fbuf[0]->buf;
        const mpeg2_fbuf_t * disp_dat = mpeg2_info(mpeg2Dec)->display_fbuf;
        ASSERT_OR_UNUSED_VAR( !disp_dat || (disp_dat->buf == dat), disp_dat );
    }
    else
    {
        int idx = (fdt == fdtRIGHT) ? 0 : 1 ;
        if( mpeg2Dec->decoder.coding_type == B_TYPE )
            idx++;
        dat = mpeg2Dec->fbuf[idx]->buf;
    }
    return dat;
}

