//
// mdemux/tests/test_mpeg2.cpp
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

#include <mdemux/tests/_pc_.h>

#include "log.h"
#include <mdemux/util.h>

#include <mlib/string.h>
#include <mlib/format.h>
#include <mlib/lambda.h>
#include <mlib/sdk/logger.h>
#include <mlib/sdk/system.h>
#include <mlib/sdk/stream_util.h>
#include <mlib/tests/test_common.h>

#include <boost/function.hpp>

struct TestMpgData
{
    const char* mpgName;
    const char* m2vName;
           int  frameCnt;
};

const TestMpgData TestMpgArr[]=
{
    // чересстрочная, Frame Picture
    { 
        "/opt/src/test-mpegs/Autumn-c3.mpg",
        "/opt/src/test-m2vs/Autumn-c3.m2v",
        73
    },

    // не по порядку pts - у каждого кадра
    // чересстрочная, Frame Picture
    { 
        "/opt/src/test-mpegs/Billiard_01_1-c3.vob",
        "/opt/src/test-m2vs/Billiard_01_1-c3.m2v",
        77
    },

    { 
        "/opt/src/test-mpegs/elephantsdream-c3.mpg",
        "/opt/src/test-m2vs/elephantsdream-c3.m2v",
        75
    },

    // 2 аудио
    // чересстрочная, Top+Bottom
    { 
        "/opt/src/test-mpegs/MonaLisa_02_6-c3.vob",
        "/opt/src/test-m2vs/MonaLisa_02_6-c3.m2v",
        76
    },

    // прогрессивная послед.
    { 
        "/opt/src/test-mpegs/slideshow-c3.mpg",
        "/opt/src/test-m2vs/slideshow-c3.m2v",
        76
    },

    // не по порядку pts - у каждого кадра
    { 
        "/opt/src/test-mpegs/tvtuner_02-c3.mpg",
        "/opt/src/test-m2vs/tvtuner_02-c3.m2v",
        74
    },

    // есть разрыв 1.4>0.7 между pts
    { 
        "/opt/src/test-mpegs/tvtuner_03-c3.mpg",
        "/opt/src/test-m2vs/tvtuner_03-c3.m2v",
        74
    },

    { 0, 0, 0 } // признак конца
};

enum TestMpgIndex
{
    tmiAutumn,
    tmiBilliard,
    tmiElephantsDream,
    tmiMonaLisa,
    tmiSlideshow,
    tmiTvTuner2,
    tmiTvTuner3
};

namespace { void BoostCheckCanOpen(const char* fname)
{
    BOOST_CHECK_MESSAGE( CanOpen(fname), "CanOpen(\"" << fname << "\") failed!" );
} }

// Проверка на читаемость тестовых данных
// BOOST_AUTO_TEST_CASE( CheckTestData )
// {
//     for(const TestMpgData* mfile=TestMpgArr; mfile->mpgName; mfile++ )
//     {
//         BoostCheckCanOpen(mfile->mpgName);
//         BoostCheckCanOpen(mfile->m2vName);
//     }
// }

typedef boost::function<void(const char*, int idx)> TestDataFnr;

static void TestData(const char* fname, int idx, TestDataFnr fnr, bool to_announce = false)
{
    if( CanOpen(fname) )
    {
        if( to_announce )
        {
            LOG_INF << io::endl;
            LOG_INF << "#" << io::endl;
            LOG_INF << "################# " << fname << " #################" << io::endl;
            LOG_INF << "#" << io::endl;
            LOG_INF << io::endl;
        }
        fnr(fname, idx);
    }
}

void ForAllTestDataMpg(const TestMpgData* f_arr, TestDataFnr fnr)
{
    for(const TestMpgData* mfile=f_arr; mfile->mpgName; mfile++ )
        TestData(mfile->mpgName, mfile-f_arr, fnr, true);
}

void ForAllTestDataM2v(const TestMpgData* f_arr, TestDataFnr fnr)
{
    for(const TestMpgData* mfile=f_arr; mfile->mpgName; mfile++ )
        TestData(mfile->m2vName, mfile-f_arr, fnr, true);
}

