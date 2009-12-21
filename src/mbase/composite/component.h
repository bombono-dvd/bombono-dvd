//
// mbase/composite/component.h
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

#ifndef __MBASE_COMPOSITE_COMPONENT_H__
#define __MBASE_COMPOSITE_COMPONENT_H__

#include <vector>
#include <mbase/project/media.h>

#include "comp_vis.h"

class MediaLink
{
    public:
                        MediaLink(Comp::Object* own);
                       ~MediaLink();
    
    Project::MediaItem  Link() { return link; }
                  void  SetLink(Project::MediaItem mi);
                  void  ClearLink() { SetLink(Project::MediaItem()); }

                        // симуляция MediaItem
                        operator Project::MediaItem() { return link; }
             MediaLink& operator =(Project::MediaItem mi) 
                        { 
                            SetLink(mi);
                            return *this; 
                        }

    protected:

        Project::MediaItem  link;
              Comp::Object* owner;
};

namespace Composition {

class MediaObj: public Object
{
    public:
                      MediaObj(): mdItem(this) {}

                      // положение на холсте
          const Rect& Placement() const { return mdPlc; }
  virtual       void  SetPlacement(const Rect& rct) { mdPlc = rct; }

           MediaLink& MediaItem() { return mdItem; }

    protected:

                Rect  mdPlc;
           MediaLink  mdItem;
};

// // шаблон для объектов, порожденных от MediaStrategy
// template<class MediaStratObj, class ParentObj>
// class MSO: public ParentObj
// {
//     public:
//   virtual  void  Accept(ObjVisitor& vis)
//                  {
//                      MediaStratObj& this_obj = static_cast<MediaStratObj&>(*this);
//                      // сначала само медиа, затем его хозяина
//                      this_obj.GetMedia()->Accept(vis);
//                      vis.Visit(this_obj);
//                  }
// };

// объект в рамке
class FramedObj: public SO<FramedObj, MediaObj>
{
    public:

                      FramedObj(const char* thm_name, Rect plc);

         std::string& Theme() { return thmName; }

                //void  Load(const char* thm_name, Rect& plc);

                protected:
                          std::string  thmName;
};


// простое наложение, например, для фона
class CanvasObj: public SO<CanvasObj, MediaObj>
{
    public:

                  CanvasObj() { }
                  CanvasObj(Rect plc) { mdPlc = plc; }

};

// группа объектов
class ListObj: public Object
{
    public:
        typedef std::vector<Object*>  ArrType;
        typedef    ArrType::iterator  Itr;

               ~ListObj() { Clear(); }

          void  Clear();
          void  Clear(Object* obj);

          void  Ins(Object& obj) { objArr.push_back(&obj); }
       ArrType& List() { return objArr; }

 virtual  void  Accept(ObjVisitor& vis);

    protected:

        ArrType objArr; // храним объекты тут
};

} // namespace Composition

#endif // #ifndef __MBASE_COMPOSITE_COMPONENT_H__


