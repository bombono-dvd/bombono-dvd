//
// mgui/sdk/packing.h
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

#ifndef __MGUI_SDK_PACKING_H__
#define __MGUI_SDK_PACKING_H__

//
// NewManaged()
// 

template<class GtkObj>
GtkObj& NewManaged()
{
    return *Gtk::manage(new GtkObj);
}

template<class GtkObj, class Arg1>
GtkObj& NewManaged(const Arg1& arg1)
{
    return *Gtk::manage(new GtkObj(arg1));
}

template<class GtkObj, class Arg1>
GtkObj& NewManaged(Arg1& arg1)
{
    return *Gtk::manage(new GtkObj(arg1));
}

template<class GtkObj, class Arg1, class Arg2>
GtkObj& NewManaged(const Arg1& arg1, const Arg2& arg2)
{
    return *Gtk::manage(new GtkObj(arg1, arg2));
}

template<class GtkObj, class Arg1, class Arg2>
GtkObj& NewManaged(Arg1& arg1, Arg2& arg2)
{
    return *Gtk::manage(new GtkObj(arg1, arg2));
}

template<class GtkObj, class Arg1, class Arg2, class Arg3>
GtkObj& NewManaged(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
{
    return *Gtk::manage(new GtkObj(arg1, arg2, arg3));
}

template<class GtkObj, class Arg1, class Arg2, class Arg3>
GtkObj& NewManaged(Arg1& arg1, Arg2& arg2, Arg3& arg3)
{
    return *Gtk::manage(new GtkObj(arg1, arg2, arg3));
}

template<class GtkObj, class Arg1, class Arg2, class Arg3, class Arg4>
GtkObj& NewManaged(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
{
    return *Gtk::manage(new GtkObj(arg1, arg2, arg3, arg4));
}

template<class GtkObj, class Arg1, class Arg2, class Arg3, class Arg4>
GtkObj& NewManaged(Arg1& arg1, Arg2& arg2, Arg3& arg3, Arg4& arg4)
{
    return *Gtk::manage(new GtkObj(arg1, arg2, arg3, arg4));
}

//

template<class GtkObj>
GtkObj& PackStart(Gtk::Box& box, GtkObj& wdg, Gtk::PackOptions opt = Gtk::PACK_SHRINK)
{
    box.pack_start(wdg, opt);
    return wdg;
}

template<class GtkObj>
GtkObj& Add(Gtk::Container& container, GtkObj& wdg)
{
    container.add(wdg);
    return wdg;
}

template<class GtkObj>
GtkObj& AddWidget(Glib::RefPtr<Gtk::SizeGroup> wdg_sg, GtkObj& wdg)
{
    wdg_sg->add_widget(wdg);
    return wdg;
}

// Разное

void PackHSeparator(Gtk::VBox& vbox);


#endif // __MGUI_SDK_PACKING_H__


