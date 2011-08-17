//
// mgui/dialog.h
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

#ifndef __MGUI_DIALOG_H__
#define __MGUI_DIALOG_H__

#include <mgui/mguiconst.h>

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

inline void ErrorBox(const std::string& err_msg, const std::string& desc_str = std::string())
{
    MessageBox(err_msg, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, desc_str);
}

void AddCancelDoButtons(Gtk::Dialog& dialog, Gtk::BuiltinStockID do_id);
inline void AddCancelSaveButtons(Gtk::Dialog& dialog)
{ AddCancelDoButtons(dialog, Gtk::Stock::SAVE); }
void BuildChooserDialog(Gtk::FileChooserDialog& dialog, bool is_open, Gtk::Widget& for_wdg);

std::string MakeMessageBoxTitle(const std::string& title);

typedef boost::function<void(Gtk::FileChooserDialog&)> FCDFunctor;
// запросить у пользователя файл для сохранения
bool ChooseFileSaveTo(std::string& fname, const std::string& title, Gtk::Widget& for_wdg,
                      const FCDFunctor& fnr = FCDFunctor());
bool CheckKeepOrigin(const std::string& fname);

//Gtk::VBox& AddHIGedVBox(Gtk::Dialog& dlg);
struct DialogVBox: public Gtk::VBox
{
    RefPtr<Gtk::SizeGroup> labelSg;

    DialogVBox(bool homogeneous = false, int spacing = 0): 
        Gtk::VBox(homogeneous, spacing), labelSg(Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL)) {}
};

DialogVBox& AddHIGedVBox(Gtk::Dialog& dlg);
DialogVBox& PackDialogVBox(Gtk::Box& box);

void AppendWithLabel(DialogVBox& vbox, Gtk::Widget& wdg, const char* label,
                     Gtk::PackOptions opt = Gtk::PACK_EXPAND_WIDGET);
Gtk::HBox& AppendWithLabel(Gtk::VBox& vbox, RefPtr<Gtk::SizeGroup> sg, Gtk::Widget& wdg, 
                           const char* label, Gtk::PackOptions opt = Gtk::PACK_EXPAND_WIDGET);

Gtk::HBox&  PackNamedWidget(Gtk::VBox& vbox, Gtk::Widget& name_wdg, Gtk::Widget& wdg,
                            RefPtr<Gtk::SizeGroup> sg, Gtk::PackOptions opt);
Gtk::Label& Pack2NamedWidget(Gtk::VBox& box, const char* label, Gtk::Widget& wdg, 
                             RefPtr<Gtk::SizeGroup> label_sg);
Gtk::Label& LabelForWidget(const char* label, Gtk::Widget& wdg);


ptr::shared<Gtk::Dialog> MakeDialog(const char* name, int min_wdh, int min_hgt, 
                                    Gtk::Window* win);
void AdjustDialog(Gtk::Dialog& dlg, int min_wdh, int min_hgt, 
                  Gtk::Window* parent_win, bool set_resizable);

bool CompleteAndRunOk(Gtk::Dialog& dlg);
// close_style - вариант для настроек (без OK)
void CompleteDialog(Gtk::Dialog& dlg, bool close_style = false);

void SetDialogStrict(Gtk::Dialog& dlg, int min_wdh, int min_hgt, bool set_resizable = false);

typedef boost::function<void(Gtk::Dialog&)> DialogFunctor;

struct DialogParams
{
   std::string  name;
         Point  minSz;
 DialogFunctor  fnr;
   Gtk::Widget* parWdg;

    DialogParams(const char* name_, const DialogFunctor& fnr_, 
                 int min_wdh, Gtk::Widget* par_wdg = 0, int min_hgt = -1):
        name(name_), minSz(min_wdh, min_hgt), fnr(fnr_), parWdg(par_wdg) {}
};

void DoDialog(const DialogParams& dp);
ActionFunctor DoDialogFunctor(const DialogParams& dp);

void AddDialogItem(Gtk::Menu& menu, const DialogParams& dp, bool is_enabled = true);

std::string QuoteForGMarkupParser(const std::string& str);
std::string MarkError(const std::string& val, bool not_error);

#endif // #ifndef __MGUI_DIALOG_H__

