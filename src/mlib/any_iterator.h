//
// mlib/any_iterator.h
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

#ifndef __MLIB_ANY_ITERATOR_H__
#define __MLIB_ANY_ITERATOR_H__

// any_iterator: Thomas Becker vs. Adobe implementation
// see results of compilation complexity, profile_any_iterator.cpp
#define ADOBE_AIT 

#ifdef ADOBE_AIT
#   include <mlib/adobe/any_iterator.hpp>
#else
#   include <mlib/any_iterator/any_iterator.hpp>
#endif

#endif // #ifndef __MLIB_ANY_ITERATOR_H__

