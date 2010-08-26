#ifndef __MLIB_FORMAT_H__
#define __MLIB_FORMAT_H__

#include <boost/format.hpp>
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

#endif // #ifndef __MLIB_FORMAT_H__

