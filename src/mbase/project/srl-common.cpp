//
// mbase/project/srl-common.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008, 2010 Ilya Murav'jov
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

#include <mbase/_pc_.h>

#include "srl-common.h"
#include <mdemux/util.h>

namespace Project
{

void Serialize(Archieve& ar, MenuParams& mp)
{
    ar & NameValue("Resolution", mp.GetSize());
    ar & NameValue("Aspect",  mp.GetAF());
}

std::string ToString(const RGBA::Pixel& pxl)
{
    using Mpeg::set_hms;
    return (str::stream("#") << std::hex 
            << set_hms() << (int)pxl.red 
            << set_hms() << (int)pxl.green 
            << set_hms() << (int)pxl.blue << (int)pxl.alpha).str();
}

// как pango_color_parse()
static bool ParseHex2(const char* src, unsigned char& c)
{
    bool res = true;
    int len = 2;
    for( const char* end = src + len; src != end; src++ )
        if( g_ascii_isxdigit(*src) )
            c = (c << 4) | g_ascii_xdigit_value(*src);
        else
        {
            res = false;
            break;
        }
    return res;
}

bool ParseColor(RGBA::Pixel& pxl, const std::string& clr_str)
{
    bool res = false;
    if( (clr_str.size() == 1+8) && (clr_str[0] == '#') )
    {
        if( ParseHex2(clr_str.c_str()+1, pxl.red)   &&
            ParseHex2(clr_str.c_str()+3, pxl.green) &&
            ParseHex2(clr_str.c_str()+5, pxl.blue)  &&
            ParseHex2(clr_str.c_str()+7, pxl.alpha) )
            res = true;
    }

    if( !res )
        pxl = RGBA::Pixel(); // черный если ошибки
    return res;
}

RGBA::Pixel MakeColor(const std::string& clr_str)
{
    RGBA::Pixel clr;
    ParseColor(clr, clr_str);
    return clr;
}

} // namespace Project

bool CanSrl(Project::Archieve& ar, int req_ver)
{
    bool res = ar.IsSave();
    if( !res )
    {
        int load_ver = ar.loadVer;
        ASSERT( load_ver != NO_HNDL );
        res = load_ver >= req_ver;
    }
    return res;
}

