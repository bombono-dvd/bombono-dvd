//
// mmpeg_demux.cpp
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mmpeg_demux.h"
#include "mmpeg.h"

MpegSeqDemuxer::MpegSeqDemuxer(int track_num)
    : implDmx(0), trkIdx(track_num)
{}

void MpegSeqDemuxer::Init(io::stream* strm, TrackBuf* buf)
{
    trkBuf = buf;

    implDmx.SetStream(strm);
    implDmx.SetService(this);
    implDmx.Begin();
}

void MpegSeqDemuxer::GetData(Mpeg::Demuxer&, int len)
{
    ASSERT(trkBuf);

    trkBuf->AppendFromStream(implDmx.ObjStrm(), len);
}

bool BufDemuxer::Demux(MpegPlayer& plyr)
{
    io::stream& strm = *plyr.inpStrm;
    int cnt = strm.raw_read(rawBuf, ARR_SIZE(rawBuf));
    // пока поток (файл) не закончился
    bool res = strm;
    if( res )
        res = Demux((uint8_t*)rawBuf, (uint8_t*)rawBuf+cnt, plyr) ? false : true ;
    return res;
}

int IDemuxer::Demux(uint8_t* beg, uint8_t* end, MpegPlayer& plyr)
{
    plyr.trkBuf.Append((char*)beg, (char*)end);
    return 0;
}

