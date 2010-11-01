//
// mlib/tests/turn_on_test_main.h
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

#ifndef __MLIB_TESTS_TEST_TOTM_H__
#define __MLIB_TESTS_TEST_TOTM_H__

#include <boost/version.hpp>

//
// for Boost > 1.33 dynamic version begin to be built
// 
#if BOOST_VERSION / 100 % 1000 > 33
#   ifndef STILL_HAVE_STATIC_BOOST_WITH_MAIN
#       define BOOST_TEST_DYN_LINK 
#   endif
#endif

#define BOOST_AUTO_TEST_MAIN


#endif // #ifndef __MLIB_TESTS_TEST_TOTM_H__

