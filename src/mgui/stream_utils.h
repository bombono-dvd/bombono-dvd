//
// mgui/stream_utils.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008 Ilya Murav'jov
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

#ifndef __MGUI_STREAM_UTILS_H__
#define __MGUI_STREAM_UTILS_H__

#include <iostream>

#include <mlib/geom2d.h>

namespace bin
{

using namespace std;

// Обертка над стандарным потоком, чтобы писать бинарно.
// В перспективе надо бы привести ее к концепции (фильтра?) Boost.IOstreams
// Замечание: эта обертка - не попытка сериализации,- просто удобный инструмент
// писать данные бинарным образом.
class stream
{
    public:

                stream(std::iostream& strm): io_strm(strm) {}

 std::iostream& strm() { return io_strm; }
                operator void*() const { return (void*)io_strm; }
          bool  operator!() const { return !operator void*(); }
        stream& seekg(streamoff off, ios_base::seekdir dir)
                { io_strm.seekg(off, dir); return *this; }
        stream& seekp(streamoff off, ios_base::seekdir dir)
                { io_strm.seekp(off, dir); return *this; }

        stream& operator >> (bool& val);
        stream& operator >> (char& val);
        stream& operator >> (short& val);
        stream& operator >> (int& val);
        stream& operator >> (long& val);
        stream& operator >> (float& val);
        stream& operator >> (double& val);

        stream& read(char* buf, streamsize cnt);
 
        stream& operator << (bool val);
        stream& operator << (char val);
        stream& operator << (short val);
        stream& operator << (int val);
        stream& operator << (long val);
        stream& operator << (float val);
        stream& operator << (double val);
 
        stream& write(const char* buf, streamsize cnt);
 
        stream& operator >> (std::iostream& to_strm) { io_strm >> to_strm.rdbuf(); return *this; }
        stream& operator << (std::iostream& from_strm) { io_strm << from_strm.rdbuf(); return *this; }

    protected:

        std::iostream& io_strm;
};

} // namespace bin

//bin::stream& operator >> (bin::stream& strm, char* str);
bin::stream& operator << (bin::stream& strm, const char* str);

bin::stream& operator >> (bin::stream& strm, std::string& str);
bin::stream& operator << (bin::stream& strm, const std::string& str);

#define UNSIGNED_STRM_OP(type)      \
    inline bin::stream& operator >> (bin::stream& strm, unsigned type& val) \
    { strm >> (type&)val; return strm; }                                    \
    inline bin::stream& operator << (bin::stream& strm, unsigned type val)  \
    { strm << (type)val; return strm; }                                     \
    /**/

UNSIGNED_STRM_OP(int)
UNSIGNED_STRM_OP(char)
UNSIGNED_STRM_OP(short)
UNSIGNED_STRM_OP(long)

#undef UNSIGNED_STRM_OP

#endif // __MGUI_STREAM_UTILS_H__

