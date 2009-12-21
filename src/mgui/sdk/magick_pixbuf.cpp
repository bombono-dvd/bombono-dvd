//
// mgui/sdk/magick_pixbuf.cpp
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

#include <mgui/_pc_.h>

#include "magick_pixbuf.h"

struct ImgViewDestroy
{
    Magick::Image* img;
   Magick::Pixels* view;
             bool  delImg;

        ImgViewDestroy(Magick::Image* i, Magick::Pixels* v, bool del_img = true)
            : img(i), view(v), delImg(del_img) {}
       ~ImgViewDestroy() {}

  void  Do(const guint8*)
        {
            delete view;
            if( delImg )
                delete img;
            delete this;
        }
};

static void AlignAsPixbuf( MagickLib::PixelPacket* p, Magick::Image& img )
{
    int wdh = img.columns();
    int hgt = img.rows();
    bool is_alpha_on = img.matte();

    MagickLib::PixelPacket* p2 = p;
    char tmp;
    for( int y=0; y<hgt; y++ )
        for( int x=0; x<wdh; x++, p2++ )
        {
            // :TODO: решить как быть с разными форматами
            // b <-> r
            tmp = p2->blue;
            p2->blue = p2->red;
            p2->red = tmp;

    	    // у GM и Gdk "противоположные" понятия о прозрачности
    	    // opacity - непрозрачность, затененность (0 - непрозрачный, 255 - полностью прозрачный)
    	    // alpha - прозрачность (0 - полностью прозрачный, 255 - непрозрачный)
    	    // alpha = 255 - opacity
            p2->opacity = is_alpha_on ? 255 - p2->opacity : 255 ;
        }
}

RefPtr<Gdk::Pixbuf> CreatePixbufFromImage(Magick::Image* img, bool del_img, 
                                          bool align_to_pixbuf)
{
    int wdh = img->columns();
    int hgt = img->rows();

    Magick::Pixels* view = new Magick::Pixels(*img);
    MagickLib::PixelPacket* p = view->get(0, 0, wdh, hgt);

    if( align_to_pixbuf )
        AlignAsPixbuf(p, *img);

    ImgViewDestroy* ivd = new ImgViewDestroy(img, view, del_img);
    Gdk::Pixbuf::SlotDestroyData d_slot = sigc::mem_fun(ivd, &ImgViewDestroy::Do);

    return Gdk::Pixbuf::create_from_data((const guint8*)p, Gdk::COLORSPACE_RGB, 
                                         true, 8, wdh, hgt, wdh*4, d_slot);
}

void AlignToPixbuf(Magick::Image& img)
{
    int wdh = img.columns();
    int hgt = img.rows();

    Magick::Pixels view(img);
    MagickLib::PixelPacket* p = view.get(0, 0, wdh, hgt);

    AlignAsPixbuf(p, img);
    view.sync();
}

RefPtr<Gdk::Pixbuf> GetAsPixbuf(Magick::Image& img)
{
    return CreatePixbufFromImage(&img, false, false);
}


