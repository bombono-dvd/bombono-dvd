#ifndef __MLIB_FORMAT_H__
#define __MLIB_FORMAT_H__

#ifdef EXT_BOOST

#include <boost/format.hpp>

#else // EXT_BOOST

// format.hpp включает все реализацию, что добавляет
// в каждый объектник ~400kb кода!

// Определение boost::format находится в format_class.hpp,
// который не является независимым (требует предварительного включения,
// см. boost/format.hpp
#include <boost/config.hpp>

#ifndef BOOST_NO_STD_LOCALE
#include <locale>
#endif

#include <boost/format/format_class.hpp>

#define INST_BFRMT_PERCENT(Type) template boost::format& boost::format::operator%<Type>(Type& x);

#endif // EXT_BOOST

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

