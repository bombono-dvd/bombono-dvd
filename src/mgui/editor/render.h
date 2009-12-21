//
// mgui/editor/render.h
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

#ifndef __MGUI_EDITOR_RENDER_H__
#define __MGUI_EDITOR_RENDER_H__

#include <mgui/render/editor.h>

#include "const.h"

bool IsInArray(int pos, const int_array& arr);

// отрисовка в пределах области
class RenderVis: public CommonRenderVis
{
    typedef CommonRenderVis MyParent;
    public:
                  RenderVis(const int_array& sel_arr, RectListRgn& r_lst)
                    : MyParent(r_lst), selArr(sel_arr) { }

   //virtual  void  VisitImpl(MenuRegion& menu_rgn);
   virtual RefPtr<Gdk::Pixbuf> CalcBgShot();
   virtual  void  Visit(FrameThemeObj& fto);

    protected:

                     const int_array& selArr;

   virtual  bool  IsSelected() { return IsInArray(objPos, selArr); }

};

void RenderEditor(MEditorArea& edt_area, RectListRgn& rct_lst);

void ResetBackgroundImage(MenuRegion& mr);

#endif // __MGUI_EDITOR_RENDER_H__