/////////////////////////////////////////////////////////////////////////////
// Тест ValidateMpeg2_System

namespace Mpeg
{

//
// StatSvc - статистика по потоку MPEG System
// 

class SystemStatSvc: public SystemServiceDecor
{
    typedef SystemServiceDecor MyParent;
    public:
                    SystemStatSvc(Demuxer& dmx): MyParent(dmx), lastPts(INV_TS) 
                    {
                        Log::TsStats.Init();
                    }

     virtual  void  GetData(Demuxer& dmx, int len);
     virtual  void  OnPack(Demuxer& dmx);

    protected:
        double lastPts;
};

void SystemStatSvc::GetData(Demuxer& dmx, int len)
{
    double pts = dmx.CurPTS();
    if( lastPts != pts )
        Log::LogPts(pts);
    lastPts = pts;

    MyParent::GetData(dmx, len);
}

void SystemStatSvc::OnPack(Demuxer& dmx)
{
    Log::LogScr(dmx.CurSCR());
    MyParent::OnPack(dmx);
}

} // namespace Mpeg

void ValidateMpeg2_SystemImpl(const char* fname)
{
    io::stream strm(fname, iof::in);
    Mpeg::Demuxer dmx(&strm);
    dmx.SetStrict(true);

    Mpeg::SystemStatSvc svc(dmx);

    //strm.seekg(1, iof::cur);

    for( dmx.Begin(); dmx.NextState(); )
        ;

    if( dmx.ErrorReason() )
        BOOST_CHECK_MESSAGE( 0, "Error found [" << fname << "]: " << dmx.ErrorReason() << " !!" );
    else
        Mpeg::Log::PrintStats();
}

BOOST_AUTO_TEST_CASE( ValidateMpeg2_System )
{
    TestDataFnr vld_fnr = bl::bind(&ValidateMpeg2_SystemImpl, bl::_1);
    ForAllTestDataMpg(TestMpgArr, vld_fnr);
}

/////////////////////////////////////////////////////////////////////////////
// Тест ValidateMpeg2_Video

namespace Mpeg
{

//
// VideoStatSvc - статистика по видеопотоку MPEG Video
// 

class VideoStatSvc: public VideoServiceDecor
{
    typedef VideoServiceDecor MyParent;
    public:
                   VideoStatSvc(Decoder& dcr): MyParent(dcr) 
                   { picHdrCnt = endCnt = errCnt = 0; isEnd = true; }

     virtual void  TagData(Decoder& dcr, VideoTag tag);
             void  PrintVStats();
                   // послед. картинку не полностью получили, за исключением
                   // isEnd
              int  PicCnt() { return picHdrCnt - (isEnd ? 0 : 1); }

    protected:
            int  picHdrCnt;
            int  endCnt;
            int  errCnt;
           bool  isEnd;
};

void VideoStatSvc::TagData(Decoder& dcr, VideoTag tag)
{
    switch( tag )
    {
    case vtFRAME_FOUND:
        isEnd = false;
        picHdrCnt++;
        break;
    case vtEND:
        isEnd = true;
        endCnt++;
        break;
    case vtERROR:
        errCnt++;
        break;
    default:
        break;
    }
    MyParent::TagData(dcr, tag);
}

void VideoStatSvc::PrintVStats()
{
    LOG_INF << "Frame Count: " << PicCnt() << io::endl;
    LOG_INF << "Video Error Count: " << errCnt << io::endl;
    LOG_INF << "Video Ends Found:  " << endCnt << io::endl;
    LOG_INF << "Video-stream decoded successfully." << io::endl; 
}

} // namespace Mpeg

