//
// mgui/rectlist.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009 Ilya Murav'jov
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

#ifndef __MGUI_RECTLIST_H__
#define __MGUI_RECTLIST_H__

#include <mlib/geom2d.h>
#include <vector>

// тип для хранения областей отрисовки
typedef std::vector<Rect> RectListRgn;
typedef RectListRgn::iterator RLRIterType;
typedef RectListRgn::const_iterator RLRIterCType;

// рассчитываем положения прямоугольников при отображении Rect(Point(0, 0), src_sz) -> plc
void MapRectList(RectListRgn& dst, RectListRgn& src, const Rect& plc, const Point& src_sz);


#endif // __MGUI_RECTLIST_H__

