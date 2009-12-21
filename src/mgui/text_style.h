//
// mgui/text_style.h
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

#ifndef __MGUI_TEXT_STYLE_H__
#define __MGUI_TEXT_STYLE_H__

#include "img_utils.h"

namespace Editor
{

struct TextStyle
{
    Pango::FontDescription  fntDsc;
                      bool  isUnderlined;
               RGBA::Pixel  color;

    TextStyle(): isUnderlined(false) {}
    TextStyle(const Pango::FontDescription& dsc, bool is_under, const RGBA::Pixel& clr)
        : fntDsc(dsc), isUnderlined(is_under), color(clr) {}
};

} // namespace Editor

#endif // __MGUI_TEXT_STYLE_H__

