//
// mlib/gettext.cpp
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

#include "gettext.h"
#include "tech.h"

const char* _context_gettext_(const char* msgid, size_t msgid_offset)
{
    ASSERT( msgid_offset > 0 );
    const char* trans = gettext(msgid);

    if( trans == msgid )
        // без перевода
        trans = msgid + msgid_offset;
    return trans;
}

boost::format BF_(const char* str)
{
    return boost::format(gettext(str));
}
boost::format BF_(const std::string& str)
{
    return boost::format(gettext(str.c_str()));
}

std::string _dots_(const char* str)
{
    return gettext(str) + std::string("...");
}

std::string _semicolon_(const char* str)
{
    return gettext(str) + std::string(":");
}

