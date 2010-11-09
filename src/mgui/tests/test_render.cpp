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

#include <mgui/sdk/player_utils.h>
#include <mgui/execution.h>
#include <mgui/init.h>
#include <mgui/author/ffmpeg.h>

#include <mlib/format.h>

namespace Project {

BOOST_AUTO_TEST_CASE( TestRenderTranscoding )
{
    return;
    std::string fname   = "/home/ilya/opt/programming/atom-project/Autumn.mpg";
    std::string a_fname = "/home/ilya/opt/programming/atom-project/transition.mpg";

    Mpeg::FwdPlayer plyr;
    RGBOpen(plyr, fname);

    bool is_pal = plyr.MInfo().vidSeq.hgt == 576;

    int fps = 30;
    double shift = 1. / fps;
    double end  = 1.; // секунда

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

BOOST_AUTO_TEST_CASE( TestStillTranscoding )
{
    return;
    std::string fname = "/home/ilya/opt/programming/atom-project/dvd_out/3.Menu 4/Menu.png";

    // в случае автономной работы ffmpeg указываем длительность аргументом
    double duration = 15;
    std::string ffmpeg_cmd = boost::format("ffmpeg -t %3$.2f -loop_input -i \"%1%\" %2%") 
        % fname % FFmpegPostArgs("../dvd_out/trans.vob", false, true) % duration % bf::stop;
 
    io::cout << ffmpeg_cmd << io::endl;
    //Execution::SimpleSpawn(ffmpeg_cmd.c_str());
    ExitData ed = System(ffmpeg_cmd);
    BOOST_CHECK( ed.IsGood() );
}

BOOST_AUTO_TEST_CASE( TestMenuSettings )
{
    return;
    GtkmmDBInit gd_init;
    RefPtr<MenuStore> ms = InitAndLoadPrj(TestProjectPath()).mnStore;
    ASSERT( Size(ms) );

    Menu mn = GetMenu(ms, ms->children().begin());
    void MenuSettings(Menu mn, Gtk::Window* win);
    MenuSettings(mn, 0);
}

BOOST_AUTO_TEST_CASE( TestCheckFFmpeg )
{
    std::string conts = GetTestFNameContents("ffmpeg05_valid_dvd_encoders.txt");
    //std::string conts = Glib::file_get_contents("/home/ilya/opt/programming/atom-project/hardy_formats_.txt");
    //std::string conts = Glib::file_get_contents("/home/ilya/opt/programming/atom-project/lucid_formats.txt");

    void TestFFmpegForDVDEncoding(const std::string& conts);
    TestFFmpegForDVDEncoding(conts);
}

} // namespace Project


