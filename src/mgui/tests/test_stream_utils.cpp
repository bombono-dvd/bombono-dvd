//
// mgui/tests/test_stream_utils.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008 Ilya Murav'jov
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

#include <mgui/tests/_pc_.h>

#include <mgui/stream_utils.h>
#include <mlib/stream.h>

BOOST_AUTO_TEST_CASE( test_stream_utils )
{
    {
        std::stringstream strm;
        bin::stream b_strm(strm);
        int i = 12345, j;

        b_strm << i;
        BOOST_CHECK( strm.str().size() == sizeof(int) );

        unsigned int u_i = 0xffffffff, u_j;
        b_strm << u_i;

        b_strm.seekg(0, std::ios_base::beg);
        b_strm >> j >> u_j;
        BOOST_CHECK_EQUAL( i, j );
        BOOST_CHECK_EQUAL( u_i, u_j );

    }

    {
        io::stream strm;
        bin::stream b_strm(strm);
    }

    {
        std::stringstream strm;
        bin::stream b_strm(strm);
        std::string str1 = "12345", str2;

        b_strm << str1;
        b_strm.seekg(0, std::ios_base::beg);
        size_t len;
        b_strm >> len;
        BOOST_CHECK( len == str1.size() );
        b_strm.seekg(0, std::ios_base::beg);
        b_strm >> str2;
        BOOST_CHECK( str1 == str2 );
    }
}