void ValidateMpeg2_VideoImpl(const char* fname, int idx)
{
    io::stream strm(fname, iof::in);
    //io::stream strm("../Autumn.m2v", iof::in);
    //io::stream strm("../mona.m2v", iof::in);

    char buf[100];
    Mpeg::Decoder dcr;
    Mpeg::VideoStatSvc svc(dcr);
    dcr.Begin();

    for( Mpeg::DecodeType stt ; stt = dcr.NextState(), true ; )
    {
//         // ошибкоустойчивость
//         static bool is_first = true;
//         if( is_first && dcr.seqInf.IsInit() && dcr.DatPos()<0x30 )
//         {
//             is_first = false;
//
//             dcr.Begin();
//             strm.seekg(0x30);
//             continue;
//         }
        if( stt == Mpeg::dtBUFFER )
        {
            int cnt = strm.raw_read(buf, sizeof(buf));
            if( !cnt )
                break;
            dcr.Feed(buf, buf+cnt);
        }
    }
    BOOST_CHECK_EQUAL( TestMpgArr[idx].frameCnt, svc.PicCnt() );
    svc.PrintVStats();
}

BOOST_AUTO_TEST_CASE( ValidateMpeg2_Video )
{
    TestDataFnr vld_fnr = bl::bind(&ValidateMpeg2_VideoImpl, bl::_1, bl::_2);
    ForAllTestDataM2v(TestMpgArr, vld_fnr);
}

/////////////////////////////////////////////////////////////////////////////
// Тест VideoLineTest

static void FlushStrStream(str::stream& strm, bool flush = false)
{
    if( flush )
    {
        LOG_INF << strm.str() << io::endl;
        strm.str( std::string() );
    }
}

void CheckVideoLine(Mpeg::VideoLine& vl, io::stream& strm)
{
    using namespace Mpeg;
    FrameList& fram_lst = vl.GetFrames();
    bool is_first = true;

    str::stream str;
    for( FrameList::PhisItr it = fram_lst.PhisBeg(), end = fram_lst.PhisEnd(); it != end; ++it )
    {
        FrameData& fd = *it;
        if( fd.opt&fdHEADER )
        {
            FlushStrStream(str, true);
            FlushStrStream(str << "HEADER", true);
        }
        if( fd.opt&fdGOP )
        {
            if( !(fd.opt&fdHEADER) )
                FlushStrStream(str, true);
            FlushStrStream(str << "GOP", true);
        }
        FlushStrStream(str << fd.typ);

        bool is_full = !fd.dat.empty();
        if( is_full && is_first )
            BOOST_ASSERT( IsTSValid(fd.pts) );
        is_first = false;

        BOOST_ASSERT( is_full || (it+1 == end) );
        if( is_full )
        {
            char buf[4];
            char* cur = buf;
            for( FrameData::ChunkList::iterator it = fd.dat.begin(), end = fd.dat.end(); it != end; ++it )
            {
                Chunk& chk = *it;
                int rest = std::min(4+int(buf-cur), chk.len);
                if( rest )
                {
                    strm.seekg(chk.extPos);
                    int cnt = strm.raw_read(cur, rest);
                    BOOST_REQUIRE( cnt == rest );

                    cur += rest;
                }
                else
                    break;
            }

            BOOST_REQUIRE( cur-buf == 4 );
            uint32_t code = htonl(*(uint32_t*)buf);

            if( fd.opt&fdHEADER )
                BOOST_ASSERT( code == vidSEQ_HDR );
            else if( fd.opt&fdGOP )
                BOOST_ASSERT( code == vidGOP );
            else
                BOOST_ASSERT( code == vidPIC );
        }
    }
    FlushStrStream(str, true);
}

void VideoLineTestImpl(const char* fname)
{
    io::stream strm(fname, iof::in);

    io::pos sz = StreamSize(strm);
    Mpeg::ParseContext cont(strm);
    Mpeg::VideoLine vl(cont);

    cont.dmx.SetStrict(true);
    vl.MakeForPeriod(0, std::min(Mpeg::BoundDecodeSize, (int)sz));
    CheckVideoLine(vl, strm);

    LOG_INF << "=====" << io::endl;

    cont.dmx.SetStrict(false);
    vl.MakeForPeriod(std::max(0, (int)sz-Mpeg::BoundDecodeSize), sz);

    CheckVideoLine(vl, strm);
}

