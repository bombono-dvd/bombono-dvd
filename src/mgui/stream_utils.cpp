//
// mgui/stream_utils.cpp
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

#include <mgui/_pc_.h>

#include "stream_utils.h"

namespace bin
{
using namespace std;

#define STRM_BASE_OP_IMPL_RGT(type) \
    stream& stream::operator >> (type& val)         \
    {                                               \
        io_strm.read((char*)&val, sizeof(type));    \
        return *this;                               \
    }                                               \
    /**/

STRM_BASE_OP_IMPL_RGT(bool)
STRM_BASE_OP_IMPL_RGT(char)
STRM_BASE_OP_IMPL_RGT(short)
STRM_BASE_OP_IMPL_RGT(int)
STRM_BASE_OP_IMPL_RGT(long)
STRM_BASE_OP_IMPL_RGT(float)
STRM_BASE_OP_IMPL_RGT(double)

stream& stream::read(char* buf, streamsize cnt)
{
    io_strm.read(buf, cnt);
    return *this;
}

#define STRM_BASE_OP_IMPL_LFT(type) \
    stream& stream::operator << (type val)              \
    {                                                   \
        io_strm.write((const char*)&val, sizeof(type)); \
        return *this;                                   \
    }                                                   \
    /**/

STRM_BASE_OP_IMPL_LFT(bool)
STRM_BASE_OP_IMPL_LFT(char)
STRM_BASE_OP_IMPL_LFT(short)
STRM_BASE_OP_IMPL_LFT(int)
STRM_BASE_OP_IMPL_LFT(long)
STRM_BASE_OP_IMPL_LFT(float)
STRM_BASE_OP_IMPL_LFT(double)

stream& stream::write(const char* buf, streamsize cnt)
{
    io_strm.write(buf, cnt);
    return *this;
}

} // namespace bin

bin::stream& operator >> (bin::stream& strm, std::string& str)
{
    size_t len;
    strm >> len;

    str.resize(len);
    strm.read(const_cast<char*>(str.c_str()), len);
    return strm;
}

bin::stream& operator << (bin::stream& strm, const char* str)
{
    size_t len = strlen(str);
    strm << len;
    strm.write(str, len);

    return strm;
}

bin::stream& operator << (bin::stream& strm, const std::string& str)
{
    size_t len = str.length();
    strm << len;
    strm.write(str.c_str(), len);

    return strm;
}




