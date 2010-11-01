//
// mgui/sdk/widget.cpp
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

#include "widget.h"
#include <mgui/img_utils.h>

void SetScaleSecondary(Gtk::HScale& scl)
{
    scl.property_draw_value() = false;
    scl.property_can_focus()  = false;
}

void ConfigureSpin(Gtk::SpinButton& btn, double val, double max)
{
    // по мотивам gtk_spin_button_new_with_range()
    int step = 1;
    btn.configure(*Gtk::manage(new Gtk::Adjustment(val, 1, max, step, 10*step, 0)), step, 0);
    btn.set_numeric(true);
}

RGBA::Pixel GetColor(const Gtk::ColorButton& btn)
{
    RGBA::Pixel pxl(RGBA::ColorToPixel(btn.get_color()));
    pxl.alpha = RGBA::FromGdkComponent(btn.get_alpha());
    return pxl;
}

void SetColor(Gtk::ColorButton& btn, const RGBA::Pixel& pxl)
{
    btn.set_color(RGBA::PixelToColor(pxl));
    btn.set_alpha(RGBA::ToGdkComponent(pxl.alpha));
}

void ConfigureRGBAButton(Gtk::ColorButton& btn, const RGBA::Pixel& pxl)
{
    btn.set_use_alpha();
    SetColor(btn, pxl);
}