// VideoLine
BOOST_AUTO_TEST_CASE( VideoLineTest )
{
    TestDataFnr vld_fnr = bl::bind(&VideoLineTestImpl, bl::_1);
    ForAllTestDataMpg(TestMpgArr, vld_fnr);
}

/////////////////////////////////////////////////////////////////////////////
// Тест Mpeg2SystemVideoTest

static void PrintDecoderState(Mpeg::Decoder& dcr)
{
    Mpeg::SequenceData& seq = dcr.seqInf;
    LOG_INF << "Progressive sequence: " << seq.isProgr << io::endl;
    LOG_INF << "Size (width*height):\t" << seq.wdh << "*" << seq.hgt << io::endl;
    LOG_INF << "Frame rate (frames/c):\t" << seq.framRat.x/(double)seq.framRat.y << io::endl;
}

void Mpeg2SystemVideoTestImpl(const char* fname, int idx)
{
    io::stream strm(fname, iof::in);

    io::pos sz = StreamSize(strm);
    Mpeg::ParseContext cont(strm);
    Mpeg::VideoLine vl(cont);

    int mem_before = GetMemSize();
    vl.MakeForPeriod(0, sz);
    //vl.MakeForPeriod(0, 1000000);
    int mem_after = GetMemSize();
    int fram_sz = vl.GetFrames().PhisSize();
    BOOST_CHECK_EQUAL(fram_sz, TestMpgArr[idx].frameCnt);

    PrintDecoderState(cont.dcr);

    LOG_INF << io::endl;
    LOG_INF << "File size: " << sz << io::endl;
    LOG_INF << "Frames count: " << fram_sz << io::endl;
    LOG_INF << "Memory before/after/after-before: " << mem_before << ", " << mem_after << ", " << mem_after-mem_before
            << io::endl;
    LOG_INF << "Ratio mem/size: " << (double)(mem_after-mem_before)/sz << io::endl;
    LOG_INF << "Ratio mem/frames (bytes/frames): " << (double)(mem_after-mem_before)/vl.GetFrames().PhisSize() << io::endl;
}

// MPEG2 System+Video
BOOST_AUTO_TEST_CASE( Mpeg2SystemVideoTest )
{
    TestDataFnr vld_fnr = bl::bind(&Mpeg2SystemVideoTestImpl, bl::_1, bl::_2);
    ForAllTestDataMpg(TestMpgArr, vld_fnr);
}

/////////////////////////////////////////////////////////////////////////////
// Тест MediaInfoTest

void MediaInfoTestImpl(const char* fname)
{
    Mpeg::PlayerData pd;
    bool res = Mpeg::GetInfo(pd, fname);

    Mpeg::MediaInfo& inf = pd.mInf;
    if( res )
    {
        LOG_INF << "File is MPEG2 compatible:" << io::endl;
        LOG_INF << "Time (begin - end): \t" << Mpeg::SecToHMS(inf.begTime) << " - " << Mpeg::SecToHMS(inf.endTime) << io::endl;
        LOG_INF << "Size (width*height):\t" << inf.vidSeq.wdh << "*" << inf.vidSeq.hgt << io::endl;
        LOG_INF << "Frame rate (frames/c):\t" << inf.vidSeq.framRat.x/(double)inf.vidSeq.framRat.y << io::endl;
    }
    else
        BOOST_CHECK_MESSAGE(0, "Error while opening the file: " << inf.ErrorReason() << ".");

}

BOOST_AUTO_TEST_CASE( MediaInfoTest )
{
    TestDataFnr vld_fnr = bl::bind(&MediaInfoTestImpl, bl::_1);
    ForAllTestDataMpg(TestMpgArr, vld_fnr);
}

/////////////////////////////////////////////////////////////////////////////
// Тест MakeForPeriodTest

