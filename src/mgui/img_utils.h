//
// mgui/img_utils.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2010 Ilya Murav'jov
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

#ifndef __MGUI_IMG_UTILS_H__
#define __MGUI_IMG_UTILS_H__

#include "mguiconst.h"

#include <mbase/pixel.h>
#include <mlib/geom2d.h>

// Вспомогательные функции для работы с Pixbuf и Cairo

// работает в обе стороны, Cairo <-> Pixbuf
// Замечания (см. gdk_cairo_set_source_pixbuf()):
// 1) для big endian работать не будет!
// 2) более того, простой обмен r<->b работает только тогда, когда альфа-канал
//  в Pixbuf полностью непрозрачный (однако, рисовать полупрозрачным с помощью
//  Cairo на не прозрачной подложке можно и таким образом).
void AlignCairoVsPixbuf(RefPtr<Gdk::Pixbuf> pix, const Rect& rgn);
// тоже, что и AlignCairoVsPixbuf(), только без ограничений (соответ., медленнее +
// при множественном использовании проявится погрешность)
void ConvertCairoVsPixbuf(RefPtr<Gdk::Pixbuf> pix, const Rect& rgn, bool from_cairo);

// привести к Pixbuf, не изменяя содержимого
RefPtr<Gdk::Pixbuf> GetAsPixbuf(Cairo::RefPtr<Cairo::ImageSurface> sur);
// наоборот
Cairo::RefPtr<Cairo::ImageSurface> GetAsImageSurface(RefPtr<Gdk::Pixbuf> pix);

//
// COPY_N_PASTE_ETALON (from Gtk; gdk_cairo_set_source_pixbuf(), gtkcairo.c)
// 
// Преобразование Pixbuf->Cairo::ImageSurface
// Отличия от пары AlignCairoVsPixbuf()+GetAsImageSurface():
//  - всегда правильно преобразует пикселы (и при прозрачности тоже);
//  - результат от времени жизни pix не зависит
// 
CR::RefPtr<CR::ImageSurface> ConvertToSurface(RefPtr<Gdk::Pixbuf> pix);
uint8_t CairoGetAlpha(uint8_t* pxl);

RefPtr<Gdk::Pixbuf> CreatePixbuf(int wdh, int hgt, bool has_alpha = true);
inline RefPtr<Gdk::Pixbuf> CreatePixbuf(const Point& sz, bool has_alpha = true)
{
    return CreatePixbuf(sz.x, sz.y, has_alpha);
}

// stride по ширине равен sz.x*[3|4], в зависимости от has_alpha
RefPtr<Gdk::Pixbuf> CreateFromData(const guint8* data, const Point& sz, bool has_alpha);

inline Rect MakeRect(GdkRectangle& g_rct)
{
    return Rect(g_rct.x, g_rct.y, g_rct.x + g_rct.width, g_rct.y + g_rct.height);
}

inline Point PixbufSize(RefPtr<Gdk::Pixbuf> pix)
{
    return Point(pix->get_width(), pix->get_height());
}

inline Rect PixbufBounds(RefPtr<Gdk::Pixbuf> pix)
{
    return Rect(0, 0, pix->get_width(), pix->get_height());
}

inline Point WidgetSize(Gtk::Widget& wdg)
{
    return Point(wdg.get_width(), wdg.get_height());
}

RefPtr<Gdk::Pixbuf> MakeSubPixbuf(RefPtr<Gdk::Pixbuf> canv_pix, const Rect& rct);
RefPtr<Gdk::Pixbuf> MakeSubPixbuf(RefPtr<Gdk::Pixbuf> canv_pix, const Point& sz);

class ImageCanvas
{
    static const int DefDivSize  = 100;
    static const int DefRestSize = 0;
    public:
                            ImageCanvas(bool has_alpha)
                                : divSz(DefDivSize), restSz(DefRestSize), hasAlpha(has_alpha) { }
                            ImageCanvas(int div_sz = DefDivSize, int rest_sz = DefRestSize)
                                : divSz(div_sz), restSz(rest_sz), hasAlpha(true) { }
                   virtual ~ImageCanvas() { ClearPixbuf(); }

                            // "подбирает" размеры холста под требуемые
                            // если да, то CanvasPixbuf() был заменен
                      bool  UpdatePixbuf(const Point& sz, bool force_init);

        RefPtr<Gdk::Pixbuf> CanvasPixbuf() { return canvPix; }

    protected:

            RefPtr<Gdk::Pixbuf> canvPix;

                           int  divSz;  // гранулярность размеров,
                           int  restSz; // divSz > restSz
                          bool  hasAlpha;

      virtual         void  ClearPixbuf() { }
      virtual         void  InitPixbuf() { }
};

class VideoArea: protected ImageCanvas
{
    typedef ImageCanvas MyParent;
    public:
                         VideoArea(bool has_alpha = true): MyParent(has_alpha) {}
                    
                         // область framPlc от полотна
     RefPtr<Gdk::Pixbuf> FramePixbuf();

    protected:

        Rect  framPlc;  // положение области в окне

               void  DrawCanvas(Gtk::Widget& wdt, const Rect& expose_rct);
                     // обновиться при смене размеров окна
               void  OnConfigure(const Point& win_sz, bool force_init = false);
    virtual    void  DoOnConfigure(bool is_update) = 0;

               Rect  CalcFramPlc(const Point& win_sz);
                     // соотношение сторон области
    virtual   Point  GetAspectRadio() = 0;

    friend class DisplayArea;
};

// удобный класс для пользователей VideoArea вместе в Gtk::DA
class DisplayArea: public Gtk::DrawingArea
{
    typedef Gtk::DrawingArea MyParent;
    public:

    virtual VideoArea& GetVA() = 0;

                 void  Rebuild() 
                       {
                           GetVA().OnConfigure(WidgetSize(*this), true);
                           queue_draw();
                       }

         virtual bool  on_expose_event(GdkEventExpose* event);
         virtual bool  on_configure_event(GdkEventConfigure* event);
};

// уместить один прямоугольник в другой с сохранением пропорций
Rect FitIntoRect(const Point& win_sz, const Point& obj_sz);

namespace RGBA
{

Pixel ColorToPixel(const Gdk::Color& clr);
unsigned int ToUint(const Gdk::Color& clr);

// перевод в GdkColor
Gdk::Color PixelToColor(const Pixel& pxl);
// --||-- одной компоненты
gushort ToGdkComponent(Pixel::ClrType n);
Pixel::ClrType FromGdkComponent(gushort c);

Pixel& GetPixel(RefPtr<Gdk::Pixbuf> pix, const Point& pnt);
void PutPixel(RefPtr<Gdk::Pixbuf> pix, const Point& pnt, const Pixel& pxl);

void Scale(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src, const Rect& plc);
void Scale(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src);

void CopyOrScale(RefPtr<Gdk::Pixbuf>& pix, RefPtr<Gdk::Pixbuf> src);

void AlphaComposite(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src, const Rect& plc);
void AlphaComposite(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src, const Point& a);

void CopyAlphaComposite(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src, bool mult = false);
// добавляет альфа-канал, если не 4
void AddAlpha(RefPtr<Gdk::Pixbuf>& pix);
} // namespace RGBA

#endif // __MGUI_IMG_UTILS_H__

