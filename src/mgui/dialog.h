//
// mgui/dialog.h
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

#ifndef __MGUI_DIALOG_H__
#define __MGUI_DIALOG_H__

#include <boost/function.hpp>


// классический диалог сообщения, "Message Box"
Gtk::ResponseType MessageBox(const std::string& msg_str, Gtk::MessageType typ,
                             Gtk::ButtonsType b_typ, const std::string& desc_str = std::string(),
                             bool def_ok = false);

typedef boost::function<void(Gtk::MessageDialog&)> MDFunctor;
Gtk::ResponseType MessageBoxEx(const std::string& msg_str, Gtk::MessageType typ,
                               Gtk::ButtonsType b_typ, const std::string& desc_str, const MDFunctor& fnr);


void SetWeblinkCallback(Gtk::MessageDialog& mdlg);
inline
Gtk::ResponseType MessageBoxWeb(const std::string& msg_str, Gtk::MessageType typ,
                                Gtk::ButtonsType b_typ, const std::string& desc_str)
{
    return MessageBoxEx(msg_str, typ, b_typ, desc_str, SetWeblinkCallback);
}

inline void ErrorBox(const std::string& err_msg)
{
    MessageBox(err_msg, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
}

void AddCancelDoButtons(Gtk::Dialog& dialog, Gtk::BuiltinStockID do_id);
inline void AddCancelSaveButtons(Gtk::Dialog& dialog)
{ AddCancelDoButtons(dialog, Gtk::Stock::SAVE); }
void BuildChooserDialog(Gtk::FileChooserDialog& dialog, bool is_open, Gtk::Widget& for_wdg);

std::string MakeMessageBoxTitle(const std::string& title);

// запросить у пользователя файл для сохранения
bool ChooseFileSaveTo(std::string& fname, const std::string& title, Gtk::Widget& for_wdg);
bool CheckKeepOrigin(const std::string& fname);

Gtk::VBox& AddHIGedVBox(Gtk::Dialog& dlg);
void CompleteDialog(Gtk::Dialog& dlg);


#endif // #ifndef __MGUI_DIALOG_H__