BOOST_AUTO_TEST_CASE( MakeForPeriodTest )
{
    const char* fname = TestMpgArr[tmiMonaLisa].mpgName;
    if( !CanOpen(fname) )
        return;

    Mpeg::PlayerData pd;
    Mpeg::MediaInfo& inf = pd.mInf;
    if( !Mpeg::GetInfo(pd, fname) )
    {
        BOOST_CHECK_MESSAGE(0, "MediaInfo Error: " << inf.ErrorReason());
        return;
    }
    LOG_INF << io::endl;
    LOG_INF << "Time (begin-end): "  << Mpeg::SecToHMS(inf.begTime) << " - " << Mpeg::SecToHMS(inf.endTime) << io::endl;

    double time = inf.FrameTime(inf.FramesCount()-1); // секунд
    Mpeg::VideoLine vl(pd.prsCont);
    if( Mpeg::MakeForTime(vl, time, inf) )
    {
        Mpeg::FrameList& fl = vl.GetFrames();
        BOOST_REQUIRE( fl.IsPlayable() );

        LOG_INF << "Seek to time: "   << time << " (sec) is done" << io::endl;
        LOG_INF << "VideoLine info :" << io::endl;
        LOG_INF << "\tPlay period: "  << Mpeg::SecToHMS(fl.TimeBeg()) << " - " 
                                       << Mpeg::SecToHMS(fl.TimeEnd()) << io::endl;
        LOG_INF << "\tFrame count: "  << fl.PhisSize() << io::endl;

    }
    else
        BOOST_CHECK_MESSAGE(0, "Cant seek to time: " << time << " (sec)");
}

/////////////////////////////////////////////////////////////////////////////
// Тест PlayerViewTest

static FILE* open_check_file(const char* filename)
{
    FILE* file = fopen(filename, "wb");
    if( !file )
    {
        io::cerr << boost::format("Could not open file \"%1%\".\n") % filename % bf::stop;
        exit(1);
    }
    return file;
}

void save_pgm(const char* filename, int width, int height,
                      int chroma_width, int chroma_height,
                      uint8_t * const * buf)
{
    static uint8_t black[16384] = { 0};

    FILE* pgmfile = open_check_file(filename);

    fprintf (pgmfile, "P5\n%d %d\n255\n",
             2 * chroma_width, height + chroma_height);
    int i;
    for(i = 0; i < height; i++)
    {
        fwrite (buf[0] + i * width, width, 1, pgmfile);
        fwrite (black, 2 * chroma_width - width, 1, pgmfile);
    }
    for(i = 0; i < chroma_height; i++)
    {
        fwrite (buf[1] + i * chroma_width, chroma_width, 1, pgmfile);
        fwrite (buf[2] + i * chroma_width, chroma_width, 1, pgmfile);
    }
    fclose (pgmfile);
}

void save_ppm(const char* filename, int width, int height, uint8_t* buf)
{
    FILE* ppmfile = open_check_file(filename);

    fprintf (ppmfile, "P6\n%d %d\n255\n", width, height);
    fwrite (buf, 3 * width, height, ppmfile);
    fclose (ppmfile);
}

static void ShowFile(const std::string& fname)
{
    std::string cmd = "eog " + fname;
    system(cmd.c_str());
}

void ShowRGB24(int width, int height, uint8_t* buf)
{
    TmpFileNames tfn;
    std::string fname = tfn.CreateName();
    save_ppm(fname.c_str(), width, height, buf);

    ShowFile(fname);
}

// показать YCbCr-изображение отдельно по плоскостям, в оттенках серого
void ShowImage(Mpeg::Player& plyr)
{
    Mpeg::SequenceData& seq = plyr.MInfo().vidSeq;
    BOOST_ASSERT( plyr.IsPlaying() );
    BOOST_ASSERT( seq.chromaFrmt == Mpeg::ct420 );

    int wdh = seq.wdh;
    int hgt = seq.hgt;
    TmpFileNames tfn;
    std::string fname = tfn.CreateName();
    save_pgm(fname.c_str(), wdh, hgt, wdh/2, hgt/2, plyr.Data());
    
    ShowFile(fname);
}

