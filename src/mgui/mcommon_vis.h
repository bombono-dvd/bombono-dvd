//
// mgui/mcommon_vis.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2009 Ilya Murav'jov
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

#ifndef __MGUI_MCOMMON_VIS_H__
#define __MGUI_MCOMMON_VIS_H__

#include <mbase/composite/component.h>

#include "mgui_vis.h"

namespace Planed {

typedef std::pair<double, double> Ratio;

// Преобразование вида 
// x' = a_x * x + b_x
// y' = a_y * y + b_y
// 
// Замечание: преобразование, естественно, с округлением (int),
// поэтому надо аккуратно устанавливать его параметры ratio и sht
class Transition
{
    public:
        typedef Planed::Ratio Ratio;

                    Transition();
                    // plc - положение области редактора
                    // abs_sz - абсолютные размеры редактируемого
                    Transition(const Rect& plc, const Point& abs_sz);

               bool IsNull() const;
       const Ratio& GetRatio() const { return rat; }
              void  SetRatio(const Ratio& new_rat) { rat = new_rat; }
       const Point& GetShift() const { return sht; }
              void  SetShift(const Point& new_sht) { sht = new_sht; }

                    /* Преобразования */
                    // из абсолютных в относительные
             Point  AbsToRel(const Point& pnt) const;
             Point  RelToAbs(const Point& pnt) const;
                    // из относительных в координаты виджета 
             Point  RelToDev(const Point& pnt) const;
             Point  DevToRel(const Point& pnt) const;

    protected:

        Ratio  rat;
        Point  sht;
};

Rect AbsToRel(const Transition& trans, const Rect& rct);
Rect RelToAbs(const Transition& trans, const Rect& rct);

Rect RelToDev(const Transition& trans, const Rect& rct);
Rect DevToRel(const Transition& trans, const Rect& rct);

} // namespace Planed

class CanvasBuf;

class CommonGuiVis: public GuiObjVisitor
{
    public:
    using GuiObjVisitor::Visit;
                    
                    CommonGuiVis(): menuRgn(0), objPos(-1), cnvBuf(0) {}

                    // так как в этот момент инициализируются такие
                    // параметры, как menuRgn и trans, то наследникам
                    // следует переопределять VisitImpl(menu_rgn);
     virtual  void  Visit(MenuRegion& menu_rgn);
     virtual  void  VisitImpl(MenuRegion&) { }
     virtual  void  SetPos(int pos) { objPos = pos; }

    protected:

                 int  objPos;
          MenuRegion* menuRgn; // инициализируются в Visit(MenuRegion& menu_rgn);
           CanvasBuf* cnvBuf;  // вся информация о том, куда и как рендерить

                    // вычислить относительные координаты
              Rect  CalcRelPlacement(const Rect& abs_plc);

 virtual CanvasBuf& FindCanvasBuf(MenuRegion& menu_rgn);
};

class CommonSelVis: public CommonGuiVis
{
    typedef CommonGuiVis MyParent;
    public:

        Point  lct;
          int  selPos; // позиция объекта под курсором 
                       // (самого верхнего, неважно, выделенного или нет)

                  CommonSelVis(int x, int y) : 
                      lct(x, y), selPos(-1) { }

                  // просчитать отношение к данному объекту
   virtual  void  MakeCalcs(const Rect& rel_plc) = 0;

   virtual  void  VisitImpl(MenuRegion& menu_rgn);
   virtual  void  Visit(FrameThemeObj& fto);
   virtual  void  Visit(TextObj& t_obj);

            void  VisitMediaObj(Comp::MediaObj& m_obj);
};

// определение объекта по координатам
class SelVis: public CommonSelVis
{
    typedef CommonSelVis MyParent;
    public:

                  SelVis(int x, int y) : MyParent(x, y) { }

   virtual  void  MakeCalcs(const Rect& rel_plc);

};

#endif // __MGUI_MCOMMON_VIS_H__


