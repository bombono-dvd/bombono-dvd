//
// mgui/sdk/menu.cpp
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

#include <mgui/_pc_.h>

#include "menu.h"

Gtk::MenuItem& AppendMI(Gtk::MenuShell& ms, Gtk::MenuItem& mi)
{
    ms.append(mi);
    return mi;
}

Gtk::Menu& MakeSubmenu(Gtk::MenuItem& mi)
{
    Gtk::Menu& sub_menu = NewManaged<Gtk::Menu>();
    mi.set_submenu(sub_menu);
    return sub_menu;
}

Gtk::MenuItem& MakeAppendMI(Gtk::MenuShell& ms, const char* name)
{
    return AppendMI(ms, NewManaged<Gtk::MenuItem>(name));
}

void AppendSeparator(Gtk::MenuShell& ms)
{
    ms.append(NewManaged<Gtk::SeparatorMenuItem>());
}

void Popup(Gtk::Menu& mn, GdkEventButton* event, bool show_all)
{
    if( show_all )
        mn.show_all();
    mn.popup(event->button, event->time);
}

void AddEnabledItem(Gtk::Menu& menu, const char* name, const ActionFunctor& fnr, bool is_enabled)
{
    Gtk::MenuItem& itm = MakeAppendMI(menu, name);
    itm.set_sensitive(is_enabled);
    itm.signal_activate().connect(fnr);
}

