//
// mbase/composite/component.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008, 2010 Ilya Murav'jov
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

#include "comp_vis.h"

#include <mbase/project/media.h>

#include <vector>

class MILinkList;

class CommonMediaLink
{
    public:
                        CommonMediaLink(Comp::Object* own);
    virtual            ~CommonMediaLink() {}

    Project::MediaItem  Link() { return link; }
                  void  SetLink(Project::MediaItem mi);
                  void  ClearLink() { SetLink(Project::MediaItem()); }

                        // симуляция MediaItem
                        operator Project::MediaItem() { return link; }
       //CommonMediaLink& operator =(Project::MediaItem mi)
       //                 {
       //                     SetLink(mi);
       //                     return *this;
       //                 }

    virtual MILinkList& GetLinks() = 0; 

    protected:
        Project::MediaItem  link;
              Comp::Object* owner;

    private:
                        CommonMediaLink(); // не требуется; не нужны
                        CommonMediaLink(const CommonMediaLink&);
       CommonMediaLink& operator =(const CommonMediaLink&);


void  ResetLink(Project::MediaItem new_ref, Project::MediaItem old_ref);
};

class MediaLink: public CommonMediaLink
{
    typedef CommonMediaLink MyParent;
    public:

                        MediaLink(Comp::Object* own): MyParent(own) {}
                       ~MediaLink() { ClearLink(); }

    virtual MILinkList& GetLinks(); 
};

class PosterLink: public CommonMediaLink
{
    typedef CommonMediaLink MyParent;
    public:
                        PosterLink(Comp::FramedObj* own);
                       ~PosterLink() { ClearLink(); }

    virtual MILinkList& GetLinks(); 
};

namespace Composition {

class MediaObj: public Object
{
    public:
                      MediaObj(): mdItem(this), playAll(false) {}

                      // положение на холсте
          const Rect& Placement() const { return mdPlc; }
  virtual       void  SetPlacement(const Rect& rct) { mdPlc = rct; }

           MediaLink& MediaItem() { return mdItem; }
	        bool& PlayAll() { return playAll; }

    protected:

                Rect  mdPlc;
           MediaLink  mdItem;
	        bool  playAll; // альтернатива явному mdItem
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
        bool  hlBorder; // highlight border

                      FramedObj(const FrameTheme& ft, Rect plc);

          FrameTheme& Theme() { return thmName; }
         std::string& ThemeName() { return thmName.themeName; }
          PosterLink& PosterItem() { return pstrLink; }

                //void  Load(const char* thm_name, Rect& plc);

    protected:
               FrameTheme  thmName;
               PosterLink  pstrLink;
};


// простое наложение, например, для фона
class CanvasObj: public SO<CanvasObj, MediaObj>
{
    public:

                  CanvasObj() { }
                  CanvasObj(Rect plc) { mdPlc = plc; }

};

} // namespace Composition

#endif // #ifndef __MBASE_COMPOSITE_COMPONENT_H__


