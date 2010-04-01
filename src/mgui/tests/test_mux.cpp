
#include <mgui/tests/_pc_.h>

#include <mgui/mux.h>
#include <mgui/prefs.h>

#include <mgui/win_utils.h>


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

