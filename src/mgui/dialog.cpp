//
// mgui/dialog.cpp
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

#include "dialog.h"
#include "sdk/widget.h"
#include "sdk/packing.h"
#include "sdk/window.h" // GetTopWindow()
#include "sdk/menu.h"

#include <mlib/gettext.h>

DialogVBox& PackDialogVBox(Gtk::Box& box)
{
    return PackStart(box, NewManaged<DialogVBox>(false, 10), Gtk::PACK_EXPAND_WIDGET);
}

DialogVBox& AddHIGedVBox(Gtk::Dialog& dlg)
{
    // :KLUDGE: почему-то set_border_width() на dlg.get_vbox() не действует, поэтому использовать
    // MakeBoxHIGed() нельзя
    Gtk::VBox& box = *dlg.get_vbox();

    //return Add(PackStart(box, NewPaddingAlg(10, 10, 10, 10), Gtk::PACK_EXPAND_WIDGET), NewManaged<Gtk::VBox>(false, 10));
    DialogVBox& vbox = PackDialogVBox(box);
    vbox.set_border_width(10);
    return vbox;
}

Gtk::HBox& PackNamedWidget(Gtk::VBox& vbox, Gtk::Widget& name_wdg, Gtk::Widget& wdg,
                           RefPtr<Gtk::SizeGroup> sg, Gtk::PackOptions opt)
{
    Gtk::HBox& hbox = PackStart(vbox, NewManaged<Gtk::HBox>());

    Add(PackStart(hbox, NewPaddingAlg(0, 0, 0, 5)), AddWidget(sg, name_wdg));
    PackStart(hbox, wdg, opt);
    return hbox;
}

Gtk::Label& LabelForWidget(const char* label, Gtk::Widget& wdg)
{
    Gtk::Label& lbl = NewManaged<Gtk::Label>(label, true);
    SetAlign(lbl);
    lbl.set_mnemonic_widget(wdg);
    return lbl;
}

Gtk::Label& Pack2NamedWidget(Gtk::VBox& box, const char* label, Gtk::Widget& wdg, 
                             RefPtr<Gtk::SizeGroup> label_sg)
{
    Gtk::HBox& hbox = AppendWithLabel(box, label_sg, wdg, label, Gtk::PACK_SHRINK);
    Gtk::Label& after_lbl = NewManaged<Gtk::Label>();
    after_lbl.set_padding(5, 0);
    PackStart(hbox, after_lbl, Gtk::PACK_SHRINK);
    return after_lbl;
}

void AppendWithLabel(DialogVBox& vbox, Gtk::Widget& wdg, const char* label, Gtk::PackOptions opt)
{
    PackNamedWidget(vbox, LabelForWidget(label, wdg), wdg, vbox.labelSg, opt);
}

Gtk::HBox& AppendWithLabel(Gtk::VBox& vbox, RefPtr<Gtk::SizeGroup> sg, Gtk::Widget& wdg, 
                           const char* label, Gtk::PackOptions opt)
{
    return PackNamedWidget(vbox, LabelForWidget(label, wdg), wdg, sg, opt);
}

void AdjustDialog(Gtk::Dialog& dlg, int min_wdh, int min_hgt, 
                  Gtk::Window* parent_win, bool set_resizable)
{
    if( parent_win )
        dlg.set_transient_for(*parent_win);
    SetDialogStrict(dlg, min_wdh, min_hgt, set_resizable);
}

ptr::shared<Gtk::Dialog> MakeDialog(const char* name, int min_wdh, int min_hgt, 
                                    Gtk::Window* win)
{
    ptr::shared<Gtk::Dialog> p_dlg = new Gtk::Dialog(name, true);
    AdjustDialog(*p_dlg, min_wdh, min_hgt, win, true);

    return p_dlg;
}

bool CompleteAndRunOk(Gtk::Dialog& dlg)
{
    CompleteDialog(dlg);
    return Gtk::RESPONSE_OK == dlg.run();
}

void DoDialog(const DialogParams& dp)
{
    Gtk::Window* win = 0;
    if( dp.parWdg )
        win = GetTopWindow(*dp.parWdg);
    // клиент может повторно установить имя диалога (set_title)
    ptr::shared<Gtk::Dialog> dlg = MakeDialog(dp.name.c_str(), dp.minSz.x, dp.minSz.y, win);
    dp.fnr(*dlg);
}

ActionFunctor DoDialogFunctor(const DialogParams& dp)
{
    return bb::bind(&DoDialog, dp);
}

void AddDialogItem(Gtk::Menu& menu, const DialogParams& dp, bool is_enabled)
{
    std::string dlg_name = _dots_(dp.name.c_str());
    AddEnabledItem(menu, dlg_name.c_str(), DoDialogFunctor(dp), is_enabled); 
}

