#ifndef __MLIB_GETTEXT_H__
#define __MLIB_GETTEXT_H__

#include <glib/gi18n.h>

#include <boost/format.hpp>

// макрос наподобие N_(), только для xgettext
#define F_(String)

// укороченный вариант для i18n + Boost.Format
inline boost::format BF_(const char* str)
{
    return boost::format(gettext(str));
}
inline boost::format BF_(const std::string& str)
{
    return boost::format(gettext(str.c_str()));
}

//
// Приведение boost::format к std::string наиболее удобным способом:
//    MessageBox(BF_("some ... template with %N%") % arg1 % arg2 % ... % bf::stop, Gtk::BUTTONS_OK);
// 
// boost::str() требует лишних скобок, за которыми неудобно следить
namespace bf { enum stop_enum { stop }; }

inline std::string operator % (boost::format& f, bf::stop_enum /*stop*/)
{
    return f.str();
}


#endif // #ifndef __MLIB_GETTEXT_H__


