//
// mgui/sdk/window.cpp
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

#include "window.h"

void RunWindow(Gtk::Window& win)
{
    win.show_all();
    Gtk::Main::run(win);
}

Gtk::Window* GetTopWindow(Gtk::Widget& wdg)
{
    return dynamic_cast<Gtk::Window*>(wdg.get_toplevel());
}

Point CalcBeautifulRect(int wdh)
{
    return Point(wdh, wdh*3/5);
}

void SetAppWindowSize(Gtk::Window& win, int wdh)
{
    Point sz = CalcBeautifulRect(wdh);
    win.set_default_size(sz.x, sz.y);
}

