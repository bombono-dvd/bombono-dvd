//
// mgui/editor/kit.cpp
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

#include <mgui/_pc_.h>

#include "kit.h"
#include "text.h"
#include "actions.h"

#include "toolbar.h"
#include <mgui/win_utils.h>
#include <mgui/project/menu-render.h>
#include <mgui/project/dnd.h>


#include <mbase/resources.h>
#include <mlib/filesystem.h>


MouseHintAdapter::MouseHintAdapter(GdkEventMotion* ev) : event(ev) {}
MouseHintAdapter::~MouseHintAdapter()
{
    if( event->is_hint )
        gdk_window_get_pointer(event->window, 0, 0, 0);
}

namespace Editor
{

static void EditorToolToggled(Gtk::RadioToolButton& sel_btn, Kit& edt)
{
    bool is_sel_tool = sel_btn.get_active();
    is_sel_tool ? ChangeToSelectTool(edt) : ChangeToTextTool(edt) ;
}

Kit::Kit(): standAlone(true)
{
    // настройка gui-параметров окна
    set_size_request(300, 200);

    AddStandardEvents(*this);

    // события фокуса
    property_can_focus() = true;

    // по умолчанию
    ChangeToSelectTool(*this);
    using namespace boost;
    toolbar.selTool.signal_toggled().connect( 
        lambda::bind(&EditorToolToggled, boost::ref(toolbar.selTool), boost::ref(*this)) );

    LoadMenu(0);

    // * DnD, от браузеров
    std::list<Gtk::TargetEntry> targets;
    targets.push_back( Project::MediaItemDnDTVTargetEntry() );
    drag_dest_set(targets);
}

// bool Kit::on_expose_event(GdkEventExpose* event)
// {
// //     Rect fram_plc = FramePlacement();
// //     RefPtr<Gdk::Pixbuf> fram_pix = FramePixbuf();
// //
// //     Point sz = fram_plc.Size();
// //     Gdk::Rectangle frame_rct(fram_plc.lft, fram_plc.top, sz.x, sz.y);
// //     frame_rct.intersect(Gdk::Rectangle(&event->area));
// //     if( !frame_rct.has_zero_area() )
// //     {
// //         int rowstride  = fram_pix->get_rowstride();
// //         int pnt_sz = fram_pix->get_n_channels();
// //
// //         int dis_x = frame_rct.get_x() - fram_plc.lft;
// //         int dis_y = frame_rct.get_y() - fram_plc.top;
// //         guchar* pixels = fram_pix->get_pixels() + rowstride * dis_y + dis_x * pnt_sz;
// //
// //         RefPtr<Gdk::Window> p_win = get_window();
// //         RefPtr<Gdk::GC> p_gc = get_style()->get_black_gc();
// //
// //         typedef void (Gdk::Drawable::*DrawFunc) (
// //                          const RefPtr<const Gdk::GC>& gc,
// //                          int x, int y, int width, int height,
// //                          Gdk::RgbDither dith, const guchar* rgb_buf, int rowstride);
// //
// //         DrawFunc draw_func = pnt_sz == 3 ? &Gdk::Drawable::draw_rgb_image
// //             : /*=4*/ &Gdk::Drawable::draw_rgb_32_image ;
// //
// //         // просто p_win->*draw_func не проходит, из-за того что RefPtr
// //         (UnRefPtr(p_win)->*draw_func)(p_gc, frame_rct.get_x(), frame_rct.get_y(),
// //                               frame_rct.get_width(), frame_rct.get_height(),
// //                               Gdk::RGB_DITHER_NORMAL,
// //                               pixels, rowstride);
// //     }
//     DrawCanvas(*this, MakeRect(event->area));
//     return true;
// }

void ClearInitTextVis::Visit(TextObj& t_obj)
{
    EdtTextRenderer& t_rndr = t_obj.GetData<EdtTextRenderer>();

    if( !mEdt )
    {
        if( !canvPix )
            t_rndr.Clear();
        else
            t_rndr.Init(canvPix);
    }
    else
    {
        t_rndr.SetEditor(mEdt);
        t_rndr.DoLayout();
    }
}

RGBA::Pixel ColorToPixel(const CR::Color& clr)
{
    return RGBA::Pixel(RGBA::Pixel::ToQuant(clr.r), RGBA::Pixel::ToQuant(clr.g),
                       RGBA::Pixel::ToQuant(clr.b), RGBA::Pixel::ToQuant(clr.a));
}

void FillAsEmptyMonitor(RefPtr<Gdk::Pixbuf> canv_pix, Gtk::Widget& wdg)
{
    //canv_pix->fill(BLACK_CLR);

    Point sz(PixbufSize(canv_pix));
    canv_pix->fill(WHITE_CLR);

    double rel = 0.5; //2./3.; // центр логотипа правее и ниже
    // :TODO: optimize?
    RefPtr<Gdk::Pixbuf> logo_pix = Gdk::Pixbuf::create_from_file(AppendPath(GetDataDir(), "area-back.jpg"));
    Point logo_sz(PixbufSize(logo_pix));
    Point a = sz - logo_sz;
    a.x = int(a.x*rel);
    a.y = int(a.y*rel);

    Rect plc(RectASz(a, logo_sz));
    RGBA::AlphaCompositePixbuf(canv_pix, logo_pix, plc, Intersection(plc, Rect0Sz(sz)));

    // :KLUDGE: куча лишних преобразований!
    RGBA::Pixel pxl(ColorToPixel(GetBorderColor(wdg)));

    if( canv_pix->get_has_alpha() )
    {
        RGBA::PixelDrawer drw(canv_pix);
        drw.SetForegroundColor(pxl.ToUint());//BLACK_CLR);
        drw.FrameRectTo(sz.x-1, sz.y-1);
    }
}

void Kit::DoOnConfigure(bool is_update)
{
    framTrans = Planed::Transition(framPlc, DisplaySizeOrDef(curMRgn));
    if( is_update )
    {
        Point sz(framPlc.Size());
        if( curMRgn )
        {
            ClearInitTextVis txt_vis(RefPtr<Gdk::Pixbuf>(0));
            txt_vis.SetEditor(this);
            curMRgn->Accept(txt_vis);

            RenderForRegion(*this, Rect0Sz(sz));
        }
        else
        {
            RefPtr<Gdk::Pixbuf> canv_pix = VideoArea::FramePixbuf();
            if( canv_pix )
                FillAsEmptyMonitor(canv_pix, *this);
        }
    }
}

bool Kit::on_button_press_event(GdkEventButton* event)
{
    GrabFocus(*this);
    return OnButtonPressEvent(*this, toolStt, event);
}

bool Kit::on_button_release_event(GdkEventButton* event)
{
    return OnButtonReleaseEvent(*this, toolStt, event);
}

bool Kit::on_motion_notify_event(GdkEventMotion* event)
{
    return OnMotionNotifyEvent(*this, toolStt, event);
}

bool Kit::on_focus_in_event(GdkEventFocus* event)
{
    return OnFocusInEvent(*this, toolStt, event);
}

bool Kit::on_focus_out_event(GdkEventFocus* event)
{
    return OnFocusOutEvent(*this, toolStt, event);
}

bool Kit::on_key_press_event(GdkEventKey* event)
{
    //return OnKeyPressEvent(*this, toolStt, event);
    OnKeyPressEvent(*this, toolStt, event);
    // чтобы по стрелкам и Tab не переходить на другие виджеты
    return true;
}

void Kit::DrawUpdate(RectListRgn& rct_lst)
{
    Rect fram_plc = FramePlacement();
    for( RLRIterType cur=rct_lst.begin(), end=rct_lst.end(); cur != end; ++cur )
    {
        Rect& r = *cur;
        queue_draw_area( fram_plc.lft+r.lft, fram_plc.top+r.top, r.rgt-r.lft, r.btm-r.top );
    }
}

namespace {
void ClearDataWare(DataWare& dw) { dw.Clear(); }
void ClearDataWareTag(DataWare& dw, const char* tag) { dw.Clear(tag); }
} // namespace

void ClearLocalData(MenuRegion& m_rgn, const std::string& tag)
{
    using namespace boost;
    function<void(DataWare&)> clear_fnr;
    if( tag.empty() )
        clear_fnr = lambda::bind(&ClearDataWare, lambda::_1);
    else
        clear_fnr = lambda::bind(&ClearDataWareTag, lambda::_1, tag.c_str());

    clear_fnr(m_rgn);
    for( Comp::ListObj::Itr itr = m_rgn.List().begin(), end = m_rgn.List().end(); 
         itr != end; ++itr )
        clear_fnr(**itr);
}

void Kit::LoadMenu(Project::Menu menu)
{
    Project::Menu old_menu = CurMenu();
    // * заканчиваем работу со старым меню
    //ClearSel(); // убрать выделенное
    if( old_menu )
        ClearRenderFrames(*this);
    toolStt->ForceEnd(*this);

    // * старое меню возвращаем в управление 
    if( old_menu )
    {
        ClearLocalData(*curMRgn);

        Project::MenuPack& mp = old_menu->GetData<Project::MenuPack>();
        // * сохраним обновленный снимок, с редактора
        RGBA::Scale(mp.CnvBuf().FramePixbuf(), CanvasBuf::FramePixbuf());
        // * управление
        SetThumbControl(mp);
    }

    //curMenu = menu;
    //Project::LoadMenu(*this, curMenu);
    if( menu )
    {
        Project::MenuPack& mp = menu->GetData<Project::MenuPack>();
        curMRgn   = &mp.thRgn;
        renderLst = &mp.CnvBuf().RenderList();

        ClearLocalData(*curMRgn);
        curMRgn->SetCanvasBuf(this);

        mp.editor = this;
    }
    else
        curMRgn = 0;

    set_sensitive(curMRgn);
    Rebuild();
}

} // namespace Editor
