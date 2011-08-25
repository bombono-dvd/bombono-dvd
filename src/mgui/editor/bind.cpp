//
// mgui/editor/bind.cpp
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

#include "bind.h"
#include "text.h"

#include <mgui/render/editor.h>

RefPtr<Gdk::Pixbuf> GetThemeIcon(FrameThemeObj& fto, const Point& sz)
{
    RefPtr<Gdk::Pixbuf>& fto_pix = GetDwPixbuf(fto);
    // размеры иконок заведомо небольшие, поэтому явная перезагрузка
    // не будет тормозом (+отрисовка не зависит от того, куда рисуем, на редактор,
    // или это авторинг уже)
    if( !fto_pix || (PixbufSize(fto_pix) != sz) )
        fto_pix = Editor::LoadThemeIcon(fto.ThemeName(), sz, false);
    return fto_pix;
}

bool IsIconTheme(FrameThemeObj& fto)
{
    return fto.Theme().isIcon;
}

void DoRenderFTO(RGBA::RgnPixelDrawer* drw, FrameThemeObj& fto, 
                 Editor::FTOData& fd, const Planed::Transition& trans)
{
    Rect rel_plc = RelPos(fto, trans);
    RefPtr<Gdk::Pixbuf> pix;
    if( IsIconTheme(fto) )
        pix = GetThemeIcon(fto, rel_plc.Size());
    else
        // используем кэш
        pix = fd.GetPix();
    drw->CompositePixbuf(pix, rel_plc);
}

namespace MBind {

TextRgnAcc::TextRgnAcc(EdtTextRenderer& t_rndr, RectListRgn& lst)
    : etr(t_rndr)
{ 
    etr.SetRgnAccumulator(lst); 
}

TextRgnAcc::~TextRgnAcc()
{ 
    etr.ClearRgnAccumulator();  
}

void FTORendering::Render()
{
    DoRenderFTO(drw, fto, fto.GetData<FTOInterPixData>(), trans);
}

void FTOMoving::Redraw()
{
    rLst.push_back(CalcPlc());
}

void TextRendering::Render()
{
    for( RLRIterCType cur = rLst.begin(), end = rLst.end(); cur != end; ++cur )
        tRndr.RenderByRegion(*cur);
}

TextMoving::TextMoving(RectListRgn& r_lst, EdtTextRenderer& t_rndr)
    :Moving(r_lst), rgnAcc(t_rndr, r_lst), tRndr(t_rndr)
{}

void TextMoving::Redraw()
{
    RedrawText(tRndr);
}

void TextMoving::Update()
{
    tRndr.DoLayout();
}

Rect TextMoving::CalcRelPlacement()
{
    return tRndr.CalcTextPlc();
}

} // namespace MBind