void PlayerViewTestImpl(const char* fname)
{
    Mpeg::FwdPlayer plyr;
    CheckOpen(plyr, fname);

    Mpeg::MediaInfo inf = plyr.MInfo();
    int n = inf.FramesCount();

    int idx_arr[] = { 0, n/2, n-1 };
    for( int i=0; i<(int)ARR_SIZE(idx_arr); i++ )
    {
        BOOST_ASSERT( plyr.SetTime( inf.FrameTime(idx_arr[i]) ) );

        //ShowImage(plyr);
    }
}

BOOST_AUTO_TEST_CASE( PlayerViewTest )
{
    TestDataFnr vld_fnr = bl::bind(&PlayerViewTestImpl, bl::_1);
    ForAllTestDataMpg(TestMpgArr, vld_fnr);
}

/////////////////////////////////////////////////////////////////////////////
// Тест PlayerViewTest

// :TODO: это долгий тест, по умолчанию не включаем;
// реализовать декларацию функции BOOST_AUTO_TEST_CASE_LOOOONG()
BOOST_AUTO_TEST_CASE( PlayerVLMaxTest )
{
    return;

    Mpeg::FwdPlayer plyr("../Autumn.mpg");
    BOOST_REQUIRE( plyr.IsOpened() );

    //Magick::Image img;
    Mpeg::FrameList& f_lst = plyr.VLine().GetFrames();
    const int autumn_frame_cnt = 1048;

    int cnt = 0;
    plyr.First();
    //Mpeg::FrameList::Itr fix_itr = f_lst.Beg();

    int beg_msz      = GetMemSize();
    double beg_clock = GetClockTime();

    for( ; !plyr.IsDone(); plyr.Next(), cnt++ )
    {
        BOOST_REQUIRE( f_lst.End() - f_lst.Beg() <= Mpeg::MaxFrameListLength );

        //ShowImage(plyr);
    }
    BOOST_CHECK_EQUAL( cnt , autumn_frame_cnt );

    double end_clock = GetClockTime();
    int end_msz       = GetMemSize();

    double time = end_clock-beg_clock;
    LOG_INF << io::endl;
    LOG_INF << "Memory before/after/after-before: " << beg_msz << ", " << end_msz << ", " << end_msz-beg_msz << io::endl;
    LOG_INF << "Decoding speed, frames/s (fps):   " << autumn_frame_cnt/time << io::endl; 
}

/////////////////////////////////////////////////////////////////////////////
// Тест PlayerCoverageTest

static void MoveToTime(Mpeg::Player& plyr, double time)
{
    BOOST_CHECK_MESSAGE( plyr.SetTime(time), "Cant move to time: " << Mpeg::SecToHMS(time) );
}

BOOST_AUTO_TEST_CASE( PlayerCoverageTest )
{
    const char* fname = TestMpgArr[tmiMonaLisa].mpgName;
    if( !CanOpen(fname) )
        return;

    Mpeg::FwdPlayer plyr(fname);
    BOOST_REQUIRE( plyr.IsOpened() );

    Mpeg::MediaInfo inf = plyr.MInfo();
    int n = inf.FramesCount();

    MoveToTime(plyr, inf.FrameTime(0));
    MoveToTime(plyr, inf.FrameTime(n-1));
    for( ; !plyr.IsDone(); plyr.Next() )
        ;

    MoveToTime(plyr, 6347.01); // 55-й
    MoveToTime(plyr, 6344.97); // 4-й
    MoveToTime(plyr, 6344.81); // 0-й
    MoveToTime(plyr, 6347.01); // 55-й
}

//
// Разное 
// 

BOOST_AUTO_TEST_CASE( PlayerOpenClose )
{
    const char* fname = TestMpgArr[tmiMonaLisa].mpgName;
    if( !CanOpen(fname) )
        return;

    Mpeg::FwdPlayer plyr;
    BOOST_CHECK( !plyr.IsOpened() );

    CheckOpen(plyr, fname);

    plyr.Close();
    BOOST_CHECK( !plyr.IsOpened() );
}

