//
// mbase/project/_extlibs_.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008, 2010 Ilya Murav'jov
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

#ifndef __MBASE_PROJECT__EXTLIBS__H__
#define __MBASE_PROJECT__EXTLIBS__H__

//
// _extlibs_.h - внешние библиотеки mbase/project
// 

#include "tech.h"

//
// mlib
//
//#include <boost/smart_ptr.hpp>
//#include <boost/intrusive_ptr.hpp>
#include <mlib/ptr.h>
#include <mlib/format.h>
#include <mlib/sdk/logger.h>
#include <mlib/stream.h>
//#ifndef EXT_BOOST
//#include <mlib/lambda.h>
//#endif // EXT_BOOST
//#include <mlib/lambda.h>
#include <mlib/bind.h>

//
// Foreach & Range 
//
#include <mlib/foreach.h>
#include <mlib/range/any_range.h>
#include <mlib/range/enumerate.h>
#include <mlib/range/filter.h>
#include <mlib/range/irange.h>
#include <mlib/range/slice.h>
#include <mlib/range/transform.h>

//
// Boost
//
#include <boost/function.hpp>
#include <mlib/sdk/bfs.h>

//#include <boost/lexical_cast.hpp>

//
// STL
//
#include <vector>
#include <list>
//#include <deque>
//#include <set>
//#include <map>
//
//#include <sstream>
#include <string>

//#include <limits>
#include <cmath>
#include <algorithm>
#include <utility>
#include <stdexcept>

//
// Misc
//
//#include <glibmm.h>
#include <glibmm/convert.h>

#include <libxml++/libxml++.h>


#endif // __MBASE_PROJECT__EXTLIBS__H__

