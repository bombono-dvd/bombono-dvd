//
// mgui/tests/test_mux.cpp
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

#include <mgui/mux.h>
#include <mgui/prefs.h>

#include <mgui/win_utils.h>

#include <mgui/sdk/widget.h>
#include <mgui/sdk/window.h>
#include <mgui/sdk/packing.h>

BOOST_AUTO_TEST_CASE( TestFileChooser )
{
    return;
    InitGtkmm();

    const char* path = "/var/tmp";
    Gtk::FileChooserButton btn("Select folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

    BOOST_CHECK( btn.set_filename(path) );
    //BOOST_CHECK( btn.set_current_folder(path) );

    //Gtk::Window win;
    //win.add(btn);
    //RunWindow(win);
    IteratePendingEvents();
    
    //io::cout << btn.get_filename() << io::endl;
    BOOST_CHECK_MESSAGE( strcmp(btn.get_filename().c_str(), path) == 0, "See more at https://bugzilla.gnome.org/show_bug.cgi?id=615353" );
}

BOOST_AUTO_TEST_CASE( TestPreferences )
{
    return;
    InitGtkmm();

    LoadPrefs();
    ShowPrefs();
}

BOOST_AUTO_TEST_CASE( TestMux )
{
    return;
    InitGtkmm();

    std::string fname;
    MuxStreams(fname);
}

