//
// mgui/editor/region.h
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

#ifndef __MGUI_EDITOR_REGION_H__
#define __MGUI_EDITOR_REGION_H__

#include <mbase/project/menu.h>

#include <mgui/menu-rgn.h>

#include "const.h"

namespace Editor
{

class Region: protected VideoArea, public CanvasBuf
{
    public:
                           Region(): curMRgn(0), renderLst(0) {}

            Project::Menu  CurMenu();
               MenuRegion& CurMenuRegion() 
                           {
                               ASSERT( curMRgn );
                               return *curMRgn;
                           }

 virtual RefPtr<Gdk::Pixbuf> Canvas() { return CanvasPixbuf(); }
 virtual               Rect  FramePlacement() { return framPlc; }
 virtual      RectListRgn& RenderList() 
                           { 
                               ASSERT(renderLst);
                               return *renderLst;
                           }

                           // выделение объектов
                     bool  IsSelObj(int pos);
                     void  SelObj(int pos);
                     void  UnSelObj(int pos);
                     void  ClearSel();
                int_array& SelArr() { return selArr; }

    protected:

                      MenuRegion* curMRgn;
                     RectListRgn* renderLst;

                       int_array  selArr;   // выделенные объекты

     virtual         void  ClearPixbuf();
     virtual         void  InitPixbuf();
     virtual        Point  GetAspectRadio();
};


} // namespace Editor

#endif // __MGUI_EDITOR_REGION_H__


