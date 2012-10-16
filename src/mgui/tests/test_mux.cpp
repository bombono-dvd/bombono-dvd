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
    InitGtkmm();

    const char* path = "/var/tmp";
    Gtk::FileChooserButton btn("Select folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

    BOOST_CHECK( SetFilename(btn, path) );
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

BOOST_AUTO_TEST_CASE( TestStrictDialogWin32 )
//void main()
{
    return;
    InitGtkmm();

    //gtk_init(NULL, NULL);
    GtkWidget* dlg = gtk_dialog_new();

    /* GDK_HINT_MIN_SIZE conflicts with gtk_window_set_resizable(false) */
    gtk_window_set_resizable((GtkWindow*)dlg, false);
    GdkGeometry geom;
    geom.min_width  = 200;
    geom.min_height = 200;
    gtk_window_set_geometry_hints(GTK_WINDOW(dlg), 0, &geom, GDK_HINT_MIN_SIZE); 
    
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), gtk_label_new("Label"));
    gtk_widget_show_all(dlg);
    g_signal_connect(dlg, "destroy", gtk_main_quit, NULL);
    gtk_main();
}

struct TIData
{
    Gtk::Window  win;
     Gtk::Image  img_wdg;

    TIData()
    {
        Add(win, img_wdg);
    }
};

// проверка темы иконок
BOOST_AUTO_TEST_CASE( TestThemeIcon )
{
    return;
    InitGtkmm();

    TIData dat;
    Gtk::Image& img_wdg = dat.img_wdg;
    RefPtr<Gdk::Pixbuf> pix = img_wdg.render_icon(Gtk::Stock::ADD, Gtk::ICON_SIZE_LARGE_TOOLBAR);
    img_wdg.set(pix);

    RunWindow(dat.win);
}

BOOST_AUTO_TEST_CASE( PrintStockThemeIconNames )
{
    return;
    InitGtkmm();

    TIData dat;
    boost_foreach( const Gtk::StockID& stock_id, Gtk::Stock::get_ids() )
    {
        Gtk::IconSet is;
        if( !Gtk::Stock::lookup(stock_id, is) )
            io::cout << "No stock id: " + stock_id.get_string() << io::endl;

        // :KLUDGE: не знаю как вывести имена файлов не через theme_lookup_icon()
        dat.img_wdg.render_icon(stock_id, Gtk::ICON_SIZE_BUTTON);
    }
}

static void OnBtnClicked()
{
    // эмулируем Glib-исключение
    RefPtr<Gdk::Pixbuf> img = Gdk::Pixbuf::create_from_file("/");
    //throw std::exception("boom");
}

BOOST_AUTO_TEST_CASE( TestHandlingExceptionsWithGtkmm )
{
    return;
    InitGtkmm();

    Gtk::Button btn;
    btn.signal_clicked().connect(&OnBtnClicked);
    btn.clicked();
}
