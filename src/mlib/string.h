//
// mlib/string.h
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

#ifndef __MLIB_STRING_H__
#define __MLIB_STRING_H__

#include "ptr.h"
#include "geom2d.h"

#include <sstream>
#include <vector>

// str::stream
namespace str
{

using namespace std;

const ios_base::openmode def_mode = ios_base::out | ios_base::in | ios_base::ate ;

class stream: public std::stringstream
{
    typedef std::stringstream my_parent;
    public:

       explicit  stream(ios_base::openmode mode = def_mode)
                             : my_parent(mode) {}
       explicit  stream(const string& str, ios_base::openmode mode = def_mode)
                             : my_parent(str, mode) {}


                 // :TODO: если потребуется передавать по ссылке T&, то
                 // надо воспользоваться "mock objects" при реализации,-
                 // у Александреску есть и в Boost где-то.
                 template<typename T>
         stream& operator << (T t)
                 {
                     my_parent& strm = *this;
                     strm << t;
                     return *this;
                 }
};

} // namespace str

namespace Str
{

// считать целое из строки
bool GetLong(long& res, const char* str);
// 
bool GetDouble(double& res, const char* str);

typedef std::vector<std::string> List;
typedef ptr::shared<List> PList;

} // namespace Str

std::string PointToStr(const Point& pnt);
std::string Double2Str(double val);
std::string Int2Str(int val);

bool ExtMatch(const char* display_name, const char* ext);

std::string QuotedName(const std::string& str);

#endif // __MLIB_STRING_H__

