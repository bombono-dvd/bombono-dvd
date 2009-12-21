//
// mgui/mcommon_vis.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007,2009 Ilya Murav'jov
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

#include "mcommon_vis.h"
#include "menu-rgn.h"
#include "text_obj.h"

namespace Planed
{


Transition::Transition(): rat(0,0)
{}

// plc - положение области редактора
// abs_sz - абсолютные размеры редактируемого
Transition::Transition(const Rect& plc, const Point& abs_sz): sht(plc.A()), rat(0,0)
{
    Point sz = plc.Size();

    ASSERT( abs_sz.x != 0 && abs_sz.y != 0 );
    rat.first  = (double)sz.x / abs_sz.x;
    rat.second = (double)sz.y / abs_sz.y;
}

bool Transition::IsNull() const
{
    return (rat.first == 0) || (rat.second == 0);
}

Point Transition::AbsToRel(const Point& pnt) const
{
    Point res_pnt;
    res_pnt.x = int(pnt.x*rat.first  + 0.5);
    res_pnt.y = int(pnt.y*rat.second + 0.5);

    return res_pnt;
}

Point Transition::RelToAbs(const Point& pnt) const
{
    Point res_pnt;
    res_pnt.x = int(pnt.x / rat.first);
    res_pnt.y = int(pnt.y / rat.second);

    return res_pnt;
}

Point Transition::DevToRel(const Point& pnt) const
{
    //Point res_pnt;
    //res_pnt.x = pnt.x - rgnPlc.lft;
    //res_pnt.y = pnt.y - rgnPlc.top;

    return pnt - sht;
}

Point Transition::RelToDev(const Point& pnt) const
{
    //Point res_pnt;
    //res_pnt.x = pnt.x + rgnPlc.lft;
    //res_pnt.y = pnt.y + rgnPlc.top;

    return pnt + sht;
}


Rect AbsToRel(const Transition& trans, const Rect& rct)
{
    return Rect(trans.AbsToRel(rct.A()), trans.AbsToRel(rct.B()));
}

Rect RelToAbs(const Transition& trans, const Rect& rct)
{
    return Rect(trans.RelToAbs(rct.A()), trans.RelToAbs(rct.B()));
}

Rect DevToRel(const Transition& trans, const Rect& rct)
{
    return Rect(trans.DevToRel(rct.A()), trans.DevToRel(rct.B()));
}

Rect RelToDev(const Transition& trans, const Rect& rct)
{
    return Rect(trans.RelToDev(rct.A()), trans.RelToDev(rct.B()));
}

} // namespace Planed

void CommonGuiVis::Visit(MenuRegion& menu_rgn)
{ 
    menuRgn = &menu_rgn;
    cnvBuf  = &FindCanvasBuf(menu_rgn);

    ASSERT( !cnvBuf->Transition().IsNull() );
    VisitImpl(menu_rgn);
}

CanvasBuf& CommonGuiVis::FindCanvasBuf(MenuRegion& menu_rgn)
{ 
    return menu_rgn.GetCanvasBuf(); 
}

Rect CommonGuiVis::CalcRelPlacement(const Rect& abs_plc)
{
    return Planed::AbsToRel(cnvBuf->Transition(), abs_plc);
}

void CommonSelVis::VisitImpl(MenuRegion& menu_rgn)
{
    lct = cnvBuf->Transition().DevToRel(lct);
    MyParent::VisitImpl(menu_rgn);
}

void CommonSelVis::VisitMediaObj(Comp::MediaObj& m_obj)
{
    MakeCalcs(CalcRelPlacement(m_obj.Placement()));
}

void CommonSelVis::Visit(FrameThemeObj& fto)
{
    VisitMediaObj(fto);
}

void CommonSelVis::Visit(TextObj& t_obj)
{
    VisitMediaObj(t_obj);
}

void SelVis::MakeCalcs(const Rect& rel_plc)
{
    if( rel_plc.Contains(lct) )
        selPos = objPos;
}

//
// MenuRegion
// 

void MenuRegion::VisitNthObj(GuiObjVisitor& gvis, int n)
{
    gvis.SetPos(n);
    objArr[n]->Accept(gvis);
}

void MenuRegion::Accept(Comp::ObjVisitor& vis)
{
   if( GuiObjVisitor* gvis = dynamic_cast<GuiObjVisitor*>(&vis) )
   {
       gvis->Visit(*this);

       for( int i=0, cnt=objArr.size(); i<cnt; i++ )
           VisitNthObj(*gvis, i);
   }
   else
       MyParent::Accept(vis);
}

void MenuRegion::AcceptWithNthObj(GuiObjVisitor& gvis, int n)
{
    gvis.Visit(*this);

    VisitNthObj(gvis, n);
}
