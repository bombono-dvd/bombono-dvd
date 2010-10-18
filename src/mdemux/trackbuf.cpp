//
// mdemux/trackbuf.cpp
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

#include <string.h>

#include "trackbuf.h"

// такую скорость прироста рекомендуют
// знатоки
const double MULT_DEGREE = 1.5;

char* Extend(char* data, int& sz, int new_sz, const int min_sz)
{
    int old_sz = sz;
    if( sz<min_sz )
        sz = min_sz;
    while( sz<new_sz )
        sz = int(sz*MULT_DEGREE);

    return (old_sz != sz) ? (char*)realloc(data, sz) : data ;
}

TrackBuf::TrackBuf() : begDat(0), endDat(0), bufSz(0),
    isUnlimited(false)
{
    // начальный размер
    Reserve(optimalSize);
}

TrackBuf::~TrackBuf()
{
    if( begDat )
        free(begDat);
}

void TrackBuf::Extend(int new_sz, bool is_add)
{
    int dat_sz = endDat-begDat;
    if( is_add )
        new_sz += dat_sz;

    if( !isUnlimited && (new_sz > MAX_STRM_BUF_SZ) )
        Error("TrackBuf: Attempt to set too big buffer size");

    begDat = ::Extend(begDat, bufSz, new_sz, optimalSize);
    endDat = begDat + dat_sz;
}

void TrackBuf::Append(const char* beg, const char* end)
{
    int app_sz = end-beg;
    Extend(app_sz, true);

    memcpy(endDat, beg, app_sz);
    endDat += app_sz;
}

void TrackBuf::AppendFromStream(io::stream& strm, int len)
{
    Extend(len, true);
    endDat += strm.raw_read(endDat, len);
}

void TrackBuf::CutStart(int off)
{
    int len = (endDat-begDat) - off;
    ASSERT( len>=0 );

    memmove(begDat, begDat+off, len);
    endDat = begDat + len;
}

