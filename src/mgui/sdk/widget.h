//
// mgui/sdk/widget.h
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

#ifndef __MGUI_SDK_WIDGET_H__
#define __MGUI_SDK_WIDGET_H__

#include <mlib/function.h>

void SetPercent(Gtk::ProgressBar& bar, double percent);

// "прокрутить" очередь сообщений (по сути Muzzle'овский CheckBreak())
void IteratePendingEvents();

inline void SetScaleSecondary(Gtk::HScale& scl)
{
    scl.property_draw_value() = false;
    scl.property_can_focus()  = false;
}

inline void SetAlign(Gtk::Label& lbl, bool is_left = true)
{
    lbl.set_alignment(is_left ? 0.0 : 1.0, 0.5);
}

typedef boost::function<void(GtkWidget*)> GtkWidgetFunctor;
void ForAllWidgets(GtkWidget* wdg, const GtkWidgetFunctor& fnr);

Gtk::Alignment& NewPaddingAlg(int top, int btm, int lft, int rgt);

Gtk::ScrolledWindow& PackInScrolledWindow(Gtk::Widget& wdg, bool need_hz = false);

// удобная функция упаковки виджета в рамку
Gtk::Frame& NewManagedFrame(Gtk::ShadowType st = Gtk::SHADOW_ETCHED_IN, 
                            const std::string& label = std::string());
Gtk::Frame& PackWidgetInFrame(Gtk::Widget& wdg, Gtk::ShadowType st,
                              const std::string& label = std::string());

#endif // #ifndef __MGUI_SDK_WIDGET_H__

