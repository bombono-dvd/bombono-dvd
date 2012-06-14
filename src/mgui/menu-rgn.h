//
// mgui/menu-rgn.h
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

#ifndef __MGUI_MENU_RGN_H__
#define __MGUI_MENU_RGN_H__

#include <mbase/project/menu.h>

#include "mcommon_vis.h"
#include "img_utils.h"
#include "rectlist.h"

// то, на чем рисуем
// :TODO: несмотря на описание, класс используется не полностью:
// - FramePlacement(), как и другие атрибуты, просто собран в структуру,
//   а не часть интерфейса (рисуют же всегда без смещения), используется
//   только редактором для своих внутренних нужд
// - то же самое с framTrans - используется только растяжение (без смещения)
// - Canvas() и FramePixbuf() дублируют друг друга; первое заведено для
//   быстрого доступа (без создания "под"-изображений), но это логически
//   усложняет; потому: отказаться Canvas()->FramePixbuf(), вместо FramePlacement() -
//   точка смещения (метод A())
class CanvasBuf
{
    public:
                             CanvasBuf(): recurseCnt(0) {}
                    virtual ~CanvasBuf() {}

                             // подложка (на чем рисуем):
                             // - размеры >= FramePlacement().Size()
                             //   (т.е. может быть больше, чем отрисованное меню на нем)
                             // - точке (0, 0) соответ. FramePlacement().A()
 virtual RefPtr<Gdk::Pixbuf> Canvas() = 0;
                             // возвращает положение композиции
                             // (где отрисовывается меню)
         virtual       Rect  FramePlacement() = 0;
                             // еще не отрисованная область на холсте
        virtual RectListRgn& RenderList() = 0;
                             // счетчик рекурсивного вхождения
                        int& RecurseCount() { return recurseCnt; }
                std::string& DataTag() { return dataTag; }

   const Planed::Transition& Transition()  { return framTrans; }

                      Point  Size() { return FramePlacement().Size(); }
                             // = Canvas(), только ограничен размерами Size()
        RefPtr<Gdk::Pixbuf>  FramePixbuf();
                             // граница области рендеринга, FramePixbuf() (используются
                             // относительные координаты!)
                       Rect  FrameRect() 
                             {
                                 Rect rct = FramePlacement();
                                 return ShiftTo00(rct);
                             }
                
   protected:
             Planed::Transition  framTrans; // переход координат
                            int  recurseCnt;
                    std::string  dataTag;   // где для данной поверхности хранить данные на объектах
};


// группа объектов
class ListObj: public Comp::Object
{
    public:
        typedef Comp::MediaObj Object;
        typedef std::vector<Object*> ArrType;
        typedef ArrType::iterator Itr;

               ~ListObj() { Clear(); }

          void  Clear();
          void  Clear(Object* obj);

          void  Ins(Object& obj) { objArr.push_back(&obj); }
       ArrType& List() { return objArr; }

 virtual  void  Accept(Comp::ObjVisitor& vis);

    protected:

        ArrType objArr; // храним объекты тут
};

class MenuRegion: public ListObj
{
    typedef ListObj MyParent;
    public:
    Project::BackSettings  bgSet;
        
                           MenuRegion(): mInf(true), bgRef(this), cnvBuf(0) {}

            virtual  void  Accept(Comp::ObjVisitor& vis);
                           // посетить только n-ный объект
                     void  AcceptWithNthObj(GuiObjVisitor& gvis, int n);

                           // работа с координатами 
                CanvasBuf& GetCanvasBuf() { ASSERT(cnvBuf); return *cnvBuf; }
                     void  SetCanvasBuf(CanvasBuf* cnv_buf) { cnvBuf = cnv_buf; }

 const Planed::Transition& Transition()  { return GetCanvasBuf().Transition(); }
                     Rect  FramePlacement() { return GetCanvasBuf().FramePlacement(); }

               MenuParams& GetParams() { return mInf; }
                MediaLink& BgRef()  { return bgRef; }
              RGBA::Pixel& BgColor()  { return bgSet.sldClr; }
                     //void  SetByMovieInfo(MovieInfo& m_inf);

    protected:
      MenuParams  mInf;
       MediaLink  bgRef;
    
       CanvasBuf* cnvBuf;

                           // посетить n-ый объект
                     void  VisitNthObj(GuiObjVisitor& gvis, int n);
};

Project::MenuMD* GetOwnerMenu(Comp::Object* obj);
void SetOwnerMenu(Comp::Object* obj, Project::MenuMD* owner);

// посетить только объект, но не его детей (касается MenuRegion)
void AcceptOnlyObject(Comp::Object* obj, GuiObjVisitor& g_vis);

RectListRgn& GetRenderList(MenuRegion& m_rgn);
void AddRelPos(RectListRgn& rct_lst, Comp::MediaObj* obj, const Planed::Transition& trans);

#endif // __MGUI_MENU_RGN_H__

