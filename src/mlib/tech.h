//
// mlib/tech.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2009 Ilya Murav'jov
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

#ifndef __MLIB_TECH_H__
#define __MLIB_TECH_H__

//
//  Технические вещи
// 
#include <boost/current_function.hpp>  // для BOOST_CURRENT_FUNCTION
#include <boost/detail/endian.hpp>     // для BOOST_XXX_ENDIAN

// для С-шного кода в С++
#if defined(__cplusplus) || defined(c_plusplus)
#define C_LINKAGE extern "C"
#define C_LINKAGE_BEGIN extern "C" {
#define C_LINKAGE_END }
#else
#define C_LINKAGE
#define C_LINKAGE_BEGIN
#define C_LINKAGE_END
#endif

// посчитать размер C-массива (T arr[] = {...}; )
#define ARR_SIZE(arr) ( sizeof(arr)/sizeof(arr[0]) ) 

// используем конструкцию наподобие
//     #include PACKON
//     struct packed_data { ... };
//     #include PACKOFF
// , чтобы у аттрибутов packed_data не было выравнивания;
// другой вариант - __attribute__ ((packed)), но он есть только у gcc
#undef HAS_PACK_STACK
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__COMO__) || defined(_MSC_VER)
#   define HAS_PACK_STACK
#endif

#define PACK_ON  <mlib/pack_on.h>
#define PACK_OFF <mlib/pack_off.h>

// ASSERT(), ASSERT_RTL()
#define ASSERT_IMPL(expr)  ((expr) ? ((void)0) : AssertImpl(#expr, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION))


#ifdef NDEBUG
#define ASSERT(expr)       ((void)0)
#else
#define ASSERT(expr)       ASSERT_IMPL(expr)
#endif // NDEBUG

#define ASSERT_RTL(expr)   ASSERT_IMPL(expr)

// ErrorMsg(), Error()
void ErrorMsg(const char* str);
void Error(const char* str);


void AssertImpl(const char* assertion, const char* file, 
                long line, const char* function);

// endianness
#if defined(BOOST_BIG_ENDIAN)
#   define HAS_BIG_ENDIAN
#elif defined(BOOST_LITTLE_ENDIAN)
#   define HAS_LITTLE_ENDIAN
#else
#   error mlib/tech.h: unknown endianness (legacy PDP arch?)
#endif

#endif // #ifndef __MLIB_TECH_H__
