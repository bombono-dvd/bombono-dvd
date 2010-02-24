
#include <mgui/tests/_pc_.h>

//#include <libintl.h>
//#define _(String) gettext(String)
//#define gettext_noop(String) String
//#define N_(String) gettext_noop(String)

#include <mgui/project/mconstructor.h>

#include <mlib/stream.h>
//#include <mlib/filesystem.h>
#include <mgui/gettext.h>

BOOST_AUTO_TEST_CASE( TestConstructor )
{
    return;
    InitI18n();
    
    const char* not_trans_text = N_("not_trans_text");
    BOOST_CHECK( strcmp(not_trans_text, "not_trans_text") == 0 );

    io::cout << _("Hello, world!") << io::endl;

    //io::cout << boost::format(_("writing %1%,  x=%2% : %3%-th try")) % "toto" % 40.23 % 50; 
}

