//
// mbase/project/const.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#ifndef __MBASE_PROJECT_CONST_H__
#define __MBASE_PROJECT_CONST_H__

// что не может быть в предкомпилированном заголовке, то включать
// вне его приходится
#include "_non_pc_.h"

#include <mlib/mlib.h>
#include <mlib/geom2d.h>
#include <mlib/sdk/misc.h>

#include <limits> // std::numeric_limits

//
// set_hi_precision - манипулятор для установки макс. точности 
// (обычно нужно для чисел с плавающей точкой)
// Пример:
//     io::cout << set_hi_precision<>() << dbl_val;
// 
// :TODO: перенести в mlib и заменить везде с strm.precision(N) на set_hi_precision<>()

template<typename Type = double>
struct set_hi_precision { };

template<typename CharT, typename Traits, typename T> inline std::basic_ostream<CharT, Traits>& 
operator << (std::basic_ostream<CharT, Traits>& os, set_hi_precision<T> /*f*/)
{ 
    os.precision(std::numeric_limits<T>::digits10 + 2);
    return os; 
}

// текущая версия BmD
extern const char* APROJECT_VERSION;
#define APROGRAM_PRINTABLE_NAME "\"Bombono DVD\""

#endif // __MBASE_PROJECT_CONST_H__