namespace Mpeg_legacy 
{

static void CheckTSPid(int ts_pid, bool is_strict = false)
{
    if( (ts_pid || is_strict) && (ts_pid<mpegFIRST_PRG || ts_pid>mpegLAST_PRG) )
        Error("Bad program number for MPEG transport stream");
}

// для удобства номер потока отсчитываем с нуля
MpegDemuxer::MpegDemuxer(int track_num)
    : trackNum(track_num+mpegFIRST_TRACK), tsPid(0),
      dmxState(dsDEMUX_SKIP), state_bytes(0)
{
    if( trackNum>mpegLAST_TRACK )
        Error("Bad track number for MPEG stream");

    CheckTSPid(tsPid);
}

TSMpegDemuxer::TSMpegDemuxer()
    : curPos(tsBuf), isFirstIn(true)
{
    CheckTSPid(tsPid);
}

TSMpegDemuxer::TSMpegDemuxer(int ts_pid)
    : curPos(tsBuf), isFirstIn(false)
{
    tsPid = ts_pid;
    CheckTSPid(tsPid, true);
}

/////////////////////////////////////////////////////////////////////
// :COPY_N_PASTE: взято из mpeg2dec.c, пакет mpeg2dec
// :WARNING:
// Пока используем реализацию демиксера из mpeg2dec.c, в пользу развития
// более интересного функционала; соответственно, делаем минимальные изменения
// в MpegDemuxer::DoDemux(), дабы (в случае ошибок) легко откатиться к оригинальному
// варианту.

int MpegDemuxer::Demux(uint8_t* beg, uint8_t* end, MpegPlayer& plyr)
{
    return DoDemux(beg, end, 0, plyr);
}

const int DEMUX_PAYLOAD_START = 1;

int MpegDemuxer::DoDemux(uint8_t * beg, uint8_t * end, int flags, MpegPlayer& plyr)
{
    MpegDecodec& dec = plyr.decodec;
    TrackBuf& trk_buf = plyr.trkBuf;

    static int mpeg1_skip_table[16] = {
        0, 0, 4, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    /*
     * dmxState:
     * if "dmxState" = dsDEMUX_HEADER, then "head_buf" contains the first
     *     "bytes" bytes from some header.
     * if "dmxState" == dsDEMUX_DATA, then we need to copy "bytes" bytes
     *     of ES data before the next header.
     * if "dmxState" == dsDEMUX_SKIP, then we need to skip "bytes" bytes
     *     of data before the next header.
     *
     * NEEDBYTES makes sure we have the requested number of bytes for a
     * header. If we dont, it copies what we have into head_buf and returns,
     * so that when we come back with more data we finish decoding this header.
     *
     * DONEBYTES updates "beg" to point after the header we just parsed.
     */


    uint8_t * header;
    int bytes;
    int len;

#define NEEDBYTES(x)						\
    do {							\
	int missing;						\
								\
	missing = (x) - bytes;					\
	if (missing > 0) {					\
	    if (header == head_buf) {				\
		if (missing <= end - beg) {			\
		    memcpy (header + bytes, beg, missing);	\
		    beg += missing;				\
		    bytes = (x);				\
		} else {					\
		    memcpy (header + bytes, beg, end - beg);	\
		    state_bytes = bytes + end - beg;		\
		    return 0;					\
		}						\
	    } else {						\
		memcpy (head_buf, header, bytes);		\
		dmxState = dsDEMUX_HEADER;				\
		state_bytes = bytes;				\
		return 0;					\
	    }							\
	}							\
    } while (0)

#define DONEBYTES(x)		\
    do {			\
	if (header != head_buf)	\
	    beg = header + (x);	\
    } while (0)

    if(flags & DEMUX_PAYLOAD_START)
        goto payload_start;
    switch(dmxState)
    {
    case dsDEMUX_HEADER:
        if(state_bytes > 0)
        {
            header = head_buf;
            bytes = state_bytes;
            goto continue_header;
        }
        break;
    case dsDEMUX_DATA:
        if(tsPid || (state_bytes > end - beg))
        {
//             decode_mpeg2 ((char*)beg, (char*)end, dec);
            trk_buf.Append((char*)beg, (char*)end);
            state_bytes -= end - beg;
            return 0;
        }
//         decode_mpeg2 ((char*)beg, (char*)(beg + state_bytes), dec);
        trk_buf.Append((char*)beg, (char*)(beg + state_bytes));
        beg += state_bytes;
        break;
    case dsDEMUX_SKIP:
        if(tsPid || (state_bytes > end - beg))
        {
            state_bytes -= end - beg;
            return 0;
        }
        beg += state_bytes;
        break;
    }

    while(1)
    {
        if(tsPid)
        {
            dmxState = dsDEMUX_SKIP;
            return 0;
        }
        payload_start:
        header = beg;
        bytes = end - beg;
        continue_header:
        NEEDBYTES (4);
        if(header[0] || header[1] || (header[2] != 1))
        {
            if(tsPid)
            {
                dmxState = dsDEMUX_SKIP;
                return 0;
            }
            else if(header != head_buf)
            {
                beg++;
                goto payload_start;
            }
            else
            {
                header[0] = header[1];
                header[1] = header[2];
                header[2] = header[3];
                bytes = 3;
                goto continue_header;
            }
        }
        if(tsPid)
        {
            if((header[3] >= 0xe0) && (header[3] <= 0xef))
                goto pes;
            fprintf (stderr, "bad stream id %x\n", header[3]);
            exit (1);
        }
        switch(header[3])
        {
        case 0xb9:  /* program end code */
            /* DONEBYTES (4); */
            /* break;         */
            return 1;
        case 0xba:  /* pack header */
            NEEDBYTES (5);
            if((header[4] & 0xc0) == 0x40)
            {   /* mpeg2 */
                NEEDBYTES (14);
                len = 14 + (header[13] & 7);
                NEEDBYTES (len);
                DONEBYTES (len);
                /* header points to the mpeg2 pack header */
            }
            else if((header[4] & 0xf0) == 0x20)
            {    /* mpeg1 */
                NEEDBYTES (12);
                DONEBYTES (12);
                /* header points to the mpeg1 pack header */
            }
            else
            {
                fprintf (stderr, "weird pack header\n");
                DONEBYTES (5);
            }
            break;
        default:
            if(header[3] == trackNum)
            {
                pes:
                NEEDBYTES (7);
                if((header[6] & 0xc0) == 0x80)
                {   /* mpeg2 */
                    NEEDBYTES (9);
                    len = 9 + header[8];
                    NEEDBYTES (len);
                    /* header points to the mpeg2 pes header */
                    if(header[7] & 0x80)
                    {
                        uint32_t pts, dts;

                        pts = (((header[9] >> 1) << 30) |
                               (header[10] << 22) | ((header[11] >> 1) << 15) |
                               (header[12] << 7) | (header[13] >> 1));
                        dts = (!(header[7] & 0x40) ? pts :
                               (((header[14] >> 1) << 30) |
                                (header[15] << 22) |
                                ((header[16] >> 1) << 15) |
                                (header[17] << 7) | (header[18] >> 1)));
                        mpeg2_tag_picture (dec.Mpeg2Dec(), pts, dts);
                    }
                }
                else
                {    /* mpeg1 */
                    int len_skip;
                    uint8_t * ptsbuf;

                    len = 7;
                    while(header[len - 1] == 0xff)
                    {
                        len++;
                        NEEDBYTES (len);
                        if(len > 23)
                        {
                            fprintf (stderr, "too much stuffing\n");
                            break;
                        }
                    }
                    if((header[len - 1] & 0xc0) == 0x40)
                    {
                        len += 2;
                        NEEDBYTES (len);
                    }
                    len_skip = len;
                    len += mpeg1_skip_table[header[len - 1] >> 4];
                    NEEDBYTES (len);
                    /* header points to the mpeg1 pes header */
                    ptsbuf = header + len_skip;
                    if((ptsbuf[-1] & 0xe0) == 0x20)
                    {
                        uint32_t pts, dts;

                        pts = (((ptsbuf[-1] >> 1) << 30) |
                               (ptsbuf[0] << 22) | ((ptsbuf[1] >> 1) << 15) |
                               (ptsbuf[2] << 7) | (ptsbuf[3] >> 1));
                        dts = (((ptsbuf[-1] & 0xf0) != 0x30) ? pts :
                               (((ptsbuf[4] >> 1) << 30) |
                                (ptsbuf[5] << 22) | ((ptsbuf[6] >> 1) << 15) |
                                (ptsbuf[7] << 7) | (ptsbuf[18] >> 1)));
                        // это временные метки, для перемещения вперед-назад при проигрывании
                        // пока не нужны, но со временем ...
                        mpeg2_tag_picture (dec.Mpeg2Dec(), pts, dts);
                    }
                }
                DONEBYTES (len);
                bytes = 6 + (header[4] << 8) + header[5] - len;
                if(tsPid || (bytes > end - beg))
                {
//                     decode_mpeg2 ((char*)beg, (char*)end, dec);
                    trk_buf.Append((char*)beg, (char*)end);
                    dmxState = dsDEMUX_DATA;
                    state_bytes = bytes - (end - beg);
                    return 0;
                }
                else if(bytes > 0)
                {
//                     decode_mpeg2 ((char*)beg, (char*)(beg + bytes), dec);
                    trk_buf.Append((char*)beg, (char*)(beg + bytes));
                    beg += bytes;
                }
            }
            else if(header[3] < 0xb9)
            {
                fprintf (stderr,
                         "looks like a video stream, not system stream\n");
                DONEBYTES (4);
            }
            else
            {
                NEEDBYTES (6);
                DONEBYTES (6);
                bytes = (header[4] << 8) + header[5];
                if(bytes > end - beg)
                {
                    dmxState = dsDEMUX_SKIP;
                    state_bytes = bytes - (end - beg);
                    return 0;
                }
                beg += bytes;
            }
        }
    }
}

int TSMpegDemuxer::Demux(uint8_t* beg, uint8_t* end, MpegPlayer& plyr)
{
    int ext_sz = end-beg;
    if( ext_sz>STRM_BUF_SZ )
        Error("TSMpegDemuxer: Too big buffer to accept");

    // :TRICKY:
    // копируем в локальный буфер, чтобы остатки с предыдущего раза
    // были оказались перед beg (простота обработки)
    // только из-за этого локальный буфер и нужен
    memcpy(curPos, beg, ext_sz);
    uint8_t* end_pos = curPos+ext_sz;
    curPos = tsBuf;

    for( uint8_t* next_pos,* data; next_pos=curPos+mpegTS_PACKET_SZ, next_pos<=end_pos; curPos=next_pos )
    {
        if( *curPos != 0x47 )
        {
            fprintf (stderr, "bad sync byte\n");
            next_pos = curPos + 1;
            continue;
        }

        int pid = ((curPos[1] << 8) + curPos[2]) & 0x1fff;
        // инициализация tsPid
        if( isFirstIn )
        {
            isFirstIn = false;
            tsPid = pid;
        }
        
        if( pid != tsPid )
            continue;
        data = curPos + 4;
        if( curPos[3] & 0x20 )
        {    /* buf contains an adaptation field */
            data = curPos + 5 + curPos[4];
            if(data > next_pos)
                continue;
        }
        if( curPos[3] & 0x10 )
            DoDemux(data, next_pos, (curPos[1] & 0x40) ? DEMUX_PAYLOAD_START : 0, plyr);
    }

    // остатки - на начало
    memcpy(tsBuf, curPos, end_pos-curPos);
    curPos = tsBuf + (end_pos-curPos);
    return 0;
}

} // namespace Mpeg_legacy 

