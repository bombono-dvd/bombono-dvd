//
// mgui/editor/kit.h
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

#ifndef __MGUI_EDITOR_KIT_H__
#define __MGUI_EDITOR_KIT_H__

#include "region.h" 
#include "tool.h" 

#include <mgui/text_style.h> 
#include <mgui/render/rgba.h> 

namespace Editor
{

// размер изображений инструментов
// в панели инструментов никто не должен превышать его
// (чтобы кнопки инструментов были квадратными, чего
// пользователь и ждет от панели инструментов)
const int TOOL_IMAGE_SIZE = 24;

struct Toolbar
{
    Gtk::RadioToolButton  selTool; // кнопки инструментов
    Gtk::RadioToolButton  txtTool;

           Gtk::ComboBox  frame_combo; // выбор рамки

  Gtk::ComboBoxEntryText  fontFmlEnt;
  Gtk::ComboBoxEntryText  fontSzEnt;
           
       Gtk::ToggleButton  bldBtn; // кнопки стилей текста
       Gtk::ToggleButton  itaBtn;
       Gtk::ToggleButton  undBtn;
        Gtk::ColorButton  clrBtn;
       Gtk::ToggleButton  frmBtn; // безопасная рамка

                          Toolbar();
                          // по font* составить
               TextStyle  GetFontDesc();   
};

// редактор меню с панелью инструментов и прочим
class Kit: public /*Gtk::DrawingArea*/ DisplayArea, public EditorRegion,
           public MEdt::ToolData
{
    typedef Gtk::DrawingArea MyParent;
    public:
                     Kit();

               void  LoadMenu(Project::Menu menu);
          VideoArea& GetVA() { return *this; }
               bool& StandAlone() { return standAlone; }
    Editor::Toolbar& Toolbar() { return toolbar; }

//        virtual bool  on_expose_event(GdkEventExpose* event);
//        virtual bool  on_configure_event(GdkEventConfigure* event);
       virtual bool  on_button_press_event(GdkEventButton* event);
       virtual bool  on_button_release_event(GdkEventButton* event);
       virtual bool  on_motion_notify_event(GdkEventMotion* event);
       virtual bool  on_focus_in_event(GdkEventFocus* event);
       virtual bool  on_focus_out_event(GdkEventFocus* event);
       virtual bool  on_key_press_event(GdkEventKey* event);
       //virtual bool  on_enter_notify_event(GdkEventCrossing* event);

                     // DnD
       virtual void  on_drag_data_received(const RefPtr<Gdk::DragContext>& context, int x, int y, 
                                           const Gtk::SelectionData& selection_data, guint info, guint time);
       virtual void  on_drag_leave(const RefPtr<Gdk::DragContext>& context, guint time);
       virtual bool  on_drag_motion(const RefPtr<Gdk::DragContext>& context, int x, int y, guint time);

         const Rect& DndSelFrame() { return dndSelFrame; }

                     // обнулить для перерисовки список областей (в относительных координатах)
               void  DrawUpdate(RectListRgn& rct_lst);

    protected:
                    bool  standAlone; // проведение обновлений - только себя или все меню
         Editor::Toolbar  toolbar;

                    Rect  dndSelFrame; // при dnd отрисовка рамки

       virtual void  DoOnConfigure(bool is_update);

void SetDndFrame(const Rect& dnd_rct);
void RecalcForDndFrame(RGBA::Drawer* lst_drawer, const Rect& dnd_rct);
};

class ClearInitTextVis: public GuiObjVisitor
{
    typedef CommonGuiVis MyParent;
    public:
                  ClearInitTextVis(RefPtr<Gdk::Pixbuf> pix)
                    : mEdt(0), canvPix(pix) { }

   virtual  void  Visit(TextObj& t_obj);
            void  SetEditor(Kit* m_edt) { mEdt = m_edt; }

    protected:

            RefPtr<Gdk::Pixbuf> canvPix;
                   Kit* mEdt;
};

void ClearLocalData(MenuRegion& m_rgn, const std::string& tag = std::string());

} // namespace Editor

#endif // __MGUI_EDITOR_KIT_H__


