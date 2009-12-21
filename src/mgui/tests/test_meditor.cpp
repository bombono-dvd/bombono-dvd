//
// mgui/tests/test_meditor.cpp
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

#include <mgui/tests/_pc_.h>

#include "mgui_test.h"

#include <mgui/project/menu-render.h>
#include <mgui/editor/toolbar.h>
#include <mgui/sdk/packing.h>

#include <mbase/project/colormd.h>

#include <mlib/lambda.h>
#include <mlib/sdk/logger.h>

namespace {

static void FillMenu(Project::Menu mn)
{
    using namespace Project;
    // --frame resources/frames/ogradient --size 340x225 --offset 30+30 --content ../Autumn.mpg 
    // --offset 100+400 --text "Preved, Medved" --offset 330+230 --content ../Autumn.mpg 
    // --offset 100+430 --text "Bye, Medved" 
    // tools/test-data/flower.jpg
    StorageItem si = new StillImageMD;
    si->MakeByPath(GetTestFileName("flower.jpg"));
    mn->BgRef() = si;

    Point sz(340, 225);
    // *
    FrameItemMD* f_md = new FrameItemMD(mn.get());
    f_md->Theme()     = "ogradient";
    f_md->Placement() = RectASz(Point(30, 30), sz);
    si = new VideoMD;
    si->MakeByPath("../Autumn.mpg");
    f_md->Ref() = si;

    // *
    TextItemMD* t_md = new TextItemMD(mn.get());
    t_md->Placement() = RectASz(Point(100, 400), Point(140, 50));
    t_md->mdName = "Preved, Medved";

    // *
    f_md = new FrameItemMD(mn.get());
    f_md->Theme() = "ogradient";
    f_md->Placement() = RectASz(Point(330, 230), sz);
    f_md->Ref() = new ColorMD(BLUE_CLR); //si;

    // *
    t_md = new TextItemMD(mn.get());
    t_md->Placement() = RectASz(Point(100, 430), Point(100, 50));
    t_md->mdName = "Bye, Medved";
}

GSList* GetGtkGroup(Gtk::RadioToolButton& btn)
{
    return gtk_radio_tool_button_get_group(btn.gobj());
}

} // namespace 

BOOST_AUTO_TEST_CASE( TestMEditor )
{
    return;
    InitGtkmm();

    Project::Menu mn = new Project::MenuMD;
    FillMenu(mn);
    Project::OpenMenu(mn);

    //Application app;
    Gtk::Window win;
    win.set_default_size(800, 600);
    win.set_title("MEditor");

    Gtk::VBox& vbox = NewManaged<Gtk::VBox>();
    win.add(vbox);
    // *
    MEditorArea editor;
    Editor::PackToolbar(editor, vbox);
    vbox.pack_start(editor);

    win.show_all();

    // загружаем после show_all(), чтобы Rebuild()
    // нормально прошел (win_sz не нуль => VideoArea::FramePixbuf() не нуль => текст будет инициализирован)
    editor.LoadMenu(mn);

    Gtk::Main::run(win);
}

///////////////////////////////////////////////////

// см. Gtk::set_group()
BOOST_AUTO_TEST_CASE( TestRadioToolButton )
{
    InitGtkmm();

    // неработающий вариант
    {
        Gtk::RadioButtonGroup group;

        Gtk::RadioToolButton btn1;
        btn1.set_group(group);

        Gtk::RadioToolButton btn2;
        btn2.set_group(group);

        // O, заработал "коробочный вариант" - в Gtkmm исправили ошибку => избавляемся 
        // от Gtk::set_group()
        BOOST_CHECK( GetGtkGroup(btn1) != GetGtkGroup(btn2) );
    }

    // работающий вариант
    {
        Gtk::RadioButtonGroup group;

        Gtk::RadioToolButton btn1;
        Gtk::set_group(btn1, group);

        Gtk::RadioToolButton btn2;
        Gtk::set_group(btn2, group);

        BOOST_CHECK( GetGtkGroup(btn1) == GetGtkGroup(btn2) );
    }
}

