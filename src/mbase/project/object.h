//
// mbase/project/object.h
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

#ifndef __MBASE_PROJECT_OBJECT_H__
#define __MBASE_PROJECT_OBJECT_H__

#include <mlib/dataware.h>
#include <mlib/ptr.h>

namespace Project
{

class ObjVisitor;

class Object: public DataWare, public ptr::base
{
    public:

        virtual       ~Object() 
                       {
                           ASSERT( use_count() == 0 );
                       }
        virtual  void  Accept(ObjVisitor& vis) = 0;
};

// список предварительных описаний объектов
class StillImageMD;
class VideoMD;
class VideoChapterMD;
class MenuMD;
//class MenuItemMD;
class FrameItemMD;
class TextItemMD;
class ColorMD;

class ObjVisitor
{
    public:

   virtual              ~ObjVisitor() { }

          virtual  void  Visit(StillImageMD& /*obj*/) {}
          virtual  void  Visit(VideoMD& /*obj*/) {}
          virtual  void  Visit(VideoChapterMD& /*obj*/) {}
          virtual  void  Visit(MenuMD& /*obj*/) {}
          //virtual  void  Visit(MenuItemMD& /*obj*/) {}
          virtual  void  Visit(FrameItemMD& /*obj*/) {}
          virtual  void  Visit(TextItemMD& /*obj*/) {}
          virtual  void  Visit(ColorMD& /*obj*/) {}
};

} // namespace Project

#endif // #ifndef __MBASE_PROJECT_OBJECT_H__

