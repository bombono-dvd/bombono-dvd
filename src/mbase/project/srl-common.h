//
// mbase/project/srl-common.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2010 Ilya Murav'jov
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

#ifndef __MBASE_PROJECT_TABLE_SRL_H__
#define __MBASE_PROJECT_TABLE_SRL_H__

#include "media.h"
#include "archieve.h"
#include <mbase/pixel.h>

namespace Project
{

template<class Item> class Table;
typedef Table<MediaItem> MediaList;

void Load(Archieve& ar, MediaList& md_list);
void Save(Archieve& ar, MediaList& md_list);
APROJECT_SRL_SPLIT_FREE(MediaList)

template<typename T>
inline void Serialize(Archieve& ar, PointT<T>& pnt)
{
    ar & NameValue("x", pnt.x);
    ar & NameValue("y", pnt.y);
}

template<typename T>
inline void Serialize(Archieve& ar, RectT<T>& rct)
{
    ar & NameValue("lft", rct.lft);
    ar & NameValue("top", rct.top);
    ar & NameValue("rgt", rct.rgt);
    ar & NameValue("btm", rct.btm);
}

void Serialize(Archieve& ar, MenuParams& mp);
void SerializeReference(Archieve& ar, const char* attr_name, MediaItem& mi);

std::string ToString(const RGBA::Pixel& pxl);
RGBA::Pixel MakeColor(const std::string& clr_str);

void SerializePostAction(Archieve& ar, PostAction& pa);

// текущая версия формата хранения проектов
extern const int PRJ_VERSION;

} // namespace Project

bool CanSrl(Project::Archieve& ar, int req_ver);

#endif // #ifndef __MBASE_PROJECT_TABLE_SRL_H__

