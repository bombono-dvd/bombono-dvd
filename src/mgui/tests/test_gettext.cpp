
#include <mgui/tests/_pc_.h>

//#include <libintl.h>
//#define _(String) gettext(String)
//#define gettext_noop(String) String
//#define N_(String) gettext_noop(String)

#include <mgui/project/mconstructor.h>
#include <mgui/win_utils.h>
#include <mgui/dialog.h>

#include <mlib/stream.h>
#include <mlib/tech.h>
//#include <mlib/filesystem.h>
#include <mgui/gettext.h>
#include <boost/format.hpp>

BOOST_AUTO_TEST_CASE( TestFormat )
{
    // Boost.Format
    std::string f_str = boost::str(boost::format("writing %2%,  x=%1% : %3%-th try") % "toto" % 40.23 % 50);
    BOOST_CHECK( strcmp(f_str.c_str(), "writing 40.23,  x=toto : 50-th try") == 0 );
}

BOOST_AUTO_TEST_CASE( TestGettext )
{
    return;
    InitI18n();
    
    const char* not_trans_text = N_("not_trans_text");
    BOOST_CHECK( strcmp(not_trans_text, "not_trans_text") == 0 );

    io::cout << _("Hello, world!") << io::endl;
    //io::cout << boost::format(_("writing %1%,  x=%2% : %3%-th try")) % "toto" % 40.23 % 50; 
}

BOOST_AUTO_TEST_CASE( TestMBLinks )
{
    return;
    InitGtkmm();

    std::string err_str = "See more about preparing video for authoring in <a href=\"http://www.bombono.org/\">online help</a>.";
    MessageBoxWeb("Message", Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, err_str);
}

