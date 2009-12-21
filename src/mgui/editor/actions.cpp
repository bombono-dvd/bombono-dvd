//
// mgui/editor/actions.cpp
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

#include <mgui/_pc_.h>

#include "actions.h"
#include "render.h"
#include "toolbar.h"

#include <mgui/project/menu-actions.h>
#include <mgui/project/dnd.h>

#include <mlib/sdk/logger.h>

void AddStandardEvents(Gtk::Widget& wdg)
{
    wdg.add_events(
        Gdk::EXPOSURE_MASK  |   // прорисовка
        //
        // для сигнала "configure-event"; но он предназначен, в оригинале, только для окон
        // верхнего уровня (GtkWindow, ...) + искусственно посылается в случае GtkDrawingArea 
        // (вне зависимости от маски); поэтому, в случае других виджетов (GtkLayout) пользуемся
        // сигналом "size-allocate" (который обычно и является суммарным эффектом от "configure-event" для
        // окна верхнего уровня)
        // 
        // Gdk::STRUCTURE_MASK |   
        Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |      // кнопки мыши
        Gdk::POINTER_MOTION_HINT_MASK|Gdk::POINTER_MOTION_MASK   // движения мыши (*HINT* - для получений
        // сообщений по требованию - 
        // вызов gdk_window_get_pointer())
        //Gdk::ENTER_NOTIFY_MASK |                               // когда зашли-вышли из окна - не нужно
        );

    // удобный способ (при разработке) визуально наблюдать, 
    // какая часть окна перерисовывается 
    //wdg.get_window()->set_debug_updates();
}

Point DisplayAspectOrDef(MenuRegion* m_rgn)
{
    return m_rgn ? m_rgn->GetParams().DisplayAspect() : MenuParams(true).DisplayAspect();
}

Point DisplaySizeOrDef(MenuRegion* m_rgn)
{
    return m_rgn ? m_rgn->GetParams().Size() : MenuParams(true).Size();
}

void RenderForRegion(MEditorArea& edt_area, RectListRgn& rct_lst)
{
    MenuRegion& m_rgn = edt_area.CurMenuRegion();
    if( ReDivideRects(rct_lst, m_rgn) )
    {
        if( edt_area.StandAlone() )
        {
            RenderEditor(edt_area, rct_lst);

            edt_area.DrawUpdate(rct_lst);
        }
        else
            RenderMenuSystem(GetOwnerMenu(&m_rgn), rct_lst);
    }
}

void RenderForRegion(MEditorArea& edt_area, const Rect& rel_rct)
{
    RectListRgn rct_lst;
    rct_lst.push_back(rel_rct);
    RenderForRegion(edt_area, rct_lst);
}

int GetObjectAtPos(MEditorArea& edt_area, const Point& lct)
{
    SelVis sel_vis(lct.x, lct.y);
    edt_area.CurMenuRegion().Accept(sel_vis);
    return sel_vis.selPos;
}

inline Gdk::ModifierType GetKeyboardState()
{
    Gdk::ModifierType mask;
    gdk_window_get_pointer(0, 0, 0, (GdkModifierType*)&mask);
    return mask;
}

inline bool IsControlPressed()
{
    return GetKeyboardState() & Gdk::CONTROL_MASK;
}

void Editor::Kit::on_drag_data_received(const RefPtr<Gdk::DragContext>& context, int x, int y, 
                                        const Gtk::SelectionData& selection_data, guint info, guint time)
{
    CheckSelFormat(selection_data);
    if(  selection_data.get_target() == Project::MediaItemDnDTVType() )
    {
        typedef Gtkmm2ext::SerializedObjectPointers<Project::MediaItem> SOPType;
        SOPType& dat = GetSOP<Project::MediaItem>(selection_data);

        if( dat.data.size() == 1 ) // только если выделен один объект
        {
            Project::MediaItem mi = *dat.data.begin();
            if( IsControlPressed() )
            {
                if( !IsMenu(mi) )
                    SetBackgroundLink(*this, mi); // меняем фон
            }
            else
            {
                int pos = GetObjectAtPos(*this, Point(x, y));
                if( pos == -1 )
                {
                    // добавляем новый 
                    const Planed::Transition tr = Transition();
                    Point center = tr.RelToAbs(tr.DevToRel(Point(x, y)));
                    Editor::AddFTOItem(*this, center, mi);
                }
                else
                {
                    // меняем ссылку
                    int_array single;
                    SetObjectsLinks(*this, mi, (single.push_back(pos), single));
                }
            }
        }
    }

    return MyParent::on_drag_data_received(context, x, y, selection_data, info, time);
}

static void DrawDndFrame(RGBA::Drawer& drw, const Rect& dnd_rct)
{
    Rect rct(dnd_rct);
    // обводим внутри
    rct.rgt -= 1;
    rct.btm -= 1;
    if( !rct.IsValid() )
        return;

    drw.SetForegroundColor(BLUE_CLR);
    drw.MoveTo(rct.lft, rct.top);
    drw.FrameRectTo(rct.rgt, rct.btm);
}

void RenderEditor(MEditorArea& edt_area, RectListRgn& rct_lst)
{
    RenderVis r_vis(edt_area.SelArr(), rct_lst);
    edt_area.CurMenuRegion().Accept(r_vis);

    DrawDndFrame(*r_vis.GetDrawer(), edt_area.DndSelFrame());
}

void Editor::Kit::SetDndFrame(const Rect& dnd_rct)
{
    if( dndSelFrame != dnd_rct ) // нужна перерисовка
    {
        RectListRgn rct_lst;
        RGBA::RectListDrawer lst_drawer(rct_lst);
        DrawDndFrame(lst_drawer, dndSelFrame);
        DrawDndFrame(lst_drawer, dnd_rct);

        // *
        dndSelFrame = dnd_rct;

        RenderForRegion(*this, rct_lst);
    }
}

void Editor::Kit::on_drag_leave(const RefPtr<Gdk::DragContext>& context, guint time)
{
    SetDndFrame(Rect());
    return MyParent::on_drag_leave(context, time);
}

bool Editor::Kit::on_drag_motion(const RefPtr<Gdk::DragContext>& context, int x, int y, guint time)
{
    // :TODO: исправить ошибку с вызовом Widget::drag_get_data()
    // и перенести код в Editor::Kit::on_drag_data_received()

    Rect frame_rct = FramePlacement();
    if( !frame_rct.Contains(Point(x, y)) )
    {
        SetDndFrame(Rect());

        // возвращения одного false не хватает почему-то
        context->drag_status((Gdk::DragAction)0, time);
        return false;
    }

    Rect dnd_rct;
    if( IsControlPressed() )
        dnd_rct = Rect0Sz(frame_rct.Size());
    else
    {
        int pos = GetObjectAtPos(*this, Point(x, y));
        if( pos != -1 )
            if( Comp::MediaObj* obj = dynamic_cast<Comp::MediaObj*>(CurMenuRegion().List()[pos]) )
                dnd_rct = Planed::AbsToRel(Transition(), obj->Placement());
    }
    SetDndFrame(dnd_rct);

    return MyParent::on_drag_motion(context, x, y, time);
}
