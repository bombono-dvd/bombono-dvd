//
// test_tech.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007 Ilya Murav'jov
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

#include <mlib/tests/_pc_.h>

#include <mlib/mlib.h>

#include PACK_ON
struct packed_struct_example
{
    int8_t  c;
    int32_t i;
};
#include PACK_OFF

#ifdef HAS_PACK_STACK
#pragma pack(8)
struct unpacked_struct_example
{
    int8_t  c;
    int32_t i;
};
#pragma pack()
#endif

BOOST_AUTO_TEST_CASE( PackTest )
{
    BOOST_CHECK_EQUAL( 5U, sizeof(packed_struct_example) );
#ifdef HAS_PACK_STACK
    BOOST_CHECK_EQUAL( 8U, sizeof(unpacked_struct_example) );
#endif
}

