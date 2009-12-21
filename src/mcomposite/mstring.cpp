//
// mstring.cpp
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

#include "mstring.h"

#include <string.h>

namespace Str {

const char* GetFileExt(const char* fpath)
{
    const char* dot = strrchr(fpath, '.');
    if( dot ) dot++;

    return dot;
}

static bool IsInArray(const char* const *exts, const char* ext)
{
    for( const char* const* beg=exts; *beg ; beg++ )
        if( strcmp(*beg, ext) == 0 )
            return true;
    return false;
}

bool IsYuvMedia(const char* fpath)
{
    if( strcmp("-", fpath) == 0 )
        return true;

    const char* ext = GetFileExt(fpath);
    if( !ext )
        return false;


    static const char* const exts[]=
    {
        "yuv", 0
    };
    return IsInArray(exts, ext);
}

bool IsPictureMedia(const char* fpath)
{
    const char* ext = GetFileExt(fpath);
    if( !ext )
        return false;

    static const char* const exts[]=
    {
        "png", "jpg", "jpeg", "bmp",
        0
    };
    return IsInArray(exts, ext);
}

static bool GetPairFromStr(Point& pt, const char* pair_str, char c)
{
    const char* sign = strchr(pair_str, c);
    if( !sign )
        return false;

    long x, y;
    bool is_ok = GetLong(x, pair_str) && GetLong(y, sign+1);
    
    pt.x = (int)x;
    pt.y = (int)y;
    return is_ok;
}

bool GetSize(Point& pt, const char* sz_str)
{
    return GetPairFromStr(pt, sz_str, 'x');
}

bool GetOffset(Point& pt, const char* off_str)
{
    return GetPairFromStr(pt, off_str, '+');
}

bool GetGeometry(Rect& rct, const char* geom_str)
{
    // a*b+x+y
    const char* plus = strchr(geom_str, '+');
    if( !plus )
        return false;

    std::string tmp;
    tmp.assign(geom_str, plus);
    Point sz;
    if( !GetSize(sz, tmp.c_str()) )
        return false;

    // x+y
    tmp.assign(plus+1, strlen(plus));
    Point off;
    if( !GetOffset(off, tmp.c_str()) )
        return false;

    rct.lft = off.x;
    rct.top = off.y;
    rct.rgt = off.x+sz.x;
    rct.btm = off.y+sz.y;
    return true;
}

}

