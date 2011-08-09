//
// mgui/editor/toolbar.h
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

FrameThemeObj* NewFTO(const FrameTheme& theme, const Rect& lct);
void AddMenuItem(MenuRegion& menu_rgn, Comp::Object* obj);

//// вспомогательная структура для передачи меню ссылок
//// от браузеров к редактору
//struct SetLinkMenu
//{
//                  bool  isForBack; // нужны ссылки для фона
//    ptr::one<Gtk::Menu> linkMenu;
//             MediaItem  newLink;
//
//    SetLinkMenu(): isForBack(false) {}
//};

typedef boost::function<std::string(const Gtk::TreeModel::iterator& iter)> I2TFunctor;

void SetTextRendererFnr(Gtk::TreeView::Column& name_cln, Gtk::CellRendererText& rndr, 
                        const I2TFunctor& fnr);
void RenderField(Gtk::CellRenderer* rndr, const Gtk::TreeModel::iterator& iter,
                 const I2TFunctor& fnr);
} // namespace Project

namespace Editor
{

// значение итератора из списка (combobox) тем
FrameTheme Iter2FT(const Gtk::TreeModel::iterator& iter);
    
FrameTheme GetActiveTheme();
Gtk::Toolbar& PackToolbar(MEditorArea& editor, Gtk::VBox& lct_box);

const double DEF_FONT_SIZE = 28.0;
const int FONT_SZ_PRECISION = 6;

typedef boost::function<bool(FrameThemeObj*, MenuRegion&)> FTOFunctor;
typedef boost::function<bool(TextObj*, MenuRegion&)> TextObjFunctor;
void ForAllSelected(FTOFunctor fto_fnr, TextObjFunctor txt_fnr);

void ForAllSelectedFTO(FTOFunctor fnr);

void AddFTOItem(MEditorArea& editor, const Point& center, Project::MediaItem mi);

} // namespace Editor

void SetSelObjectsTStyle(MEditorArea& edt_area, const Editor::TextStyle& ts, bool only_clr);

void SetBackgroundLink(Project::MediaItem mi);
void SetLinkForObject(MEditorArea& edt_area, Project::MediaItem mi, int pos, bool for_poster);

void ToggleSafeArea();

namespace Project {

struct CommonMenuBuilder
{
            MediaItem  curItm; // текущая ссылка
                       // визуальная ссылка (=> только "первичка") 
                       // для переходов (может быть меню, не может быть рисунком)
                 bool  forPoster;
                 bool  onlyWithAudio; // картинки запрещены

            Gtk::Menu& resMenu;  // результирующее меню; обязательно должно быть присоединено после Create(),
                                 // иначе утечка
Gtk::RadioButtonGroup  radioGrp;

                           CommonMenuBuilder(MediaItem cur_itm, bool for_poster, bool only_with_audio = false);
    virtual               ~CommonMenuBuilder() {}

    virtual ActionFunctor  CreateAction(MediaItem mi) = 0;

                Gtk::Menu& Create();
    virtual          void  AddConstantChoice();

Gtk::RadioMenuItem& AddMediaItemChoice(Gtk::Menu& lnk_list, MediaItem mi, const std::string& name = std::string());
void AddPredefinedItem(const std::string& label, bool is_active, const ActionFunctor& fnr);
};

void AppendRadioItem(Gtk::RadioMenuItem& itm, bool is_active, const ActionFunctor& fnr, Gtk::Menu& lnk_list);
void AddNoLinkItem(CommonMenuBuilder& mb, bool exp_link);

class EditorMenuBuilder: public CommonMenuBuilder
{
    typedef CommonMenuBuilder MyParent;
    public:
                           EditorMenuBuilder(MediaItem cur_itm, MEditorArea& ed, bool for_poster): 
                               MyParent(cur_itm, for_poster), editor(ed) {}
    protected:
            MEditorArea& editor;
};

class EndActionMenuBld: public CommonMenuBuilder
{
    typedef CommonMenuBuilder MyParent;
    public:
    typedef boost::function<void(EndActionMenuBld&)> Functor;

                    EndActionMenuBld(PostAction& pa, const ActionFunctor& on_updater,
                                     const Functor& cc_adder);

                     void  AddConstantItem(const std::string& label, PostActionType typ);

    virtual ActionFunctor  CreateAction(Project::MediaItem mi);
    virtual          void  AddConstantChoice();

    protected:
                 PostAction& pAct;
        const ActionFunctor  onUpdater;
              const Functor  ccAdder;
};

} // namespace Project

#endif // __MGUI_EDITOR_TOOLBAR_H__

