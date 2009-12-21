//
// mgui/tests/test_menu.cpp
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

#include "test_mbrowser.h"
#include "test_author.h"

#include <mgui/project/menu-actions.h>
#include <mgui/project/menu-render.h>
#include <mgui/author/output.h>
#include <mgui/sdk/window.h>
#include <mgui/sdk/packing.h>
#include <mgui/dialog.h>

#include <mbase/project/colormd.h>

//#include <mlib/sdk/logger.h>

namespace Project
{
bool CanOpenAsVideo(const char* fname, std::string& err_string, bool& must_be_video);

BOOST_AUTO_TEST_CASE( TestConstructor )
{
    return;
    GtkmmDBInit gd_init;

    //bool must_be_video = false;
    //std::string err_str;
    //if( !CanOpenAsVideo("../ntsc/Autumn_ntsc.mpg", err_str, must_be_video) )
    //{
    //    MessageBox("Can't add file:", Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, err_str);
    //}
    //return;

    //ConstructorApp app; 

    std::string prj_fname = TestProjectPath();
    //BuildConstructor(app, prj_fname, false);
    //PackOutputAndRun(app, prj_fname);
    RunConstructor(prj_fname, false);
}

BOOST_AUTO_TEST_CASE( TestMapRectList )
{
    RectListRgn lst;
    lst.push_back( Rect(4, 0, 8, 4) );

    RectListRgn d_lst;
    MapRectList(d_lst, lst, Rect(2, 2, 6, 6), Point(8, 4));
    BOOST_CHECK_EQUAL( Rect(4, 2, 6, 6), d_lst.front() );
}

const uint BLUE_COLOR = 0x0000ffff;
const uint RED_COLOR  = 0xff0000ff;

namespace { 

void ViewPicture(RefPtr<Gdk::Pixbuf> pix)
{
    Gtk::Window win;
    win.add(NewManaged<Gtk::Image>(pix));

    RunWindow(win);
}

MenuPack& CheckMenuPack(Menu mn, bool check_thumb_updated = true)
{
    MenuPack& mp = mn->GetData<MenuPack>();
    BOOST_CHECK( !GetRenderList(mp.thRgn).size() );
    if( check_thumb_updated )
        BOOST_CHECK( !mp.thumbNeedUpdate );

    return mp;
}

Rect MakeSmallRect(const Point& a, const Point& small_sz, int i, int j)
{
    return RectASz(a + Point(i*small_sz.x, j*small_sz.y), small_sz);
}

void StressRenderSystem(Menu mn, const RectListRgn& lst, int cnt)
{
    for( int i=0; i<cnt; i++ )
    {
        RectListRgn l(lst);
        ASSERT( l.size() );

        RenderMenuSystem(mn, l);
        CheckMenuPack(mn, false);
    }
}

void PushBackXYRect(RectListRgn& lst, int i, int j, const Point& a, const Point& small_sz, 
                    const Planed::Transition& trans)
{
    Rect small_rct = MakeSmallRect(a, small_sz, i, j);
    Rect dev_rct = AbsToRel(trans, small_rct);
    //io::cout << "small_rct " << small_rct << ", dev_rct " << dev_rct << io::endl;
    lst.push_back(dev_rct);
}

// прогон рендеринга при одном небольшом изменении
void SmallChangeStress(MenuPack& mp, const Point& a, int cnt)
{
    Point small_sz(100, 100);
    RectListRgn lst;
    PushBackXYRect(lst, 1, 1, a, small_sz, mp.thRgn.Transition());
    // *
    StressRenderSystem(mp.Owner(), lst, cnt);
}

// прогон рендеринга при большом изменении
void BigChangeStress(MenuPack& mp, const Point& a, int cnt)
{
    Point small_sz(100, 100);
    RectListRgn lst;
    for( int i=0; i<3; i++ )
        for( int j=0; j<3; j++ )
            PushBackXYRect(lst, i, j, a, small_sz, mp.thRgn.Transition());
    // *
    StressRenderSystem(mp.Owner(), lst, cnt);
}

// для тестирования производительности
void TestChangeProductivity(MenuPack& mp, const Point& a, int cnt)
{
    RefPtr<Gdk::Pixbuf> pix = mp.CnvBuf().FramePixbuf();

    pix->fill(BLACK_CLR);
    SmallChangeStress(mp, a, cnt);
    //BigChangeStress(mp, Point(100, 100), cnt);
    ViewPicture(pix);
}

} // namespace

// одно меню с одним пунктом, без рекурсии
BOOST_AUTO_TEST_CASE( TestMenuRender1 )
{
    GtkmmDBInit gd_init;

    RefPtr<MenuStore> mn_store = CreateEmptyMenuStore();
    Point a(150, 150);
    Point sz(300, 300);
    // *
    Menu mn = new MenuMD;
    mn->BgRef() = new ColorMD(RED_COLOR); // si;

    FrameItemMD* f_md = new FrameItemMD(mn.get());
    f_md->Placement() = RectASz(a, sz);
    f_md->Ref() = new ColorMD(BLUE_COLOR); // si; 
    OpenPublishMenu(mn_store->append(), mn_store, mn);

    MenuPack& mp = CheckMenuPack(mn);
    // * проверка цветности
    RefPtr<Gdk::Pixbuf> pix = mp.CnvBuf().FramePixbuf();
    //ViewPicture(pix);

    BOOST_CHECK_EQUAL( RGBA::GetPixel(pix, Point(0, 0)).ToUint(), RED_COLOR );
    Point blue_pnt = mp.thRgn.Transition().AbsToRel(a);
    BOOST_CHECK_EQUAL( RGBA::GetPixel(pix, blue_pnt).ToUint(), BLUE_COLOR );

    // * производительность
    //TestChangeProductivity(mp, a, 5000);
}

// 2 меню, рекурсивно вложенных друг в друга
BOOST_AUTO_TEST_CASE( TestMenuRender2 )
{
    GtkmmDBInit gd_init;

    RefPtr<MenuStore> mn_store = CreateEmptyMenuStore();
    Point a(150, 150);
    Rect lct = RectASz(a, Point(300, 300));

    // 1-е
    Menu mn1 = new MenuMD;
    //StorageItem si = new StillImageMD;
    //si->MakeByPath(GetTestFileName("flower.jpg"));
    mn1->BgRef() = new ColorMD(RED_COLOR); // si;

    FrameItemMD* f_md1 = new FrameItemMD(mn1.get());
    f_md1->Placement() = lct;

    // 2-е
    Menu mn2 = new MenuMD;
    //si = new VideoMD;
    //si->MakeByPath("../Autumn.mpg");
    mn2->BgRef() = new ColorMD(BLUE_COLOR); // si;

    FrameItemMD* f_md2 = new FrameItemMD(mn2.get());
    f_md2->Placement() = lct;

    f_md1->Ref() = mn2;
    f_md2->Ref() = mn1;
    // публикация
    OpenMenu(mn1);
    OpenMenu(mn2);
    PublishMenu(mn_store->append(), mn_store, mn1);
    PublishMenu(mn_store->append(), mn_store, mn2);

    MenuPack& mp1 = CheckMenuPack(mn1);
    RefPtr<Gdk::Pixbuf> pix1 = mp1.CnvBuf().FramePixbuf();
    MenuPack& mp2 = CheckMenuPack(mn2);
    RefPtr<Gdk::Pixbuf> pix2 = mp2.CnvBuf().FramePixbuf();

    // * проверка цветности
    Point f_pnt = mp1.thRgn.Transition().AbsToRel(a);
    //ViewPicture(pix1);
    BOOST_CHECK_EQUAL( RGBA::GetPixel(pix1, f_pnt).ToUint(), BLUE_COLOR );
    BOOST_CHECK_EQUAL( RGBA::GetPixel(pix2, f_pnt).ToUint(), RED_COLOR );

    // * производительность
    //TestChangeProductivity(mp1, a, 4000);
}

} // namespace Project

