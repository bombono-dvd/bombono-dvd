
#include <mgui/tests/_pc_.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop(String)

#include <mbase/resources.h>

#include <mlib/stream.h>
#include <mlib/filesystem.h>

BOOST_AUTO_TEST_CASE( TestConstructor )
{
    setlocale(LC_ALL, "");
    const char* prefix = GetInstallPrefix();
    std::string locale_prefix(prefix ? (fs::path(prefix) / "share" / "locale").string() : "build/po/locale");
    bindtextdomain("bombono-dvd", locale_prefix.c_str());
    textdomain("bombono-dvd");
    
    const char* not_trans_text = N_("not_trans_text");
    BOOST_CHECK( strcmp(not_trans_text, "not_trans_text") == 0 );

    io::cout << _("Hello, world!") << io::endl;
}

