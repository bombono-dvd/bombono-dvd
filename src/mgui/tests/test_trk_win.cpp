//
// mgui/tests/test_trk_win.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#include "mgui_test.h"

#include <mgui/timeline/service.h> 
#include <mgui/timeline/mviewer.h>
#include <mgui/sdk/cairo_utils.h>
#include <mgui/sdk/window.h>

static void CheckFramesToTime(long b_pos, std::string& str, double fps)
{
    using namespace Timeline;
    FramesToTime(str, b_pos, fps);

    long e_pos;
    BOOST_CHECK( ParsePointerPos(e_pos, str.c_str(), fps) );
    BOOST_CHECK_EQUAL( b_pos, e_pos );
}

BOOST_AUTO_TEST_CASE( TestFramesToTimeConvert )
{
    int b_pos;
    double fps;
    std::string str;
    using namespace Timeline;

    //
    // "NTSC VIDEO"
    //
    fps = Mpeg::NTSC_VIDEO_FRAME_FPS;
    b_pos = 0;
    CheckFramesToTime(b_pos, str, fps);

    // для "NTSC VIDEO" будет пропущен кадр 33;29
    b_pos = 1018;
    CheckFramesToTime(b_pos, str, fps);
    BOOST_CHECK_EQUAL( str, "00:00:33;28" );

    b_pos = 1019;
    CheckFramesToTime(b_pos, str, fps);
    BOOST_CHECK_EQUAL( str, "00:00:34;00" );

    //
    // "PAL/SECAM FILM"
    //
    fps = Mpeg::PAL_SECAM_FRAME_FPS;
    b_pos = 1018;
    CheckFramesToTime(b_pos, str, fps);
    BOOST_CHECK_EQUAL( str, "00:00:40;18" );

    b_pos = 1019;
    CheckFramesToTime(b_pos, str, fps);
    BOOST_CHECK_EQUAL( str, "00:00:40;19" );
}

////////////////////////////////////////////////////////
namespace Timeline
{

class TestMonitor: public Timeline::Monitor 
{
    public:
                      TestMonitor();
                     //~TestMonitor() { SetCurrentVideo(oldVideo); }

    virtual     void  GetFrame(RefPtr<Gdk::Pixbuf> pix, int frame_pos);
    virtual     void  OnSetPos() {}

    protected:

        Project::VideoItem oldVideo;
};

TestMonitor::TestMonitor()
{
    //oldVideo = SetCurrentVideo(new Project::VideoMD);

    //ASSERT( plyr.Open("/opt/src/Video/MonaLisa/ml_02_6.vob") );
    //ASSERT( plyr.Open("/opt/src/Video/Billiard/vts_01_1.vob") );
    std::string fname = "/opt/src/Video/elephantsdream.mpg";
    bool is_open = plyr.Open(fname.c_str());
    //bool is_open = plyr.Open("../Autumn.mpg");
    ASSERT( is_open );

    CurrVideo->MakeByPath(fname);
    double fps = Mpeg::GetFrameFPS(plyr);

    curPos = TimeToFrames(0, 5, 1, 16, fps); // 5 мин. 1 с 16-й кадр
    PushBackDVDMark(TimeToFrames(0, 0, 59, 21, fps));
    PushBackDVDMark(TimeToFrames(0, 3, 10,  2, fps));
}

void TestMonitor::GetFrame(RefPtr<Gdk::Pixbuf> pix, int)
{
    static RefPtr<Gdk::Pixbuf> fl_pix;
    if( !fl_pix )
        fl_pix = Gdk::Pixbuf::create_from_file(GetTestFileName("flower.jpg"));

    RGBA::Scale(pix, fl_pix);
}

} // namespace Timeline

void DoTestTrackLayout()
{
    return;
    InitGtkmm();
    Gtk::Window win;

    Timeline::TestMonitor mon;

    TrackLayout layout(mon);
    PackTrackLayout(win, layout);

    win.set_default_size(800, 200);
    RunWindow(win);
}

BOOST_AUTO_TEST_CASE( TestTrackLayout )
{
    DoTestTrackLayout();
}

///////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( TestTLMonitor )
{
    return;
    InitGtkmm();

    RunMViewer();
}

///////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( TestFCW )
{
    return;
    InitGtkmm();
    Gtk::Window win;

    PackFileChooserWidget(win, OpenFileFnr(), false);

    win.set_default_size(800, 600);
    RunWindow(win);
}

///////////////////////////////////////////////////////

// 
// Ошибка: при (интерактивном) изменении размеров da
// вместо картинки отрисовывается "цветные полосы".
// Причем проявляется только некоторое время сразу после
// выхода из спящего режима и только если меняется точка приложения
// cairo_set_source(), !=(0,0) (!!)
// 

// bool DoTestFill(Gtk::DrawingArea& da, GdkEventExpose* )//event)
// {
//     static Cairo::RefPtr<Cairo::ImageSurface> sur;
//     if( !sur )
//         sur = Cairo::ImageSurface::create_from_png(GetTestFileName("flower.png"));
//
//     Point sz(WidgetSize(da));
//     Cairo::RefPtr<Cairo::Context> cr_pp = da.get_window()->create_cairo_context();
//
//     double r = std::min(sz.x, sz.y)/2.;
//     cr_pp->set_source(sur, sz.x/2.0 - r, sz.y/2.0 - r);
//     cr_pp->arc(sz.x/2., sz.y/2., r, 0, 2.0*M_PI);
//     //cr->close_path();
//
//     cr_pp->fill();
//     //cr->paint();
//
//     return true;
// }
//
// BOOST_AUTO_TEST_CASE( CairoImageFillError )
// {
//     TestExampleDA(DoTestFill);
// }

///////////////////////////////////////////////////////

namespace Timeline 
{
void PaintPointer(CR::RefPtr<CR::Context> cr, DPoint pos, Point sz, bool form_only = false);
} //namespace Timeline

bool DoCairoScale(Gtk::DrawingArea& da, GdkEventExpose* )//event)
{
    Point sz(WidgetSize(da));

    //Cairo::RefPtr<Cairo::Context> cr = da.get_window()->create_cairo_context();
    Cairo::RefPtr<Cairo::Context> cr = CreateSVGCairoContext("../CairoCurveTest", sz);

    io::cout << "sz " << sz.x << " " << sz.y << io::endl;

    double lct_x = sz.x/2.0;
    double lct_y = sz.y;

    Timeline::PaintPointer(cr, DPoint(lct_x, lct_y), sz);


    return true;
}

BOOST_AUTO_TEST_CASE( CairoCurveTest )
{
    //TestExampleDA(DoCairoScale);
}

