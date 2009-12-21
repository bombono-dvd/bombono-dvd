//
// mgui/editor/visitor.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
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

#ifndef __MGUI_EDITOR_VISITORS_H__
#define __MGUI_EDITOR_VISITORS_H__

#include <mgui/render/rgba.h>

// общий класс для получения набора областей отрисовки
class RectListVis: public CommonGuiVis
{
    typedef CommonGuiVis MyParent;
    public:
                  RectListVis()
                    : rctDrw(rLst) { }

           RectListRgn& RectList() { return rLst; }

    protected:

             RectListRgn  rLst;
    RGBA::RectListDrawer  rctDrw;
};

// получить набор областей рамок для списка объектов
class FrameRectListVis: public RectListVis
{
    typedef RectListVis MyParent;
    public:
                  FrameRectListVis(const int_array& pos_arr)
                    : posArr(pos_arr) { }

   virtual  void  Visit(FrameThemeObj& fto);
   virtual  void  Visit(TextObj& t_obj);

   virtual  void  VisitMediaObj(Comp::MediaObj& m_obj);

    protected:

         const int_array& posArr;
};

// общий класс для отрисовки (выделенных) объектов
class CommonDrawVis: public RectListVis
{
    typedef RectListVis MyParent;
    public:
    typedef ptr::shared<MBind::Moving> Manager;

                  CommonDrawVis(const int_array& sel_arr): selArr(sel_arr) { }

    protected:

  const int_array& selArr;

            void  Draw(Manager ming);
            bool  IsObjSelected() { return IsInArray(objPos, selArr); }

Manager  MakeFTOMoving(FrameThemeObj& fto);
Manager  MakeTextMoving(TextObj& t_obj);
};

class SelRectVis: public CommonDrawVis
{
    typedef CommonDrawVis MyParent;
    public:
                  SelRectVis(const int_array& sel_arr): MyParent(sel_arr) {}

   virtual  void  Visit(FrameThemeObj& fto) { RedrawMO(MakeFTOMoving(fto)); }
   virtual  void  Visit(TextObj& t_obj)     { RedrawMO(MakeTextMoving(t_obj)); }

    protected:
            void  RedrawMO(Manager ming);
};

class ClearLinkVis: public SelRectVis
{
    typedef SelRectVis MyParent;
    public:
        Project::MediaItem newMI;

                  ClearLinkVis(const int_array& sel_arr, Project::MediaItem mi)
                    : MyParent(sel_arr), newMI(mi) {}

   virtual  void  Visit(FrameThemeObj& fto);
   virtual  void  Visit(TextObj& t_obj);
};

#endif // __MGUI_EDITOR_KIT_H__

