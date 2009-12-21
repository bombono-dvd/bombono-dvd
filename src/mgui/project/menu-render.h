//
// mgui/project/menu-render.h
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

#ifndef __MGUI_PROJECT_MENU_RENDER_H__
#define __MGUI_PROJECT_MENU_RENDER_H__

#include <mbase/project/menu.h>
#include <mgui/render/menu.h>

namespace Project
{

void OpenMenu(Menu mn);
void SaveMenu(Menu mn);
void ClearMenuSavedData(Menu mn);


void EraseLinkedMenus(MenuPack& mp);

void SetThumbControl(MenuPack& mp);

void RenderThumbnail(MenuPack& mn_pack);

struct SimpleInitTextVis: public GuiObjVisitor
{
    CanvasBuf& cBuf;

                  SimpleInitTextVis(CanvasBuf& c_buf)
                    : cBuf(c_buf) { }

   virtual  void  Visit(TextObj& t_obj);
};

class CommonRgnListCleaner
{
    public:
                    CommonRgnListCleaner(CanvasBuf& cnv_buf);
     virtual       ~CommonRgnListCleaner() {}
    protected:
                CanvasBuf& cnvBuf;
                
              void  Clean();
      virtual void  OnClean() {}
};

class RgnListCleaner: public CommonRgnListCleaner
{
    typedef CommonRgnListCleaner MyParent;
    public:
        RgnListCleaner(CanvasBuf& cnv_buf): MyParent(cnv_buf) {}
       ~RgnListCleaner() { Clean(); }
};

} // namespace Project

#endif // #ifndef __MGUI_PROJECT_MENU_RENDER_H__

