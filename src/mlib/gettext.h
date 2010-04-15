//
// mlib/gettext.h
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

inline std::string _dots_(const char* str)
{
    return gettext(str) + std::string("...");
}

inline std::string _semicolon_(const char* str)
{
    return gettext(str) + std::string(":");
}

// вариант _() с добавлением точек в конце для пунктов меню
#define DOTS_(str) _dots_(str).c_str()
#define SMCLN_(str) _semicolon_(str).c_str()


#endif // #ifndef __MLIB_GETTEXT_H__


