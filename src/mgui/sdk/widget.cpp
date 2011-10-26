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
#include "packing.h"

#include <mgui/img_utils.h>
#include <mgui/win_utils.h> // CreateButtonWithIcon()

void SetScaleSecondary(Gtk::HScale& scl)
{
    scl.property_draw_value() = false;
    scl.property_can_focus()  = false;
}

Gtk::Adjustment& CreateAdj(double val, double max, double min, int step)
{
    return *Gtk::manage(new Gtk::Adjustment(val, min, max, step, 10*step, 0));
}

void ConfigureSpin(Gtk::SpinButton& btn, double val, double max, double min, int step)
{
    // по мотивам gtk_spin_button_new_with_range()
    btn.configure(CreateAdj(val, max, min, step), step, 0);
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

Gtk::VBox& PackParaBox(Gtk::VBox& vbox)
{
    return PackStart(vbox, NewManaged<Gtk::VBox>(false, 4));
}

Gtk::Label& NewBoldLabel(const std::string& label)
{
    return NewMarkupLabel("<span weight=\"bold\">" + label + "</span>", true);
}

Gtk::VBox& PackParaBox(Gtk::VBox& vbox, const char* name)
{
    Gtk::VBox& box = PackParaBox(vbox);
    Gtk::Label& a_lbl = PackStart(box, NewBoldLabel(name));
    SetAlign(a_lbl);

    return box;
}

std::string _remove_underscore_(const char* str)
{
    std::string res;
    const char* next;
    for( const char* cur = str, *end = str + strlen(str) ; cur < end; 
         cur = next )
    {
        next = g_utf8_next_char(cur);
        if( *cur != '_' )
            res.append(cur, next);
    }
    return res;
}

// установка пустой строки в Gtk::FileChooser почему-то устанавливает
// текущий путь в него (в режиме открытия файла); и далее, последующий
// get_filename() выдает текущий путь вместо "пусто"
bool SetFilename(Gtk::FileChooser& fc, const std::string& fpath)
{
    bool res = fpath.size();
    if( res )
    {
#if !GTK_CHECK_VERSION(2,24,3)
        // Federico Mena Quintero поправил ошибку для >= 2.24.3
        // https://bugzilla.gnome.org/show_bug.cgi?id=643170
        // (наконец-то, спустя год как я оформил https://bugzilla.gnome.org/show_bug.cgi?id=615353)
        // для меньших версий проходит только, если только без скрытых файлов
        fc.set_show_hidden(false);
#endif
        res = fc.set_filename(fpath);
    }

    return res;
}

Gtk::HBox& PackCompositeWdg(Gtk::Widget& wdg)
{
    Gtk::HBox& hbox = NewManaged<Gtk::HBox>();
    PackStart(hbox, wdg, Gtk::PACK_EXPAND_WIDGET);
    return hbox;
}

void PackCompositeWdgButton(Gtk::HBox& hbox, const Gtk::BuiltinStockID& stock_id,
                            const ActionFunctor& fnr, const char* tooltip)
{
    Gtk::Button& btn = PackStart(hbox, *CreateButtonWithIcon("", stock_id, tooltip));
    btn.set_focus_on_click(false);
    btn.signal_clicked().connect(fnr);
}


