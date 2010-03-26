//
// mgui/trackwindow.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
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

#ifndef __MGUI_TRACKWINDOW_H__
#define __MGUI_TRACKWINDOW_H__

#include <mgui/timeline/mviewer.h>
#include <mgui/sdk/widget.h>

typedef boost::function<void(Gtk::HPaned&, Timeline::DAMonitor&, TrackLayout&)> TWFunctor;

void PackTrackWindow(Gtk::Container& contr, TWFunctor tw_fnr);

inline Gtk::Widget& PackMonitorIn(Timeline::DAMonitor& mon)
{
    return PackWidgetInFrame(mon, Gtk::SHADOW_ETCHED_IN);
}

// для размещения браузера с выровненными верхом и низом
Gtk::Container& PackAlignedForBrowserTB(Gtk::Container& par_contr);
Gtk::HButtonBox& InsertButtonArea(Gtk::VBox& vbox, Gtk::ButtonBoxStyle style);

#endif // __MGUI_TRACKWINDOW_H__

