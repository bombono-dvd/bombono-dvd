//
// mgui/project/handler.h
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

#ifndef __MGUI_PROJECT_HANDLER_H__
#define __MGUI_PROJECT_HANDLER_H__

#include <mbase/project/handler.h>

namespace Project
{

inline bool& GetBrowserDeletionSign(MediaItem mi)
{
    return mi->GetData<bool>("BrowserDeletionSign");
}

DEFINE_REG_INV_HANDLER( OnChangeName )

inline void DoNameChange(MediaItem mi, const std::string& new_name)
{
    // посылаем 2 раза - до и после
    // если понадобится различать - запишем в локальные данные boo after_change = false, true
    InvokeOnChangeName(mi);
    mi->mdName = new_name;
    InvokeOnChangeName(mi);
}

} // namespace Project

#endif // #ifndef __MGUI_PROJECT_HANDLER_H__

