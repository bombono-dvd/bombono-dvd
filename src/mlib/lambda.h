//
// mlib/lambda.h
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

#ifndef __MLIB_LAMBDA_H__
#define __MLIB_LAMBDA_H__

// Замечание: оригинальную Boost.Lambda нельзя использовать
// в gcc' PCH, см. ошибку gcc #10591
#include <boost/lambda/lambda.hpp> // для _1,..., и операторов =,+,-,...
#include <boost/lambda/bind.hpp>   // bind
//#include <boost/lambda/if.hpp>     // if_then

namespace bl = boost::lambda;

#endif // __MLIB_LAMBDA_H__

