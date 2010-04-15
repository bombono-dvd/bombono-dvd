//
// mgui/design.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2010 Ilya Murav'jov
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

#ifndef __MGUI_DESIGN_H__
#define __MGUI_DESIGN_H__

#include <gtkmm/widget.h>
//
// Все, что касается дизайна в рамках всего проекта
// 

typedef unsigned int uint;

// для VideoArea
inline void SetVideoBackground(Gtk::Widget& wdt)
{
    // сменим цвет чтоб отделить виджет
    wdt.modify_bg(Gtk::STATE_NORMAL, Gdk::Color("light grey")); // устанавливается при "реализации" виджета
}

//
// Монтажное окно
//

// Цвета

// белый
const uint WHITE_CLR      = 0xffffffff;
const uint TOP_LFT_CLR    = WHITE_CLR;
const uint BLACK_CLR      = 0x000000ff;
const uint TRANS_CLR      = 0x00000000;

// оригинальные цвета
//const uint SCALE_CLR      = 0xd5d5d5ff;
//const uint TRACK_CLR      = 0xc2cedbff;
const uint TRACK_CLR      = 0xbdd1eaff;
const uint BLUE_CLR       = 0x3376ffff;
const uint RED_CROSS_LINE = 0xff2d1dff;
const uint GOLD_CLR       = 0xf0e060ff; // цвет текста по умолчанию
//
// Общая компоновка окна
// 

const uint WDG_BORDER_WDH = 6;   // ширина отступов между основными виджетами
const uint BROWSER_WDH    = 230; // ширина виджетов ОВФ(окно выбора файлов) и MediaBrowser

#endif // __MGUI_DESIGN_H__


