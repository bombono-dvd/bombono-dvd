//
// mdemux/mpeg2demux.cpp
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

#include <mdemux/mpeg.h>

#include <map>
#include <strings.h> // strcasecmp()

#include <mlib/read_stream.h>
#include <mlib/ptr.h>
#include <mlib/string.h>
#include <mlib/filesystem.h> // get_extension()
#include <mlib/lambda.h>

//
// mpeg2demux - утилита-демиксер PS MPEG
// 

namespace Mpeg {

static bool IsPrivate1Data(uint32_t code)
{
    return code == sidPRIV1;
}

class DemuxSvc: public Service
{
    public:
                      DemuxSvc(const std::string& dst, bool as_vob): dstName(dst), 
                          asVob(as_vob) {}

        virtual bool  Filter(uint32_t code);
        virtual void  GetData(Mpeg::Demuxer& dmx, int len);

    protected:
        typedef std::map<std::string, ptr::shared<io::stream> > StreamMap;
                  StreamMap  strmMap;

                std::string  dstName;
                       bool  asVob; // звук из Private Stream 1 вынимать (для .vob-файлов)

bool IsVobData(uint32_t code)
{
    return asVob && IsPrivate1Data(code);
}

};

bool DemuxSvc::Filter(uint32_t code)
{
    return IsContentCode(code) || IsVobData(code);
}

static std::string MakePESKey(int id, const char* ext)
{
    return (str::stream() << id << "." << ext).str();
}

static bool ReadPart(io::stream& strm, uint8_t* buf, int sz, int& len)
{
    bool res = false;
    if( (len >= sz) && (sz == strm.raw_read((char*)buf, sz)) )
    {
        res = true;
        len -= sz;
    }
    return res;
}

// в формате опции -L для mplex
static std::string MakeKeyNameForLPCM(int track, uint8_t inf)
{
    int sample_rate = (inf & 0x10) ? 96000 : 48000;
    int channels    = (inf & 0x07) + 1;
    const char* bps;
    switch( inf >> 6 )
    {
    case 0:
        bps = "16";
        break;
    case 1:
        bps = "20";
        break;
    case 2:
        bps = "24";
        break;
    case 3:
        bps = "unknown"; // неправильно, ошибка в потоке
        io::cerr << "Cant get 'bits per sample' value for lpcm audio stream: " << track << io::endl;
        break;
    default:
        ASSERT(0);
    }

    std::string header_str = (str::stream() << sample_rate << ":" << channels << ":" << bps << ".lpcm").str();
    return MakePESKey(track, header_str.c_str());
}

void DemuxSvc::GetData(Mpeg::Demuxer& dmx, int len)
{
    uint32_t cur_code = dmx.FilterCode();
    ASSERT( cur_code != sidFF );

    std::string key;
    typedef boost::function<std::string()> StringFnr;
    StringFnr get_key_name; // отложенное получение инфо ради lpcm

    if( IsAudioCode(cur_code) )
        key = MakePESKey(cur_code - sidAUDIO_BEG, "mp2");
    else if( IsVideoCode(cur_code) )
        key = MakePESKey(cur_code - sidVIDEO_BEG, "m2v");
    else
    {
        ASSERT( IsVobData(cur_code) );
        // для DVD в private stream 1 лежит звук и субтитры, 
        // Doc: DVD-Video/1. Mpucoder Specs/DVD/ass-hdr.html (= www.mpucoder.com/DVD/ass-hdr.html)

        io::stream& strm = dmx.ObjStrm();
        // берем только звук пока
        uint8_t buf[4];
        if( !ReadPart(strm, buf, 4, len) )
            return;

        uint8_t strm_id = buf[0];
        int track       = strm_id & 0x0F;
        if( (strm_id & 0xF0) == 0x80 )      // ac3 & dts
        {
            bool is_ac3 = true; // ac3 vs dts
            if( track > 7 )
            {
                is_ac3 = false;    
                track -= 8;
            }
            key = MakePESKey(track, is_ac3 ? "ac3" : "dts");
        }
        else if( (strm_id & 0xF8) == 0xA0 ) // lpcm
        {
            if( !ReadPart(strm, buf, 3, len) )
                return;

            get_key_name = boost::lambda::bind(&MakeKeyNameForLPCM, track, buf[1]);
            key = MakePESKey(track, "lpcm");
        }
        else
            return;
    }
    ASSERT( !key.empty() );

    StreamMap::iterator itr = strmMap.find(key);
    if( itr == strmMap.end() )
    {
        // создаем нужный поток
        std::string fname = dstName + "." + (get_key_name ? get_key_name() : key);
        ptr::shared<io::stream> strm(new io::stream(fname.c_str(), iof::out));
        if( !*strm )
            io::cerr << "Cant write to file " << fname.c_str() << "." << io::endl;

        itr = strmMap.insert(std::make_pair(key, strm)).first;
    }
    ASSERT( itr != strmMap.end() );

    io::stream& dst_strm = *itr->second;
    if( dst_strm )
        ReadStream(MakeWriter(dst_strm), dmx.ObjStrm(), len);
}

} // namespace Mpeg

int main(int argc, char *argv[])
{
    if( argc<3 )
    {
        io::cerr << "Usage: \t mpeg2demux src-file(.mpg, .vob) dst-file-w/o-extension" << io::endl;
        io::cerr << "\t Demux video from src-file(.mpg, .vob). If src-file has .vob\n"
                    "\t extension then ac3/dts/lpcm audios are to be pulled too.\n\n" 
                    "\t If there is a lpcm audio in src-file it will have info \n"
                    "\t for mplex muxer in its name, how to mux with -L option." << io::endl;
        return 1;
    }

    const char* src_fname = argv[1];
    io::stream src_strm(src_fname, iof::in);
    if( !src_strm )
    {
        io::cerr << "Cant open file " << argv[1] << "." << io::endl;
        return 2;
    }

    Mpeg::DemuxSvc wv(argv[2], strcasecmp(get_extension(src_fname).c_str(), "VOB") == 0);
    Mpeg::Demuxer dmx(&src_strm, &wv);
    // в начале требуем без ошибок  
    dmx.SetStrict(true);

    for( dmx.Begin(); dmx.NextState(); )
        if( src_strm.tellg() > 100000 )
        {
            dmx.SetStrict(false);
        }
    if( !src_strm.eof() )
    {
        io::cerr << "Error: " << dmx.ErrorReason() << io::endl;
        return 3;
    }

    return 0;
}

