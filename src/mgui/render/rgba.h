//
// mgui/render/rgba.h
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

#ifndef __MGUI_RENDER_RGBA_H__
#define __MGUI_RENDER_RGBA_H__

#include <boost/function.hpp> // для RgnPixelDrawer

#include <mgui/rectlist.h>
#include <mgui/img_utils.h>

namespace RGBA
{

// Замечание:
// рисование происходит не включая последнюю точку, т.е. (to_x,to_y)
// не обрабатывается (исключение - FrameRectTo(), так как рисует замкнутую
// линию)
class Drawer
{
    public:
                   Drawer();
          virtual ~Drawer() {}
    
                   // установить цвет
             void  SetForegroundColor(const int clr) { SetForegroundColor(Pixel(clr)); }
             void  SetForegroundColor(const Pixel& clr);
    
                   // переместиться
             void  MoveTo(int to_x, int to_y);
                   // нарисовать линию до
             void  LineTo(int to_x, int to_y);
                   // закрасить прямоугольник до
             void  RectTo(int to_x, int to_y);
                   // очертить прямоугольник до
             void  FrameRectTo(int to_x, int to_y);

    protected:

                Pixel  fillClr; // основной цвет
              guint32  intClr;

                  int  curX;
                  int  curY;

    virtual  void  LineToImpl(int to_x, int to_y) = 0;
    virtual  void  RectToImpl(int to_x, int to_y) = 0;
    virtual  void  FrameRectToImpl(int to_x, int to_y);

             bool  CheckFromLessTo(int to_x, int to_y);

    friend class RangeNormalizer;
};

// рисователь 
class PixelDrawer: public Drawer
{
    public:
                      PixelDrawer(RefPtr<Gdk::Pixbuf> canv_pix);

  RefPtr<Gdk::Pixbuf> Canvas() { return canvPix; }
    protected:

           RefPtr<Gdk::Pixbuf> canvPix;  // на чем рисуем
                        Pixel* dat;
                          int  rowStrd;

 virtual  void  LineToImpl(int to_x, int to_y);
 virtual  void  RectToImpl(int to_x, int to_y);

           int  CalcAdds(int x, int y) { return y*rowStrd + x*4; }
         Pixel* DataAt(int add);
};

// вместо рисования получить список областей, где рисовали
class RectListDrawer: public Drawer
{
    public:
                   RectListDrawer(RectListRgn& rlst): rLst(rlst) {}

    protected:

                 RectListRgn& rLst;

    virtual  void  LineToImpl(int to_x, int to_y);
    virtual  void  RectToImpl(int to_x, int to_y);
};

// рисователь с ограничением на область
class RgnPixelDrawer: public PixelDrawer
{
    typedef PixelDrawer MyParent;
    public:

    typedef Rect RgnType;
    typedef boost::function<void(const RgnType&)> DrwFunctor;

                   RgnPixelDrawer(RefPtr<Gdk::Pixbuf> canv_pix, RectListRgn* r_lst = 0);

             void  DrawWithFunctor(const Rect& plc_rct, DrwFunctor& drw_fnr);

                   // отрисовать картинку с учетом ограничения по области rLst
    virtual  void  ScalePixbuf(RefPtr<Gdk::Pixbuf> pix, const RgnType& plc);
    virtual  void  CompositePixbuf(RefPtr<Gdk::Pixbuf> pix, const RgnType& plc);
                   // реализация ~ Gdk::Pixbuf::fill()
    virtual  void  Fill(const RgnType& plc);

    protected:
                
           const RectListRgn* rLst;

    virtual  void  LineToImpl(int to_x, int to_y);
    virtual  void  RectToImpl(int to_x, int to_y);

             void  LineToImplRect(const RgnType& cut_rct, bool is_horiz);
             void  RectToImplRect(const RgnType& cut_rct);
};

// plc - местоположение, куда отображаем картинку
// drw_rgn - та часть plc, которую хотим отобразить/отрисовать (не затрагивая остальное) 
void ScalePixbuf(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src,
            const Rect& plc, const Rect& drw_rgn);

void AlphaCompositePixbuf(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src,
            const Rect& plc, const Rect& drw_rgn);

void CopyArea(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src, 
              const Rect& plc, const Rect& drw_rgn);

} // namespace RGBA

// нарисовать рамку (для объекта с координатами plc_rct)
void DrawGrabFrame(RGBA::Drawer& drw, const Rect& plc_rct);

#endif // __MGUI_RENDER_RGBA_H__

