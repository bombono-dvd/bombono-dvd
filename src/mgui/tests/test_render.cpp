//
// mgui/tests/test_render.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

#include <mgui/tests/_pc_.h>

#include "test_author.h"
#include "test_mbrowser.h"

//#include <mgui/sdk/player_utils.h>
#include <mgui/execution.h>
#include <mgui/init.h>
#include <mgui/author/ffmpeg.h>
#include <mgui/project/video.h>

#include <mgui/ffviewer.h>
#include <mgui/dialog.h>
#include <mgui/execution.h> // PipeOutput()

#include <mlib/format.h>

namespace Project {

BOOST_AUTO_TEST_CASE( TestRenderTranscoding )
{
    return;
    //std::string fname   = "/home/ilya/opt/programming/atom-project/AV-Samples/Autumn.mpg";
    //std::string a_fname = "/home/ilya/opt/programming/atom-project/AV-Samples/Transition.mpg";
    std::string fname   = "../AV-Samples/Hellboy_1080p-mpeg2.ts";
    std::string a_fname = "../AV-Samples/Hellboy_1080p-mpeg2.ts";

    //Mpeg::FwdPlayer plyr;
    FFViewer plyr;
    RGBOpen(plyr, fname);

    bool is_pal = FrameFPS(plyr) == 25.0; // plyr.MInfo().vidSeq.hgt == 576;

    int fps = 30;
    double shift = 1. / fps;
    double end  = 10.; // секунда

    std::string ffmpeg_cmd = boost::format("ffmpeg -r %1% -f image2pipe -vcodec ppm -i pipe: %2%")
        % fps % FFmpegPostArgs("../dvd_out/trans.vob", false, is_pal, a_fname, 6) % bf::stop;
    io::cout << ffmpeg_cmd << io::endl;

    ExitData ed;
    {
        FFmpegCloser pc(ed);
        pc.pid = Spawn(0, ffmpeg_cmd.c_str(), 0, true, &pc.inFd);
        ASSERT( pc.inFd != NO_HNDL );
    
        int i = 0;
        PPMWriter ppm_writer(pc.inFd);
        for( double cur = 0; cur < end; cur += shift, i++ )
        {
            RefPtr<Gdk::Pixbuf> img = GetRawFrame(cur, plyr);
            ASSERT( img ); 
    
            ppm_writer.Write(img);
        }
    }
}

static void RunFFmpeg(const std::string& ffmpeg_cmd)
{
    io::cout << ffmpeg_cmd << io::endl;
    //Execution::SimpleSpawn(ffmpeg_cmd.c_str());
    ExitData ed = System(ffmpeg_cmd);
    BOOST_CHECK( ed.IsGood() );
}

BOOST_AUTO_TEST_CASE( TestStillTranscoding )
{
    return;
    std::string fname = "tools/test-data/flower.jpg";

    // в случае автономной работы ffmpeg указываем длительность аргументом
    double duration = 15; // 0.1;
    std::string ffmpeg_cmd = boost::format("ffmpeg -t %3$.2f -loop_input -i \"%1%\" %2%") 
        % fname % FFmpegPostArgs("../dvd_out/trans.vob", false, true) % duration % bf::stop;
 
    RunFFmpeg(ffmpeg_cmd);
}

BOOST_AUTO_TEST_CASE( TestDVDTranscoding )
{
    return;
    const char* src_fname = "../AV-Samples/ЧастноеВидео_dv.avi";
    std::string ffmpeg_cmd = FFmpegToDVDTranscode(src_fname, "../dvd_out/trans.vob", false, true,
                                                  DVDDims2TDAuto(dvd720));
    RunFFmpeg(ffmpeg_cmd);
}

static AStores& LoadTestPrj()
{
    return InitAndLoadPrj(TestProjectPath());
}

BOOST_AUTO_TEST_CASE( TestMenuSettings )
{
    return;
    GtkmmDBInit gd_init;
    RefPtr<MenuStore> ms = LoadTestPrj().mnStore;
    ASSERT( Size(ms) );

    Menu mn = GetMenu(ms, ms->children().begin());

    DialogParams MenuSettingsDialog(Menu mn, Gtk::Widget* par_wdg);
    DoDialog(MenuSettingsDialog(mn, 0));
}

BOOST_AUTO_TEST_CASE( TestCheckFFmpeg )
{
    std::string conts = GetTestFNameContents("ffmpeg05_valid_dvd_encoders.txt");
    //std::string conts = Glib::file_get_contents("/home/ilya/opt/programming/atom-project/hardy_formats_.txt");
    //std::string conts = Glib::file_get_contents("/home/ilya/opt/programming/atom-project/lucid_formats.txt");

    void TestFFmpegForDVDEncoding(const std::string& conts);
    TestFFmpegForDVDEncoding(conts);
}

BOOST_AUTO_TEST_CASE( TestSetSubtitles )
{
    return;
    GtkmmDBInit gd_init;
    RefPtr<MediaStore> md = LoadTestPrj().mdStore;
    ASSERT( Size(md) );

    VideoItem vi = IsVideo(md->GetMedia(md->children().begin()));
    ASSERT( vi );

    DialogParams SubtitlesDialog(VideoItem vi, Gtk::Widget* par_wdg);
    DoDialog(SubtitlesDialog(vi, 0));
}

bool GetEncoding(const std::string& fpath, std::string& enc_str, 
                 const std::string& opts);

bool GetRussianEncoding(const std::string& fpath, std::string& enc_str)
{
    return GetEncoding(fpath, enc_str, "-L russian ");
}

static std::string TestGetEncoding(const char* fname)
{
    std::string enc_str;
    BOOST_CHECK( GetRussianEncoding(GetTestFileName(fname), enc_str) );
    return enc_str;
}

BOOST_AUTO_TEST_CASE( TestEnca )
{
    BOOST_CHECK_EQUAL( TestGetEncoding("enca/cp1251.srt"), "CP1251" );
    BOOST_CHECK_EQUAL( TestGetEncoding("enca/koi8r.srt"),  "KOI8-R" );
    BOOST_CHECK_EQUAL( TestGetEncoding("enca/utf8.srt"),   "UTF-8" );
    // :KLUDGE: вот тут enca не прав - это UTF-16LE, так как UCS-2
    // бывает только BE (см. BOM)
    BOOST_CHECK_EQUAL( TestGetEncoding("enca/utf16.srt"),  "UCS-2" );
}

} // namespace Project

BOOST_AUTO_TEST_CASE( TestStressFFViewer )
{
    return;
    //const char* fname = "../AV-Samples/M.Jackson_1080p-h264.mkv";
    const char* fname = "../AV-Samples/Chuzhaja_720w_mpeg4.avi";

    FFViewer ffv;
    bool res = ffv.Open(fname);
    BOOST_CHECK( res );

    double dur = Duration(ffv);
    BOOST_CHECK_GT( dur, 3600 ); // 1 час

    bool SetTime(FFViewer& ffv, double time);
    for( int i=0; i<5; i++ )
    {
        double tm = dur/2 + 30*(i%2 ? i : -i);
        res = SetTime(ffv, tm);
        BOOST_CHECK( res );
    }

    // повторная открываемость
    for( int i=0; i<5; i++ )
        RGBOpen(ffv, fname);
}

