//
// mgui/sdk/textview.h
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

#ifndef __MGUI_SDK_TEXTVIEW_H__
#define __MGUI_SDK_TEXTVIEW_H__

#include <mgui/mguiconst.h>

#include <mlib/function.h>

#include <string>
#include <utility>

Gtk::Widget& PackDetails(Gtk::TextView& txt_view);

typedef std::pair<Gtk::TextIter, Gtk::TextIter> TextIterRange;
typedef boost::function<void(RefPtr<Gtk::TextTag>)> InitTagFunctor;

TextIterRange AppendText(Gtk::TextView& txt_view, const std::string& text);
void ApplyTag(Gtk::TextView& txt_view, const TextIterRange& tir, 
              const std::string& tag_name, InitTagFunctor fnr);

TextIterRange AppendNewText(Gtk::TextView& txt_view, const std::string& line, bool is_out);
void AppendCommandText(Gtk::TextView& txt_view, const std::string& title);

#endif // #ifndef __MGUI_SDK_TEXTVIEW_H__

