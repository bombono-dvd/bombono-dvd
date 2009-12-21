//
// mgui/project/thumbnail.h
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

#ifndef __MGUI_PROJECT_THUMBNAIL_H__
#define __MGUI_PROJECT_THUMBNAIL_H__

#include <mbase/project/menu.h>
#include <mgui/design.h>
#include <mgui/img_utils.h>

// размеры миниатюры объектов в браузере (thumbnail) - ширина
const int SMALL_THUMB_WDH = 48;
const int THUMB_WDH       = 60;
const int BIG_THUMB_WDH   = 90;

namespace Project
{

//
// PixbufSource - изображение + признак "можно ли изменять"
// 

class PixbufSource
{
    public:
                        PixbufSource(): readOnly(false) {}
                        PixbufSource(RefPtr<Gdk::Pixbuf> p, bool ro): pix(p), readOnly(ro) {}

                        // получить изображение только чтения
    RefPtr<Gdk::Pixbuf> ROPixbuf() const { return pix; }
                        // получить изображение в полное владение
    RefPtr<Gdk::Pixbuf> RWPixbuf() const 
                        {
                            return (readOnly && pix) ? pix->copy() : pix ;
                        }

    private:
        RefPtr<Gdk::Pixbuf> pix;
                      bool  readOnly;
};

// сервисный класс - получение данных с мин. издержками
class PixExtractor
{
    public:
               virtual     ~PixExtractor() {}

                            // получить изображение с размерами sz
    virtual   PixbufSource  Make(const Point& sz) = 0;
                            // заполнить изображением объект pix
    virtual           void  Fill(RefPtr<Gdk::Pixbuf>& pix) = 0;
};

typedef ptr::shared<PixExtractor> PixbufExtractor;

// чтение изображения с диска
class ImgFilePE: public PixExtractor
{
    public:
                          ImgFilePE(std::string img_fname): imgFName(img_fname) {}

    virtual PixbufSource  Make(const Point& sz);
    virtual         void  Fill(RefPtr<Gdk::Pixbuf>& pix);

    protected:
        std::string imgFName;

        RefPtr<Gdk::Pixbuf> MakeImage(const Point& sz);
};

// адаптация изображения
class ImagePE: public PixExtractor
{
    public:
                          ImagePE(RefPtr<Gdk::Pixbuf> orig_pix, bool ro)
                            : origPix(orig_pix), readOnly(ro) 
                          { ASSERT(orig_pix); }

    virtual PixbufSource  Make(const Point& sz);
    virtual         void  Fill(RefPtr<Gdk::Pixbuf>& pix);

    protected:
        RefPtr<Gdk::Pixbuf> origPix;
                      bool  readOnly;
};

// вернуть копию изображения с заданными размерами
inline RefPtr<Gdk::Pixbuf> MakeCopyWithSz(RefPtr<Gdk::Pixbuf> pix, const Point& sz)
{
    return ImagePE(pix, true).Make(sz).RWPixbuf();
}

// однопиксельное изображение с цветом clr
class Color11PE: public PixExtractor
{
    public:
                          Color11PE(int clr_): clr(clr_) {}
                            
    virtual PixbufSource  Make(const Point& sz);
    virtual         void  Fill(RefPtr<Gdk::Pixbuf>& pix);

    protected:
             int clr;
};

// пустое (черное) изображение
class BlackPE: public Color11PE
{
    public:
        BlackPE(): Color11PE(BLACK_CLR) {}
};

//
// PrimaryShotGetter
//
// создать по первичному Media (не меню) его миниатюру
// если IsNullSize(thumb_sz), то вернет изображение оригинального размера
class PrimaryShotGetter: public ObjVisitor
{
    public:

 virtual  void  Visit(StillImageMD& obj);
 virtual  void  Visit(VideoMD& obj);
 virtual  void  Visit(VideoChapterMD& obj);
 virtual  void  Visit(MenuMD&) { ASSERT(0); } // использовать GetRenderedShot() вместо
 virtual  void  Visit(ColorMD& obj);

    static    PixbufExtractor  Make(MediaItem mi);
    static RefPtr<Gdk::Pixbuf> Make(MediaItem mi, const Point& thumb_sz)
    {
        return Make(mi)->Make(thumb_sz).RWPixbuf();
    }

    protected:

    PixbufExtractor  pixExt;
};

// рассчитать размеры прямоугольника по зад. площади, сохраняя пропорции
Point CalcProportionSize(const Point& sz, double etalon_square);
// различные функции с использованием соотношения 4:3
inline Point CalcProportionSize4_3(const Point& sz, int wdh)
{
    return CalcProportionSize(sz, wdh*wdh*0.75);
}
inline Point Calc4_3Size(int wdh)
{
    return Point(wdh, int(wdh*0.75));
}
Point CalcThumbSize(MediaItem mi);
Point CalcAspectSize(VideoMD& vi);

// кэшированное изображение медиа
RefPtr<Gdk::Pixbuf> GetCacheShot(MediaItem mi);

// рассчитать кэш-снимок медиа
RefPtr<Gdk::Pixbuf> GetCalcedShot(MediaItem mi);
// тоже самое, но для первичных медиа только
RefPtr<Gdk::Pixbuf> GetPrimaryShot(MediaItem mi);

void SetDirtyCacheShot(MediaItem mi);
// вернуть копию снимка с заданными размерами
inline RefPtr<Gdk::Pixbuf> GetCalcedShot(MediaItem mi, const Point& sz)
{
    return MakeCopyWithSz(GetCalcedShot(mi), sz);
}

void StampFPEmblem(MediaItem mi, RefPtr<Gdk::Pixbuf> pix);

} // namespace Project

#endif // #ifndef __MGUI_PROJECT_THUMBNAIL_H__

