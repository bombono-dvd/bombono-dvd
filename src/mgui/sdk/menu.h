//
// mgui/sdk/menu.h
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

#ifndef __MGUI_SDK_MENU_H__
#define __MGUI_SDK_MENU_H__

#include "packing.h"

inline Gtk::MenuItem& AppendMI(Gtk::MenuShell& ms, Gtk::MenuItem& mi)
{
    ms.append(mi);
    return mi;
}

inline Gtk::Menu& MakeSubmenu(Gtk::MenuItem& mi)
{
    Gtk::Menu& sub_menu = NewManaged<Gtk::Menu>();
    mi.set_submenu(sub_menu);
    return sub_menu;
}

//
// NewPopupMenu() - создание контекстного меню
// Удаляется автоматом сразу после деактивации и выполнения соответ. действия
// Замечание: для таких меню обычно перед Popup() следует вызвать show_all()
// 
Gtk::Menu& NewPopupMenu();
inline void Popup(Gtk::Menu& mn, GdkEventButton* event, bool show_all = false)
{
    if( show_all )
        mn.show_all();
    mn.popup(event->button, event->time);
}


#endif // __MGUI_SDK_MENU_H__

