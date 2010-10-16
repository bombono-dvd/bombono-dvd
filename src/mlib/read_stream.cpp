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
//#include "lambda.h"
#include "bind.h"
#include "sdk/stream_util.h"

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

bool ReadAllStream(ReadFunctor fnr, io::stream& strm)
{
    return ReadStream(fnr, strm, StreamSize(strm));
}

static char* CopyToString(char* buf, int sz, std::string& res)
{
    res += std::string(buf, sz);
    return buf;
}

static std::string ReadAllStream(io::stream& strm)
{
    std::string res;
    ReadAllStream(bb::bind(&CopyToString, _1, _2, boost::ref(res)), strm);

    return res;
}

std::string ReadAllStream(const fs::path& path)
{
    io::stream strm(path.string().c_str(), iof::in);
    return ReadAllStream(strm);
}

void WriteAllStream(const fs::path& path, const std::string& str)
{
    io::stream strm(path.string().c_str(), iof::out);
    strm.write(str.c_str(), str.size());
}

static char* CopyFilePart(io::stream& dst, char* buf, int len)
{
    dst.write(buf, len);
    return buf;
}

ReadFunctor MakeWriter(io::stream& dst_strm)
{
    return bb::bind(&CopyFilePart, boost::ref(dst_strm), _1, _2);
}

static char* ShiftBuf(char* buf, int len)
{
    buf += len;
    return buf;
}

ReadFunctor MakeBufShifter()
{
    return bb::bind(&ShiftBuf, _1, _2);
}

