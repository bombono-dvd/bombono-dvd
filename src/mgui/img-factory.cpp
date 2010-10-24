//
// mgui/img-factory.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#include <mbase/resources.h>
#include <mlib/filesystem.h>

#include "img-factory.h"

//
// GetFactoryImage()
// 

RefPtr<Gdk::Pixbuf> GetFactoryImage(const std::string& img_str)
{
    typedef std::map<std::string, RefPtr<Gdk::Pixbuf> > FIMap;
    static FIMap fi_map; 

    FIMap::iterator itr = fi_map.find(img_str);
    if( itr == fi_map.end() )
    {
        RefPtr<Gdk::Pixbuf> img = Gdk::Pixbuf::create_from_file(AppendPath(GetDataDir(), img_str));
        fi_map[img_str] = img;
        return img;
    }
    return itr->second;
}

static void CheckEmblem(RefPtr<Gdk::Pixbuf> pix, RefPtr<Gdk::Pixbuf> emblem)
{
    Point e_sz(PixbufSize(emblem));
    Point p_sz(PixbufSize(pix));
    ASSERT( (e_sz.x <= p_sz.x) && (e_sz.y <= p_sz.y) );
}

RefPtr<Gdk::Pixbuf> GetCheckEmblem(RefPtr<Gdk::Pixbuf> pix, const std::string& emblem_str)
{
    RefPtr<Gdk::Pixbuf> emblem = GetFactoryImage(emblem_str);
    CheckEmblem(pix, emblem);
    return emblem;
}

void StampEmblem(RefPtr<Gdk::Pixbuf> pix, const std::string& emblem_str)
{
    RefPtr<Gdk::Pixbuf> emblem = GetCheckEmblem(pix, emblem_str);
    // левый нижний угол
    RGBA::AlphaComposite(pix, emblem, Point(0, pix->get_height() - emblem->get_height()));
}


