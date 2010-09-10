//
// mgui/timeline/mviewer.h
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

#ifndef __MGUI_TIMELINE_MVIEWER_H__
#define __MGUI_TIMELINE_MVIEWER_H__

#include <mlib/function.h> 

typedef boost::function<void(const char*, Gtk::FileChooser&)> OpenFileFnr;
// add_open_button = true - для просмотрщика mviewer
// false - для самого приложения
ActionFunctor PackFileChooserWidget(Gtk::Container& contr, OpenFileFnr fnr, bool is_mviewer);
Gtk::Label& MakeTitleLabel(const char* name);

void RunMViewer();

#endif // #ifndef __MGUI_TIMELINE_MVIEWER_H__

