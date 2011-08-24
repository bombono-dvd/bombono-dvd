//
// mgui/render/menu.h
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

#ifndef __MGUI_RENDER_MENU_H__
#define __MGUI_RENDER_MENU_H__

#include <mgui/menu-rgn.h>
#include <mgui/editor/const.h>

#include "rgba.h"
#include "common.h"

namespace Project
{

class PixCanvasBuf: public CanvasBuf
{
    public:

                       void  Set(RefPtr<Gdk::Pixbuf> cnv, 
                                 const Planed::Transition& trans) 
                             { 
                                 canvPix   = cnv;
                                 framTrans = trans;
                             }
         RefPtr<Gdk::Pixbuf> Source() { return canvPix; }

 virtual RefPtr<Gdk::Pixbuf> Canvas() { return canvPix; }
         virtual       Rect  FramePlacement()
                             { return PixbufBounds(Canvas()); }
        virtual RectListRgn& RenderList()   { return renderLst; }

    protected:
        RefPtr<Gdk::Pixbuf> canvPix;
               RectListRgn  renderLst;
};

struct MenuPack: public DWConstructorTag
{
         MenuRegion  thRgn;
               bool  thumbNeedUpdate; // надо обновить миниатюру (после отрисовки)

 RefPtr<Gdk::Pixbuf> thumbPix; // собственно, миниатюра с cnvBuf == GetCacheShot()
        MEditorArea* editor;

                MenuPack(DataWare& dw);

  PixCanvasBuf& CnvBuf() { return cnvBuf; }
        MenuMD* Owner()  { return owner; }

    protected:
        PixCanvasBuf  cnvBuf;
              MenuMD* owner;
};

inline MenuRegion& GetMenuRegion(Menu mn) { return mn->GetData<MenuPack>().thRgn; }

// промежуточные данные для миниатюры меню
class FTOThumbData: public Editor::FTOData
{
    typedef Editor::FTOData MyParent;
    public:
                   FTOThumbData(DataWare& dw): MyParent(dw) {}
    protected:

 virtual const Editor::ThemeData& GetTheme(std::string& thm_nm);
 virtual      RefPtr<Gdk::Pixbuf> CalcSource(Project::MediaItem mi, const Point& sz);
};

void SetCBDirty(CanvasBuf& cb);
void SetMenuDirty(Menu mn);
// отрисовать меню, если еще не
RefPtr<Gdk::Pixbuf> GetRenderedShot(Menu mn);

MenuPack& UpdateMenuPack(Menu mn);

bool IsMenuToBe4_3();
bool Is4_3(Menu mn);
void SaveMenu(Menu mn, Menu orig_mn);

} // namespace Project

#endif // __MGUI_RENDER_MENU_H__

