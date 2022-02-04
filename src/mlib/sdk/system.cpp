//
// mlib/sdk/system.cpp
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

#include <time.h>           // clock()

#include <mlib/string.h>

#include "system.h"

int GetMemSize()
{
    pid_t pid = getpid();
    str::stream ss;
    ss << "/proc/" << pid << "/statm";
    std::string str = ss.str();

    io::stream strm(str.c_str(), iof::in);
    int mem;
    strm >> mem;
    return mem*4096;
}

double GetClockTime()
{
    clock_t clock_time = clock();
    return clock_time/(double)CLOCKS_PER_SEC;
}


