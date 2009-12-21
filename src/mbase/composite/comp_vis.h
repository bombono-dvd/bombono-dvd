//
// mbase/composite/comp_vis.h
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

#ifndef __MBASE_COMPOSITE_COMP_VIS_H__
#define __MBASE_COMPOSITE_COMP_VIS_H__

#include <mlib/patterns.h>
#include <mlib/dataware.h>

namespace Composition {

// 
// Базовое определение компоновщика и посетителя
// 
// При добавлении:
//  - посетителя:
//    1) держим описание посетителя в mvisitor.h
//  - компоновочного объекта:
//    1) добавить сюда в список предварительных описаний
//    2) добавить метод Visit() к ObjVisitor
//    3) само описание класса держим где-нибудь (обычно в mcomponent.h)

class ObjVisitor;

class Object: public DataWare
{
    public:

     virtual       ~Object() { }
                    // метод посещения по образцу "Посетитель"
     virtual  void  Accept(ObjVisitor& vis) = 0;
};

// список предварительных описаний объектов
class FramedObj;
class CanvasObj;

class MovieMedia;

// образец "Посетитель"
class ObjVisitor
{
    public:

   virtual              ~ObjVisitor() { }

          virtual  void  Visit(Object& /*obj*/) {}
          virtual  void  Visit(FramedObj& /*obj*/) {}
          virtual  void  Visit(CanvasObj& /*obj*/) {}
        
          virtual  void  Visit(MovieMedia& /*obj*/) {}
};

template<class SimpleObj, class ParentObj = Object>
class SO: public SimpleVisitorObject<SimpleObj, ParentObj, ObjVisitor>
{};

} // namespace Composition

namespace Comp = Composition;
typedef Comp::FramedObj FrameThemeObj;
typedef Comp::CanvasObj SimpleOverObj;

#endif // #ifndef __MBASE_COMPOSITE_COMP_VIS_H__

