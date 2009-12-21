//
// mgui/render/editor.h
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

#ifndef __MGUI_RENDER_EDITOR_H__
#define __MGUI_RENDER_EDITOR_H__

#include <mgui/mcommon_vis.h>

#include "rgba.h"
#include "common.h"

// "качественный" отрисовщик
class HiQuData: public Editor::FTOData
{
    typedef Editor::FTOData MyParent;
    public:
                   HiQuData(DataWare& dw): MyParent(dw) {}
    protected:

 virtual const Editor::ThemeData& GetTheme(std::string& thm_nm);
// virtual      RefPtr<Gdk::Pixbuf> CalcSource(Project::MediaItem mi, const Point& sz);
};

// промежуточные данные для редактора
class FTOInterPixData: public HiQuData
{
    typedef HiQuData MyParent;
    public:
                   FTOInterPixData(DataWare& dw): MyParent(dw) {}
    protected:

 virtual      RefPtr<Gdk::Pixbuf> CalcSource(Project::MediaItem mi, const Point& sz);
};

namespace MBind
{
class Rendering;
class TextRendering;
}

class CommonRenderVis: public CommonGuiVis
{
    typedef CommonGuiVis MyParent;
    public:
                  CommonRenderVis(RectListRgn& r_lst)
                      : rLst(r_lst) { }

   virtual  void  VisitImpl(MenuRegion& menu_rgn);
   virtual  void  Visit(TextObj& t_obj);

   typedef ptr::one<RGBA::RgnPixelDrawer> Drawer;
          Drawer& GetDrawer() { return drw; }
            
    protected:
        ptr::one<RGBA::RgnPixelDrawer> drw; 
                          RectListRgn& rLst;

   virtual  bool  IsSelected() { return false; }
            void  RenderObj(Comp::MediaObj& m_obj, MBind::Rendering& ring);

                  // отрисовка фона
   virtual  void  RenderBackground();
   virtual RefPtr<Gdk::Pixbuf> CalcBgShot() { return RefPtr<Gdk::Pixbuf>(); }

MBind::TextRendering Make(TextObj& t_obj);
};

#endif // #ifndef __MGUI_RENDER_EDITOR_H__

