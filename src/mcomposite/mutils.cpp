//
// mutils.cpp
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

#include "mconst.h"
#include "mutils.h"

Point MovieInfo::PixelAspect() const
{
    y4m_ratio_t ratio = y4m_si_get_sampleaspect(&streamInfo);
    return Point(ratio.n, ratio.d);
}

// :TODO: надо в MovieInfo делать полноценный класс - 
// справка о медиа (кодек, тип файла, миксер, частота кадров ...)
Point MovieInfo::AspectRadio() const
{
    Point res = PixelAspect();

    Point sz = Size();
    ReducePair(sz);

    res.x *= sz.x;
    res.y *= sz.y;
    ReducePair(res);
    return res;
}

