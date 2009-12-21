//
// mlib/tech.cpp
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

#include "tech.h"

#include <stdlib.h>
#include <string>
#include <sstream>

#include "stream.h"

static std::string MakeErrorStr(const char* assertion, const char* file, 
                                long line, const char* function)
{
    std::stringstream strm;
    strm << file << ":" << line << ": " << function << ": Assertion \""
         << assertion << "\" failed.";
    return strm.str();
}

void ErrorMsg(const char* str)
{
    io::cerr << str << io::endl;
}

void AssertImpl(const char* assertion, const char* file, 
                long line, const char* function)
{
    std::string str = MakeErrorStr(assertion, file, line, function);

    ErrorMsg(str.c_str());
    abort();
}

void Error(const char* str)
{
    ErrorMsg(str);
    throw str;
}

