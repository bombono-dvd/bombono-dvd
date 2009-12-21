//
// mvisitor.h
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

#ifndef __MVISITOR_H__
#define __MVISITOR_H__

#include <Magick++.h>

#include <mbase/composite/component.h>

#include "mconst.h"

namespace Composition {

class MovieVisitor: public Comp::ObjVisitor
{
    public:

    virtual  void  Visit(FrameThemeObj& obj);
    virtual  void  Visit(SimpleOverObj& obj);
};

} // namespace Composition

struct FrameCounter
{
    int  framNum;
    int  curNum;

                    FrameCounter(): framNum(-1), curNum(0) { }
        
              bool  IsDone() const { return framNum != -1 && !(curNum < framNum); }
      FrameCounter& operator++() { curNum++; return *this; }
              void  Set(int num) { curNum = 0; framNum = num>=0 ? num : -1 ; }
};

// наложение кадров на изображение (ImgComposVis::canvImg)
class ImgComposVis: public Comp::MovieVisitor, public FrameCounter, public Iterator<Magick::Image>
{
    typedef Comp::MovieVisitor MyParent;
    public:
    using Iterator<Magick::Image>::operator++;

                 ImgComposVis(int wdh, int hgt);

  virtual  void  Visit(FrameThemeObj& obj);
  virtual  void  Visit(SimpleOverObj& obj);

  virtual  void  Visit(Comp::MovieMedia& obj);

  Magick::Image& CanvImg() const { return (Magick::Image&)canvImg; }
           void  First(Comp::ListObj& lst) { grp = &lst; First(); }

  virtual  void  First();
  virtual  void  Next();
  virtual  bool  IsDone() const;
  virtual  Magick::Image& CurrentItem() const { return CanvImg(); }

    protected:

          Magick::Image  canvImg;
                   bool  isEnd;

          Comp::ListObj* grp;
};

class DoBeginVis: public Comp::MovieVisitor
{
    typedef Comp::MovieVisitor MyParent;
    public:

   Comp::ListObj& lstObj;
Comp::MovieMedia* basMedia; // считаем как эталон
   SimpleOverObj* bckSoo;
    FrameCounter  framCnt;
             int  outFd;
        
                      DoBeginVis(Comp::ListObj& lst): basMedia(0), bckSoo(0), 
                          lstObj(lst), isGood(true), outFd(OUT_HNDL) { }

                      // подготовить и проверить все перед работой
                bool  Begin();
                
        virtual void  Visit(Comp::MovieMedia& mm);
        virtual void  Visit(FrameThemeObj& obj);

                bool  IsGood()  { return isGood; }
           MovieInfo& PatInfo() { return patInfo; }

    protected:

        MovieInfo  patInfo; // 
             bool  isGood;
};

#endif // #ifndef __MVISITOR_H__


