//
// mdemux/util.h
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

#ifndef __MDEMUX_UTIL_H__
#define __MDEMUX_UTIL_H__

#include <string>
#include <mlib/sdk/stream_util.h>

namespace Mpeg 
{ 

struct set_hms { };

template<typename CharT, typename Traits> inline std::basic_ostream<CharT, Traits>& 
operator << (std::basic_ostream<CharT, Traits>& os, set_hms /*f*/)
{ 
    os.width(2); 
    os.fill('0'); 
    return os; 
}

// в формат hh:mm:ss.
std::string SecToHMS(double time, bool round_sec = false);

} // namespace Mpeg

#endif // __MDEMUX_SEEK_H__

