//
// mlib/format.h
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

#ifndef __MLIB_FORMAT_H__
#define __MLIB_FORMAT_H__

#if !(defined(EXT_BOOST) || defined(NDEBUG))
#define EXTERNAL_INSTANTIATE
#endif

#ifdef EXTERNAL_INSTANTIATE

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

#else // EXTERNAL_INSTANTIATE

#include <boost/format.hpp>

#endif // EXTERNAL_INSTANTIATE

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

