//
// mlib/instantiate.cpp
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

#include "format.h"

#ifdef EXTERNAL_INSTANTIATE

#include <boost/format.hpp> // определение класса 

template class boost::basic_format<char>;

INST_BFRMT_PERCENT(const std::string)
INST_BFRMT_PERCENT(std::string)
INST_BFRMT_PERCENT(char const*)
INST_BFRMT_PERCENT(char* const) // = const char*
INST_BFRMT_PERCENT(char const* const)
INST_BFRMT_PERCENT(unsigned long long) // для uint64_t
INST_BFRMT_PERCENT(const double)
INST_BFRMT_PERCENT(double)
INST_BFRMT_PERCENT(unsigned int)
INST_BFRMT_PERCENT(unsigned char)
INST_BFRMT_PERCENT(char const)
INST_BFRMT_PERCENT(long)
INST_BFRMT_PERCENT(unsigned long)
INST_BFRMT_PERCENT(const long)

// :KLUDGE: в зависимости от режима оптимизации компиляции инстанцировать приходится разные объекты,
// в зависимости от того, inline'ит компилятор operator%() и feed() или нет
// Поэтому оставим режим инстанцирования только для разработки (такая же причина, что и "быстрая" сборка)

//#define INST_BFRMT_FEED(Type) template boost::format& boost::io::detail::feed<char, std::char_traits<char>, std::allocator<char>, Type&>(boost::format& self, Type& x);
//
//INST_BFRMT_FEED(char const* const)
//INST_BFRMT_FEED(double const)
//INST_BFRMT_FEED(int const)
//INST_BFRMT_FEED(int)
//INST_BFRMT_FEED(std::string)

#endif // EXTERNAL_INSTANTIATE


