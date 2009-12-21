//
// mgui/mgui_vis.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008 Ilya Murav'jov
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

#ifndef __MGUI_MGUI_VIS_H__
#define __MGUI_MGUI_VIS_H__

#include <mbase/composite/comp_vis.h>

//
// Определение посетителя GUI-объектов
// Все также устроено, как в comp_vis.h
// 

// список предварительных описаний
class MenuRegion;
class TextObj;

class GuiObjVisitor: public Comp::ObjVisitor
{
    public:
    using Comp::ObjVisitor::Visit;

        //virtual GuiObjVisitor* GetGuiObjVisitor() { return this; }

                virtual  void  Visit(MenuRegion& /*gobj*/) { }
                virtual  void  Visit(TextObj& /*obj*/) { }

                               // установить порядковый номер очередного объекта (от MenuRegion)
                virtual  void  SetPos(int /*pos*/) { }
};


#endif // __MGUI_MGUI_VIS_H__

