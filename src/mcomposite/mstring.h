//
// mstring.h
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

#ifndef __MSTRING_H__
#define __MSTRING_H__

#include <mlib/string.h>

#include "mutils.h"

namespace Str
{

// получить расширение файла
const char* GetFileExt(const char* fpath);

// это yuv4mpeg
bool IsYuvMedia(const char* fpath);

// это изображение
bool IsPictureMedia(const char* fpath);

// считать размеры вида AxB
bool GetSize(Point& pt, const char* sz_str);

// считать смещение вида A+B
bool GetOffset(Point& pt, const char* off_str);

// считать координаты прямоугольника вида AxB+
bool GetGeometry(Rect& rct, const char* geom_str);

}


#endif // #ifndef __MSTRING_H__

