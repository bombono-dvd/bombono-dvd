//
// mdemux/tests/lpcm2wav.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009 Ilya Murav'jov
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

#include <mlib/mlib.h>
#include <mlib/lambda.h>
#include <mlib/read_stream.h>     // ReadStream()
#include <mlib/sdk/stream_util.h> // StreamSize()

#include <mdemux/trackbuf.h>

#include <boost/static_assert.hpp>
#include <byteswap.h>             // bswap_NN(), в POSIX отсутствует
#include <sstream>


#ifdef HAS_BIG_ENDIAN
#   define be2me_16(x) (x)
#   define be2me_32(x) (x)
#   define le2me_16(x) bswap_16(x)
#   define le2me_32(x) bswap_32(x)
#else // HAS_LITTLE_ENDIAN
#   define be2me_16(x) bswap_16(x)
#   define be2me_32(x) bswap_32(x)
#   define le2me_16(x) (x)
#   define le2me_32(x) (x)
#endif

//
// часть кода сперта из ao_pcm.c, MPlayer
//

// см. документацию на WAV здесь, http://www.microsoft.com/whdc/device/audio/multichaud.mspx

#define WAV_ID_RIFF 0x46464952 /* "RIFF" */
#define WAV_ID_WAVE 0x45564157 /* "WAVE" */
#define WAV_ID_FMT  0x20746d66 /* "fmt " */
#define WAV_ID_DATA 0x61746164 /* "data" */
#define WAV_ID_PCM  0x0001

#include PACK_ON
struct PCMWaveHeader
{
	uint32_t riff;
	uint32_t fileLen;
	uint32_t wave;
	uint32_t frmt;
	uint32_t frmtLen;
	uint16_t frmtTag;
	uint16_t channels;
	uint32_t sampleRate;
	uint32_t bytesPerSecond;
	uint16_t blockAlign;
	uint16_t bits;
	uint32_t data;
	uint32_t dataLen;
};
#include PACK_OFF

const int PCM_WAVE_HEADER_SIZE = 44;
BOOST_STATIC_ASSERT( PCM_WAVE_HEADER_SIZE == sizeof(PCMWaveHeader) );

static bool ParsePcmTriple(const char* triple, PCMWaveHeader& hdr)
{
    std::string buf(triple);
    for( int i=0; i<(int)buf.size(); i++ )
        if( buf[i] == ':' )
            buf[i] = ' ';
    std::stringstream ss(buf, std::ios::in);
    ss >> hdr.sampleRate >> hdr.channels >> hdr.bits;

    bool res = false;
    switch( hdr.bits )
    {
    case 16: case 20: case 24:
        res = true;
    }

    if( !res )
        io::cerr << "Incorrect bps value (must be 16, 20, 24): " << hdr.bits << io::endl;
    return res;
}

typedef boost::function<bool(TrackBuf& tb)> TBReadFunctor;

bool ReadStreamThroughTB(TBReadFunctor fnr, io::stream& strm, io::pos len)
{
    bool is_break = false;
    TrackBuf buf;
    for( int cnt, old_cnt; ; len -= cnt )
    {
        cnt     = std::min(len, (io::pos)STRM_BUF_SZ);
        old_cnt = buf.Size(); 
        buf.AppendFromStream(strm, cnt);

        cnt = buf.Size()-old_cnt;
        if( cnt <= 0 )
            break; 

        if( !fnr(buf) )
        {
            is_break = true;
            break;
        }
        if( buf.Size() > 3*STRM_BUF_SZ )
            Error("ReadStreamThroughTB: Destination is too passive!");
    }
    return is_break;
}

static void WriteOutData(io::stream& dst, char* data, TrackBuf& buf, int len)
{
    if( len > 0 )
    {
        dst.write(data, len);
        buf.CutStart(len);
    }
}

static bool Swap16AndWrite(TrackBuf& buf, io::stream& dst)
{
    int i = 0;
    char* data = buf.Beg();
    for( ; i < buf.Size()-1; i += 2 )
        std::swap(data[i], data[i+1]);

    WriteOutData(dst, data, buf, i);
    return true;
}

// если временно нужно место для работы
static char* ReserveSameSize(TrackBuf& buf)
{
    buf.Reserve(buf.Size()*2);
    return buf.End();
}

// 
// COPY_N_PASTE_ETALON
// Алгоритмы для 20 и 24 бит взяты как есть из (исходника PgcDemux.cpp) программы PgcDemux,
// http://download.videohelp.com/jsoto/dvdtools.htm
// 
// От себя добавлю,- какой урод додумался так байты намешать, и зачем!?

