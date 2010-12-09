//
// mgui/gettext.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

#ifndef __MGUI_GETTEXT_H__
#define __MGUI_GETTEXT_H__

#include <mlib/gettext.h>

// зависит от g_utf8_next_char() - потому здесь
std::string _remove_underscore_(const char* str);
// когда одно и то же выражение требуется в 2 вариантах - без
// и с указанием горячей клавишы, тогда вместо добавления работы
// переводчику можно из переведенного (в UTF-8 только) убрать 
// подчеркивание(я).
#define RMU_(str) _remove_underscore_(str).c_str()

#endif // #ifndef __MGUI_GETTEXT_H__

