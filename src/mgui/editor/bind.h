//
// mgui/editor/bind.h
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

#ifndef __MGUI_EDITOR_BIND_H__
#define __MGUI_EDITOR_BIND_H__

#include <mgui/rectlist.h>
#include <mgui/text_obj.h>
#include <mgui/img_utils.h>
#include <mgui/mcommon_vis.h>

class EdtTextRenderer;

namespace RGBA {
class RgnPixelDrawer;
}

namespace Editor {
class FTOData;
}

RefPtr<Gdk::Pixbuf>& GetDwPixbuf(DataWare& dw);
void ClearDwPixbuf(DataWare& dw);

bool IsIconTheme(FrameThemeObj& fto);

RefPtr<Gdk::Pixbuf> GetThemeIcon(FrameThemeObj& fto, const Point& sz);
void DoRenderFTO(RGBA::RgnPixelDrawer* drw, FrameThemeObj& fto, 
                 Editor::FTOData& fd, const Planed::Transition& trans);
// 
// Интерфейсы, позволяющие научить редактор 
// оперировать(рисовать, перемещать, ...) новыми объектами
// Чтобы объект (наследник от MediaObj, имеет (Set)Placement(); ):
//  - научить отрисовываться:
//    - реализовать интерфейс MBind::Rendering вспомогательным классом,
//      который и будет отрисовывать
//    - в RenderVis создать соответ. Visit() ("привязаться")
//  - мог выделяться (select):
//    - "привязаться" к CommonSelVis, FrameRectListVis;
//  - научить перемещаться/скалироваться:
//    - реализовать интерфейс MBind::Moving вспомогательным классом;
//    - (только для перемещения) "привязаться" к RepositionVis
//    - (только для скалирования) "привязаться" к SetVectorVis и ResizeVis
// 
// См. примеры : FrameThemeObj, TextObj
namespace MBind
{

class Action
{
    public:
       virtual     ~Action() { }
};

class Rendering: public Action
{
    public:

    virtual   void  Render() = 0;
};

class Moving: public Action
{
    public:
                    Moving(RectListRgn& r_lst): rLst(r_lst) {}
                    // добавить в область отрисовки объект
    virtual   void  Redraw() = 0;
                    // перерассчитать объект (при скалировании)
    virtual   void  Update() = 0;
                    // положение объекта
    virtual   Rect  CalcRelPlacement() = 0;
    protected:
        RectListRgn& rLst;
};

//
// FTO
//

struct FTOData
{
        FrameThemeObj& fto;
   Planed::Transition  trans;

        FTOData(FrameThemeObj& f, const Planed::Transition& tr)
            : fto(f), trans(tr) {}

  Rect  CalcPlc() { return RelPos(fto, trans); }
};

class FTORendering: public Rendering, protected FTOData
{
    public:
                    FTORendering(FrameThemeObj& f, const Planed::Transition& tr, 
                                 RGBA::RgnPixelDrawer* d)
                        : FTOData(f, tr), drw(d) {}
    virtual   void  Render();
    protected:
        RGBA::RgnPixelDrawer* drw;
};

class FTOMoving: public Moving, protected FTOData
{
    public:
                    FTOMoving(RectListRgn& r_lst, FrameThemeObj& f, 
                              const Planed::Transition& tr)
                        : Moving(r_lst), FTOData(f, tr) {}
    virtual   void  Redraw();
    virtual   void  Update() { }
    virtual   Rect  CalcRelPlacement() { return CalcPlc(); }
};

//
// Текст
//

class TextRgnAcc
{
    public:
                TextRgnAcc(EdtTextRenderer& t_rndr, RectListRgn& lst);
               ~TextRgnAcc();
    protected:

        EdtTextRenderer& etr;
};

class TextRendering: public Rendering
{
    public:
                    TextRendering(RectListRgn& r_lst, EdtTextRenderer& t_rndr)
                        : rLst(r_lst), tRndr(t_rndr)  {}
    virtual   void  Render();
    protected:
    EdtTextRenderer& tRndr;
        RectListRgn& rLst;
};

class TextMoving: public Moving
{
    public:
                    TextMoving(RectListRgn& r_lst, EdtTextRenderer& t_rndr);
    virtual   void  Redraw();
    virtual   void  Update();
    virtual   Rect  CalcRelPlacement();
    protected:
        EdtTextRenderer& tRndr;
             TextRgnAcc  rgnAcc;
};

} // namespace MBind

#endif // __MGUI_EDITOR_BIND_H__

