//
// mgui/editor/render.cpp
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

#include "bind.h"
#include "render.h"
#include "text.h"
#include "actions.h" // DisplayAspectOrDef()

#include <mlib/lambda.h>
#include <mgui/render/menu.h>
#include <mgui/project/thumbnail.h>
#include <mgui/author/script.h>    // ConsoleMode::Flag


/////////////////////////////////////////////////////
// Отрисовка

const Editor::ThemeData& HiQuData::GetTheme(std::string& thm_nm) 
{ 
    return Editor::ThemeCache::GetTheme(thm_nm); 
}

RefPtr<Gdk::Pixbuf> FTOInterPixData::CalcSource(Project::MediaItem mi, const Point& sz)
{ 
    using namespace Project;
    RefPtr<Gdk::Pixbuf> pix;
    if( Menu mn = IsMenu(mi) )
    {
        using namespace boost;
        ShotFunctor s_fnr = lambda::bind(&GetRenderedShot, mn);
        pix = MakeCopyWithSz(GetInteractiveLRS(s_fnr), sz);
    }
    else
        pix = PrimaryShotGetter::Make(mi, sz);
    return pix;
}

/////////////////////////////////////////////////////
// RenderVis

void CommonRenderVis::VisitImpl(MenuRegion& menu_rgn)
{
    drw = new RGBA::RgnPixelDrawer(cnvBuf->Canvas(), &rLst); 
    RenderBackground();

    MyParent::VisitImpl(menu_rgn);
}

static void FillSolid(CommonRenderVis::Drawer& drw, MenuRegion* menu_rgn,
                      const Rect& plc)
{
    drw->SetForegroundColor(menu_rgn->BgColor());
    drw->Fill(plc);
}

void DoRenderBackground(CommonRenderVis::Drawer& drw, RefPtr<Gdk::Pixbuf> pix,
                        MenuRegion* m_rgn, const Rect& plc)
{
    Point sz = PixbufSize(pix);
    // первоначально сделал получение пропорций через CalcBgShot(), вместе
    // с самой картинкой, но это неадекватно: суть CalcBgShot() - получение
    // тяжелых данных наиболее оптимальным путем; остальное можно получить 
    // и напрямую
    Point dar;
    Project::MediaItem bg_mi = m_rgn->BgRef();
    if( Project::VideoItem vi = IsVideo(bg_mi) )
        dar = CalcAspectSize(*vi);
    else if( Project::ChapterItem ci = IsChapter(bg_mi) )
        dar = CalcAspectSize(*ci->owner);
    else // if( IsStillImage(bg_mi) )
        // может быть ColorMD для тестов, потому убрал проверку
        dar = sz;

    Point dst_dar = DisplayAspectOrDef(m_rgn);

    switch( m_rgn->bgSet.bsTyp )
    {
    case Project::bstPAN_SCAN:
        {
            Rect rct = FitIntoRect(PixbufSize(pix), dar, dst_dar);
            RefPtr<Gdk::Pixbuf> sub_pix = MakeSubPixbuf(pix, rct);
            if( !sub_pix )
                // вырожденные случаи вроде однопиксельного источника
                sub_pix = pix;
            drw->ScalePixbuf(sub_pix, plc);
        }
        break;
    case Project::bstLETTERBOX:
        {
            FillSolid(drw, m_rgn, plc);

            Rect rct = FitIntoRect(plc.Size(), dst_dar, dar);
            drw->ScalePixbuf(pix, rct);
        }
        break;
    case Project::bstSTRETCH:
    default:
        drw->ScalePixbuf(pix, plc);
        break;
    }
}

void CommonRenderVis::RenderBackground()
{
    Rect plc = cnvBuf->FrameRect();
    if( menuRgn->BgRef().Link() )
        DoRenderBackground(drw, CalcBgShot(), menuRgn, plc);
    else
        FillSolid(drw, menuRgn, plc);
}

RefPtr<Gdk::Pixbuf>& GetDwPixbuf(DataWare& dw)
{
    return dw.GetData<RefPtr<Gdk::Pixbuf> >();
}

void ClearDwPixbuf(DataWare& dw)
{
    ClearRefPtr(GetDwPixbuf(dw));
}

void ResetBackgroundImage(MenuRegion& mr)
{
    ClearDwPixbuf(mr);
}

RefPtr<Gdk::Pixbuf> RenderVis::CalcBgShot()
{
    RefPtr<Gdk::Pixbuf>& pix = GetDwPixbuf(*menuRgn);
    if( !pix )
        pix = Project::PrimaryShotGetter::Make(menuRgn->BgRef(), Point());
    return pix;
}

void CommonRenderVis::RenderObj(Comp::MediaObj& m_obj, MBind::Rendering& ring)
{
    ring.Render();

    Rect rel_plc = CalcRelPlacement(m_obj.Placement());
    if( IsSelected() )
        DrawGrabFrame(*drw, rel_plc);
}

void RenderVis::Visit(FrameThemeObj& fto)
{
    MBind::FTORendering ring(fto, menuRgn->Transition(), drw.get());
    RenderObj(fto, ring);
}

EdtTextRenderer& FindTextRenderer(TextObj& t_obj, CanvasBuf& cnv_buf)
{
    const std::string& tag = cnv_buf.DataTag();
    ASSERT( !(Execution::ConsoleMode::Flag && tag.empty()) );
    return tag.empty() ? t_obj.GetData<EdtTextRenderer>() : t_obj.GetData<EdtTextRenderer>(tag.c_str()) ;
}

MBind::TextRendering CommonRenderVis::Make(TextObj& t_obj)
{
    return MBind::TextRendering(rLst, FindTextRenderer(t_obj, *cnvBuf));
}

void CommonRenderVis::Visit(TextObj& t_obj)
{
    MBind::TextRendering ring(Make(t_obj));
    RenderObj(t_obj, ring);
}

