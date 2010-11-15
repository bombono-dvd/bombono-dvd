//
// mdemux/tests/ac3_splitter.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009-2010 Ilya Murav'jov
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

#include <mdemux/tests/_pc_.h>

#include <mdemux/trackbuf.h>
#include <mlib/string.h>

#include <mlib/lambda.h>
#include <mlib/read_stream.h>     // ReadStream()
#include <mlib/sdk/stream_util.h> // StreamSize()

#include <vector>
#include <sstream>
#include <math.h>

// 
// COPY_N_PASTE_REALIZE
// 
// a52_syncinfo - из пакета liba52 - http://liba52.sourceforge.net/
// 

#define A52_CHANNEL 0
#define A52_MONO 1
#define A52_STEREO 2
#define A52_3F 3
#define A52_2F1R 4
#define A52_3F1R 5
#define A52_2F2R 6
#define A52_3F2R 7
#define A52_CHANNEL1 8
#define A52_CHANNEL2 9
#define A52_DOLBY 10
#define A52_CHANNEL_MASK 15

#define A52_LFE 16
#define A52_ADJUST_LEVEL 32

int a52_syncinfo (uint8_t * buf, int * flags,
                  int * sample_rate, int * bit_rate)
{
    static uint8_t halfrate[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3};

    static int rate[] = { 32,  40,  48,  56,  64,  80,  96, 112,
        128, 160, 192, 224, 256, 320, 384, 448,
        512, 576, 640};
    static uint8_t lfeon[8] = {0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01};
    int frmsizecod;
    int bitrate;
    int half;
    int acmod;

    if ((buf[0] != 0x0b) || (buf[1] != 0x77))   /* syncword */
        return 0;

    if (buf[5] >= 0x60)     /* bsid >= 12 */
        return 0;
    half = halfrate[buf[5] >> 3];

    /* acmod, dsurmod and lfeon */
    acmod = buf[6] >> 5;
    *flags = ((((buf[6] & 0xf8) == 0x50) ? A52_DOLBY : acmod) |
              ((buf[6] & lfeon[acmod]) ? A52_LFE : 0));

    frmsizecod = buf[4] & 63;
    if (frmsizecod >= 38)
        return 0;
    bitrate = rate [frmsizecod >> 1];
    *bit_rate = (bitrate * 1000) >> half;

    switch (buf[4] & 0xc0)
    {
    case 0:
        *sample_rate = 48000 >> half;
        return 4 * bitrate;
    case 0x40:
        *sample_rate = 44100 >> half;
        return 2 * (320 * bitrate / 147 + (frmsizecod & 1));
    case 0x80:
        *sample_rate = 32000 >> half;
        return 6 * bitrate;
    default:
        return 0;
    }
}

void ErrorAndExit(const std::string& err_msg, int status)
{
    if( !err_msg.empty() )
        io::cerr << err_msg << io::endl;
    std::exit(status);
}

static ptr::shared<io::stream> MakeDestStream(const std::string& prefix, int num)
{
    std::string dst_name = (str::stream() << prefix << "." << num << ".ac3").str();
    ptr::shared<io::stream> dst = new io::stream(dst_name.c_str(), iof::out);
    if( !*dst )
        ErrorAndExit((str::stream() << "Can't write to file: " << dst_name).str(), 2);

    return dst;
}

char* FillBySplit(char* buf, int cnt, io::stream& dst1, io::stream& dst2, const io::pos& split_pos, io::pos& write_pos)
{
    if( write_pos + cnt < split_pos )
        dst1.write(buf, cnt);
    else if( write_pos < split_pos )
    {
        int to_dst1 = int(split_pos-write_pos);
        dst1.write(buf, to_dst1);
        dst2.write(buf+to_dst1, cnt-to_dst1);
    }
    else // write_pos >= split_pos
        dst2.write(buf, cnt);

    write_pos += cnt;
    return buf;
}


int main(int argc, char *argv[])
{
    if( argc != 4 )
    {
        io::cerr << "Usage: \t ac3_splitter <seconds_to_split> <file.ac3> <out_file_prefix>"      << io::endl;
        io::cerr << "\t ac3_splitter divides one AC3-stream into two by time <seconds_to_split>." << io::endl;

        ErrorAndExit(std::string(), 1);
    }

    double sec_time;
    std::stringstream time_str(argv[1]);
    time_str >> sec_time;
    io::cout << "Split time (sec): \t" << sec_time << io::endl;
    ASSERT( sec_time > 0 );

    const char* strm_fname = argv[2];
    io::stream strm(strm_fname, iof::in);
    if( !strm )
        ErrorAndExit((str::stream() << "Can't open file: " << strm_fname).str(), 2);
    
    TrackBuf buf;
    typedef std::vector<io::pos> FrameArray;
    FrameArray frame_array;

    // 1 парсим файл и узнаем длину в секундах
    io::pos hdr_pos = 0;
    int prev_sample_rate = 0;
    for( int len=0 ; buf.AppendFromStream(strm, 7), strm; 
         strm.seekg(hdr_pos+(io::pos)len, iof::beg), buf.Clear() )
    {
        ASSERT( !buf.IsEmpty() );
        if( buf.Size() != 7 )
            ErrorAndExit((str::stream() << "Read error: unexpected end of file?" << strm_fname).str(), 4);

        hdr_pos = strm.tellg()-(io::pos)7;
        int flags, sample_rate, bit_rate;
        len = a52_syncinfo((uint8_t*)buf.Beg(), &flags, &sample_rate, &bit_rate);
        if( len == 0 )
            ErrorAndExit((str::stream() << "Not AC-3 header at pos: " << hdr_pos).str(), 4);

        if( prev_sample_rate && prev_sample_rate != sample_rate )
            ErrorAndExit( (str::stream() << "Variable sample rate in stream: " << prev_sample_rate
                           << " follows " << sample_rate).str(), 4);

        prev_sample_rate = sample_rate;
        frame_array.push_back(hdr_pos);
        //io::cout << ".";
    }

    ASSERT( !strm );
    if( !strm.eof() )
        ErrorAndExit((str::stream() << "Unknown error reading file: " << strm_fname).str(), 5);

    if( frame_array.empty() )
        ErrorAndExit((str::stream() << "File is empty: " << strm_fname).str(), 5);
    ASSERT( prev_sample_rate );

    // 2 точка разбиения
    double frame_time =  6 * 256 / (double)prev_sample_rate;
    double duration   = frame_array.size() * frame_time;
    io::cout << "Sound duration (sec): \t" << duration << io::endl;

    double split_point = sec_time / frame_time;
    if( frame_array.size() < split_point )
        ErrorAndExit("Split time is bigger than duration.", 6);
    int frame_num = int(round(split_point));
    io::cout << "Making split at time: \t" << frame_num * frame_time << io::endl;
    if( frame_num == (int)frame_array.size() )
        ErrorAndExit("No need for making split at the file end.", 6);

    // 3 запись
    ptr::shared<io::stream> dst1 = MakeDestStream(argv[3], 1);
    ptr::shared<io::stream> dst2 = MakeDestStream(argv[3], 2);

    io::pos tmp_pos = 0;
    ReadFunctor fnr = boost::lambda::bind(&FillBySplit, boost::lambda::_1, boost::lambda::_2, 
                                          boost::ref(*dst1), boost::ref(*dst2), 
                                          frame_array[frame_num], boost::ref(tmp_pos));

    strm.clear();
    strm.seekg(0, iof::beg);
    ReadAllStream(fnr, strm);

    return 0;
}