static bool Reconstruct20bitLPCM(TrackBuf& buf, int ncha, io::stream& dst)
{
#define hi_nib(a)	((a>>4) & 0x0f)
#define lo_nib(a)	(a & 0x0f)

    int j = 0;
    char* buffer   = buf.Beg();
    char* mybuffer = ReserveSameSize(buf);
    for( ; j < (buf.Size()-5*ncha+1) ; j+=(5*ncha) )
    {
        for( int i=0; i<ncha; i++ )
        {
            mybuffer[j+5*i+0] = (hi_nib(buffer[j+4*ncha+i])<<4) + hi_nib(buffer[j+4*i+1]);
            mybuffer[j+5*i+1] = (lo_nib(buffer[j+4*i+1])<<4) + hi_nib(buffer[j+4*i+0]);
            mybuffer[j+5*i+2] = (lo_nib(buffer[j+4*i+0])<<4) + lo_nib(buffer[j+4*ncha+i]);
            mybuffer[j+5*i+3] = buffer[j+4*i+3];
            mybuffer[j+5*i+4] = buffer[j+4*i+2];
        }
    }
    
    WriteOutData(dst, mybuffer, buf, j);
    return true;
}

static bool Reconstruct24bitLPCM(TrackBuf& buf, int ncha, io::stream& dst)
{
    int j = 0;
    char* buffer   = buf.Beg();
    char* mybuffer = ReserveSameSize(buf);
    for( ; j < (buf.Size()-6*ncha+1) ; j+=(6*ncha) )
    {
        for( int i=0; i<2*ncha; i++ )
        {
            mybuffer[j+3*i+2]=buffer[j+2*i];
            mybuffer[j+3*i+1]=buffer[j+2*i+1];
            mybuffer[j+3*i]=  buffer[j+4*ncha+i];
        }
    }
    
    WriteOutData(dst, mybuffer, buf, j);
    return true;
}

int main(int argc, char *argv[])
{
    if( argc != 4 )
    {
        io::cerr << "Usage: \t lpcm2wav sample_rate:channels:bps file.lpcm out_file.wav" << io::endl;
        io::cerr << "\t lpcm2wav makes wav from dvd lpcm stream."                        << io::endl;

        return 1;
    }

    PCMWaveHeader hdr;
    if( !ParsePcmTriple(argv[1], hdr) )
    {
        io::cerr << "Can't get correct sample_rate:channels:bps triple!" << io::endl;
        return 2;
    }
    uint32_t samplerate = hdr.sampleRate;
    uint16_t channels   = hdr.channels;
    uint16_t bits       = hdr.bits;

    io::cerr << "Sample rate (Hz):\t " << samplerate  << io::endl;
    io::cerr << "Number of channels:\t "  << channels << io::endl;
    io::cerr << "Sample size (bit):\t "   << bits     << io::endl;

    io::stream strm(argv[2], iof::in);
    if( !strm )
    {
        io::cerr << "Can't open file: " << argv[2] << io::endl;
        return 3;
    }

    io::pos file_big_sz = StreamSize(strm);
    if( file_big_sz > UINT32_MAX-100 ) // ограничение размера wav-файла 32битами
    {
        io::cerr << "Can't do wav-file because " << argv[2] << " is too big, > 3,9Gb." << io::endl;
        return 4;
    }
    uint32_t file_sz = file_big_sz;

    // заполняем оставшееся, c учетом little endian
    hdr.riff    = le2me_32(WAV_ID_RIFF);
    hdr.fileLen = le2me_32(file_sz + PCM_WAVE_HEADER_SIZE - 8);
    hdr.wave    = le2me_32(WAV_ID_WAVE);

    hdr.frmt     = le2me_32(WAV_ID_FMT);
    hdr.frmtLen  = le2me_32(16);
    hdr.frmtTag  = le2me_16(WAV_ID_PCM);
    hdr.channels = le2me_16(channels);
    hdr.sampleRate     = le2me_32(samplerate);
    hdr.bytesPerSecond = le2me_32(channels * samplerate * bits / 8);
    hdr.blockAlign     = le2me_16(channels * bits / 8);
    hdr.bits     = le2me_16(bits);

    hdr.data     = le2me_32(WAV_ID_DATA);
    hdr.dataLen  = le2me_32(file_sz);
    
    // запись
    io::stream wav_strm(argv[3], iof::out);
    if( !wav_strm )
    {
        io::cerr << "Can't write to file: " << argv[3] << io::endl;
        return 5;
    }

    wav_strm.raw_write((const char*)&hdr, PCM_WAVE_HEADER_SIZE);
    const char* incomplete_samples = "Error: Number of PCM samples is incomplete.";
    switch( bits )
    {
    case 16:
        {
            if( file_sz%2 )
                io::cerr << incomplete_samples << io::endl;
            TBReadFunctor fnr = bl::bind(&Swap16AndWrite, bl::_1, boost::ref(wav_strm));
            ReadStreamThroughTB(fnr, strm, file_sz);
        }
        break;
    case 20:
        {
            if( file_sz%(5*channels) )
                io::cerr << incomplete_samples << io::endl;
            TBReadFunctor fnr = bl::bind(&Reconstruct20bitLPCM, bl::_1, channels, boost::ref(wav_strm));
            ReadStreamThroughTB(fnr, strm, file_sz);
        }
        break;
    case 24:
        {
            if( file_sz%(6*channels) )
                io::cerr << incomplete_samples << io::endl;
            TBReadFunctor fnr = bl::bind(&Reconstruct24bitLPCM, bl::_1, channels, boost::ref(wav_strm));
            ReadStreamThroughTB(fnr, strm, file_sz);
        }
        break;
    default:
        ASSERT(0);
    }
    return 0;
}

