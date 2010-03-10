//
// mgui/editor/toolbar.h
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

#ifndef __MGUI_EDITOR_TOOLBAR_H__
#define __MGUI_EDITOR_TOOLBAR_H__

#include <gtk/gtkradiotoolbutton.h>
#include "kit.h"

namespace Gtk 
{

// ошибка в Gtk::RadioToolButton - метод set_group не обновляет группу,
// как это сделано в Gtk::RadioButton -> написал свою правильную версию set_group()
inline void set_group(RadioToolButton& btn, RadioButtonGroup& grp)
{
    btn.set_group(grp);
    grp = btn.get_group();
}

} // namespace Gtk

void ChangeToSelectTool(MEditorArea& edt_area);
void ChangeToTextTool(MEditorArea& edt_area);

namespace Project
{

void AddFTOItem(MenuRegion& menu_rgn, const std::string& theme, const Rect& lct, MediaItem mi);
void AddMenuItem(MenuRegion& menu_rgn, Comp::Object* obj);

// вспомогательная структура для передачи меню ссылок
// от браузеров к редактору
struct SetLinkMenu
{
                  bool  isForBack; // нужны ссылки для фона
    ptr::one<Gtk::Menu> linkMenu;
             MediaItem  newLink;

    SetLinkMenu(): isForBack(false) {}
};

} // namespace Project

namespace Editor
{

extern Gtk::TreeModelColumn<std::string> FrameTypeColumn;
std::string GetActiveTheme(Gtk::ComboBox& combo);
Gtk::Toolbar& PackToolbar(MEditorArea& editor, Gtk::VBox& lct_box);

const double DEF_FONT_SIZE = 28.0;
const int FONT_SZ_PRECISION = 6;

typedef boost::function<bool(FrameThemeObj*, MenuRegion&)> FTOFunctor;
typedef boost::function<bool(TextObj*, MenuRegion&)> TextObjFunctor;
void ForAllSelected(FTOFunctor fto_fnr, TextObjFunctor txt_fnr, MenuRegion& mn_rgn, const int_array& sel_arr);

void ForAllSelectedFTO(FTOFunctor fnr, MenuRegion& mn_rgn, const int_array& sel_arr);

void AddFTOItem(MEditorArea& editor, const Point& center, Project::MediaItem mi);

} // namespace Editor

void SetSelObjectsLinks(MEditorArea& edt_area, Project::MediaItem mi, bool is_background);
void SetSelObjectsTStyle(MEditorArea& edt_area, const Editor::TextStyle& ts, bool only_clr);

void SetBackgroundLink(MEditorArea& edt_area, Project::MediaItem mi);
void SetLinkForObject(MEditorArea& edt_area, Project::MediaItem mi, int pos, bool for_poster);

namespace Project {

class CommonMenuBuilder
{
    public:
                           CommonMenuBuilder(MediaItem cur_itm, MEditorArea& ed, bool for_poster): 
                               curItm(cur_itm), editor(ed), forPoster(for_poster) {}
    virtual               ~CommonMenuBuilder() {}

    virtual ActionFunctor  CreateAction(MediaItem mi) = 0;

                Gtk::Menu& Create();

    protected:
              MediaItem  curItm; // текущая ссылка
            MEditorArea& editor;
                   bool  forPoster; // визуальная ссылка или для переходов (может быть меню, не может быть рисунком)

Gtk::RadioMenuItem& 
AddMediaItemChoice(Gtk::Menu& lnk_list, MediaItem mi, Gtk::RadioButtonGroup& grp,
                   const std::string& name = std::string());
};

} // namespace Project

#endif // __MGUI_EDITOR_TOOLBAR_H__

