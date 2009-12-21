//
// mtheme.h
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

#ifndef __MTHEME_H__
#define __MTHEME_H__

// #include <string>

#include <Magick++.h>

#include <mbase/composite/comp_vis.h>
#include "mutils.h"

void ZoomImage(Magick::Image& dst, const Magick::Image& src, int width, int height);

Magick::Image& FrameImg(FrameThemeObj& fto);
Magick::Image& VFrameImg(FrameThemeObj& fto);

void LoadTheme(FrameThemeObj& fto);

// отобразить на холсте элемент
void Composite(Magick::Image& canv_img, const Magick::Image& obj_img, FrameThemeObj& fto); 

#endif // #ifndef __MTHEME_H__

