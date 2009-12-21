//
// mlib/read_stream.cpp
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

#include "const.h"
#include "read_stream.h"
#include "lambda.h"

bool ReadStream(ReadFunctor fnr, io::stream& strm, io::pos len)
{
    bool is_break = false;
    char buf[STRM_BUF_SZ];
    for( int cnt;
         cnt=std::min(len, (io::pos)STRM_BUF_SZ), cnt = cnt ? strm.raw_read(buf, cnt) : 0, cnt>0; 
         len -= cnt )
    {
        if( !fnr(buf, cnt) )
        {
            is_break = true;
            break;
        }
    }
    return is_break;
}

static char* CopyFilePart(io::stream& dst, char* buf, int len)
{
    dst.write(buf, len);
    return buf;
}

ReadFunctor MakeWriter(io::stream& dst_strm)
{
    return bl::bind(&CopyFilePart, boost::ref(dst_strm), bl::_1, bl::_2);
}

static char* ShiftBuf(char* buf, int len)
{
    buf += len;
    return buf;
}

ReadFunctor MakeBufShifter()
{
    return bl::bind(&ShiftBuf, bl::_1, bl::_2);
}

