//
// mmpeg.cpp
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

#include <stdlib.h>

#include "mmpeg.h"

//
// MpegPlayer
//

void PlayerState::ChangeState(MpegPlayer& plyr, PlayerState& stt)
{
    plyr.ChangeState(&stt);
}

// в конечном состоянии ничего не делаем
void PlayerEnd::NextState(MpegPlayer& /*player*/)
{}

void PlayerPict::NextState(MpegPlayer& player)
{
    ChangeState(player, PlayerDecode::Instance());
}

void PlayerSeq::NextState(MpegPlayer& player)
{
    SeqData& dat = player;
    ASSERT( !dat.isSet );
    dat.isSet = true;

    ChangeState(player, PlayerDecode::Instance());
}

void PlayerDemux::NextState(MpegPlayer& player)
{
    TrackBuf& buf    = player.trkBuf;
    SeqDemuxer* dmx  = player.demuxer;
    DemuxData& dat   = player;

    while( !buf.IsFull() &&   // пока не заполнен буфер
           !dat.isEnd         // и демиксер дошел до конца дорожки(программы)
         )
    {
        if( !dmx->Demux(player) )
        {
            dat.isEnd = true;
            break; // закончилась наша дорожка в потоке
        }
    }

    if( buf.IsEmpty() )
        ChangeState(player, PlayerEnd::Instance());
    else
        ChangeState(player, PlayerDecode::Instance());

    return;
}

void PlayerDecode::NextState(MpegPlayer& player)
{
    mpeg2dec_t* dec = player.decodec.Mpeg2Dec();
    const mpeg2_info_t* info = mpeg2_info(dec);

    mpeg2_state_t state;
    //const mpeg2_sequence_t* sequence;
    while( 1 )
    {
        state = mpeg2_parse(dec);
        //sequence = info->sequence;
        switch(state)
        {
        case STATE_BUFFER:
            {
                TrackBuf& buf = player.trkBuf;
                if( buf.IsEmpty() )
                {
                    ChangeState(player, PlayerDemux::Instance());
                    return;
                }
                else
                {
                    mpeg2_buffer(dec, (uint8_t*)buf.Beg(), (uint8_t*)buf.End());
                    buf.Clear();
                }
                break;
            }
        case STATE_SEQUENCE:
            {
                ChangeState(player, PlayerSeq::Instance());
                return;
            }
        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
            {
                // :TODO: непонятно, почему здесь info->display_fbuf может быть нуль
                // (хотя, судя по тому, как другие используют, это нормально)
                //ASSERT(info->display_fbuf);
                if(info->display_fbuf)
                {
                    ChangeState(player, PlayerPict::Instance());
                    return;
                }
                break;
            }
        default:
            break;
        }
    }
}


///////////////////////////////
// сам проигрыватель

MpegPlayer::MpegPlayer(io::stream* strm, SeqDemuxer* dmx)
    : demuxer(dmx), inpStrm(strm)
{
    demuxer->Init(strm, &trkBuf);

    // начальное состояние
    ChangeState(&PlayerDemux::Instance());
}

MpegPlayer::~MpegPlayer()
{
    if( inpStrm )
        delete inpStrm;
    if( demuxer )
        delete demuxer;
}

bool MpegPlayer::NextImage()
{
    while( NextState(), *this )
        if( state->IsPict() )
            return true;
    return false;
}

bool MpegPlayer::NextSeq()
{
    while( NextState(), *this )
        if( state->IsSeq() )
            return true;
    return false;
}

void MpegPlayer::ChangeState(PlayerState* stt)
{
    state = stt;
}

/////////////////////////////////////////////////////////

bool GetMovieInfo(MovieInfo& mi, MpegDecodec& dec)
{
    mi.Clear();

    mpeg2dec_t* mpeg2dec = dec.Mpeg2Dec();
    const mpeg2_info_t* info = mpeg2_info(mpeg2dec);

    // 1 размеры
    mi.ySz = Point(info->sequence->width, info->sequence->height);

    y4m_si_set_width(&mi.streamInfo,  mi.ySz.x);
    y4m_si_set_height(&mi.streamInfo, mi.ySz.y);

    // 2 хроматический режим (хроморежим, chroma)
    mi.uSz = mi.vSz = Point(info->sequence->chroma_width, info->sequence->chroma_height);
    if( (mi.ySz.x/2 != mi.uSz.x) || (mi.ySz.y/2 != mi.uSz.y) ) 
    {
        // не 4:2:0
        return false;
    }
    y4m_si_set_chroma(&mi.streamInfo, Y4M_CHROMA_420MPEG2);

    // 3 частота кадров (framerate)
    static unsigned int frame_period[16] = 
    {
        0, 1126125, 1125000, 1080000, 900900, 900000, 540000, 450450, 450000,
        /* unofficial: xing 15 fps */
        1800000,
        /* unofficial: libmpeg3 "Unofficial economy rates" 5/10/12/15 fps */
        5400000, 2700000, 2250000, 1800000, 0, 0
    };

    int frame_rate = 0;
    for(int i=0; i<16; i++)
        if( frame_period[i] == info->sequence->frame_period )
        {
          frame_rate = i;
          break;
        }
    y4m_ratio_t ratio = mpeg_framerate( frame_rate );
    y4m_si_set_framerate(&mi.streamInfo, ratio);

    // 4 развертка (interlace)
    // :TODO: в принципе, смысла устанавливать интерлейсинг пока нет, да
    // и как его устанавливать пока непонятно,-
    // libmpeg2 реально устанавливает данные о нем (верхние строчки "TOP",
    // или нижние "BOTTOM") в каждой картинке, а не в заголовке
    y4m_si_set_interlace(&mi.streamInfo, Y4M_ILACE_NONE);
//     if( info->sequence->flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE )
//         y4m_si_set_interlace(this->strm_info, Y4M_ILACE_NONE );
//     else
//         y4m_si_set_interlace(this->strm_info, Y4M_ILACE_NONE);

    // 5 аспект точек (aspect)
    // следует передавать соотношение не всего кадра, а именно пиксела!      
    // т.е. не 4:3, 16:9 и т.д. 
    ratio.n = info->sequence->pixel_width;
    ratio.d = info->sequence->pixel_height;
    y4m_si_set_sampleaspect(&mi.streamInfo, ratio);


    // 6 дополнительные тэги yuv4mpeg (xtags )
    // :TODO: а они нам(!) вообще нужны?
    y4m_ratio_t tmp_ratio = ratio;
    mpeg_framerate_code_t display_aspect_code = 
        mpeg_guess_mpeg_aspect_code(info->sequence->flags&SEQ_FLAG_MPEG2 ? 2 : 1,
                                    tmp_ratio,
                                    mi.ySz.x, mi.ySz.y);
    // mpeg_guess_mpeg_aspect_code() работает примерно так
    /*
    y4m_frame_rate.n = info->sequence->pixel_width*this->width;
    y4m_frame_rate.d = info->sequence->pixel_height*this->height;
    {
        int w = y4m_frame_rate.n;
        int h = y4m_frame_rate.d;
                     find greatest common divisor 
        while( w )
        {
            int t = w;
            w = h%w;
            h = t;
        }
        y4m_frame_rate.n /= h;
        y4m_frame_rate.d /= h;
    }
    */
    char buf[20];
    snprintf( buf, 19, "XM2AR%03d", display_aspect_code );
    y4m_xtag_add( y4m_si_xtags(&mi.streamInfo), buf );

    mi.Init();
    return true;
}

