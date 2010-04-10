
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

