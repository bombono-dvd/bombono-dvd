//
// mgui/img_utils.cpp
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

#include <mgui/_pc_.h>

#include "img_utils.h"
#include "win_utils.h"

#include <mbase/resources.h>

RefPtr<Gdk::Pixbuf> GetAsPixbuf(Cairo::RefPtr<Cairo::ImageSurface> sur)
{
    ASSERT( sur->get_format() == Cairo::FORMAT_ARGB32 );
    return Gdk::Pixbuf::create_from_data((const guint8*)sur->get_data(), 
                                         Gdk::COLORSPACE_RGB,
                                         true, 8, 
                                         sur->get_width(), sur->get_height(), 
                                         sur->get_stride());
}

Cairo::RefPtr<Cairo::ImageSurface> GetAsImageSurface(RefPtr<Gdk::Pixbuf> pix)
{
    ASSERT(pix->get_has_alpha());
    int wdh = pix->get_width();
    int hgt = pix->get_height();

    return Cairo::ImageSurface::create( pix->get_pixels(), Cairo::FORMAT_ARGB32, wdh, hgt,
                                        pix->get_rowstride() );
}

RefPtr<Gdk::Pixbuf> CreatePixbuf(int wdh, int hgt, bool has_alpha)
{
    ASSERT( (wdh >= 0) && (hgt >= 0) );
    if( !(wdh && hgt) )
        return RefPtr<Gdk::Pixbuf>();
    return Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, has_alpha, 8, wdh, hgt);
}

// stride по ширине равен sz.x*[3|4], в зависимости от has_alpha
RefPtr<Gdk::Pixbuf> CreateFromData(const guint8* data, const Point& sz, bool has_alpha)
{
    int pix_sz = has_alpha ? 4 : 3 ;
    return Gdk::Pixbuf::create_from_data(data, Gdk::COLORSPACE_RGB, has_alpha, 8, sz.x, sz.y, sz.x*pix_sz);
}

static void FreePixbuf(RefPtr<Gdk::Pixbuf>* pix)
{
    delete pix;
}

// быстрая операция d = c*a/255
#define MULT_DIV255(d,c,a,t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

inline void ConvertPixbufToCairo32(guint8* pixels, int stride, int wdh, int hgt,
                                   int real_channels)
{
    guchar tmp;
    for( int j = hgt; j; j-- )
    {
        guchar *p = pixels;

        if( real_channels == 3 )
        {
            guchar *end = p + 4 * wdh;

            while( p < end )
            {
#ifdef HAS_LITTLE_ENDIAN
                tmp  = p[0];
                p[0] = p[2];
                p[2] = tmp;
#else
                p[3] = p[2];
                p[2] = p[1];
                p[1] = p[0];
#endif
                p += 4;
            }
        }
        else
        {
            guchar *end = p + 4 * wdh;
            guint t1,t2,t3;

            while( p < end )
            {
#ifdef HAS_LITTLE_ENDIAN
                tmp = p[0];
                MULT_DIV255(p[0], p[2], p[3], t1);
                MULT_DIV255(p[1], p[1], p[3], t2);
                MULT_DIV255(p[2], tmp,  p[3], t3);
                //p[3] = p[3];
#else
                tmp = p[3];
                MULT_DIV255(p[3], p[2], tmp, t3);
                MULT_DIV255(p[2], p[1], tmp, t2);
                MULT_DIV255(p[1], p[0], tmp, t1);
                p[0] = tmp;
#endif

                p += 4;
            }
        }
        pixels += stride;
    }
}


CR::RefPtr<CR::ImageSurface> ConvertToSurface(RefPtr<Gdk::Pixbuf> pix)
{
    // добавляем канал, если не 4
    int real_channels = pix->get_n_channels();
    RGBA::AddAlpha(pix);

    int wdh = pix->get_width();
    int hgt = pix->get_height();
    int stride = pix->get_rowstride();
    guint8* pixels = pix->get_pixels();

    Cairo::Format format = (real_channels == 3) ? Cairo::FORMAT_RGB24 : Cairo::FORMAT_ARGB32 ;

    CR::RefPtr<CR::ImageSurface> sur = CR::ImageSurface::create( pixels, format, wdh, hgt, stride );

    static cairo_user_data_key_t key;
    cairo_surface_set_user_data (sur->cobj(), &key, new RefPtr<Gdk::Pixbuf>(pix), 
                                 (cairo_destroy_func_t)FreePixbuf);

    ConvertPixbufToCairo32(pixels, stride, wdh, hgt, real_channels);
    return sur;
}

// COPY_N_PASTE_ETALON из convert_bgra_to_rgba(), проект MPX/AudioSource
// Возможная оптимизация - просчитать все деления заранее:
// 
//  guint8 table[65536];
//
//  for (unsigned int a = 0; a <= 0xff; a++)
//    for (unsigned int x = 0; x <= 0xff; x++)
//      table[(a << 8) + x] = convert_color_channel (x, a);
//
//And then modify the computation in convert_bgra_to_rgba() to:
//
//  guint8 const* row = table + (src_pixel[3] << 8);
//
//  dst_pixel[0] = row[src_pixel[2]];
//  dst_pixel[1] = row[src_pixel[1]];
//  dst_pixel[2] = row[src_pixel[0]];
//  dst_pixel[3] = src_pixel[3];
//

inline guint8
convert_color_channel (guint8 src,
                       guint8 alpha)
{
    return alpha ? ((guint (src) << 8) - src) / alpha : 0;
}

//void
//convert_bgra_to_rgba (guint8 const* src,
//                      guint8*       dst,
//                      int           width,
//                      int           height)
void ConvertCairoToPixbuf32(guint8* pixels, int stride, int wdh, int hgt)
{
    for( int y = 0; y<hgt; y++, pixels += stride )
    {
        guint8* p = pixels;
        for( int x = 0; x<wdh; x++, p += 4 )
        {
#ifdef HAS_LITTLE_ENDIAN
            guint8 tmp = p[0];
            p[0] = convert_color_channel(p[2], p[3]);
            p[1] = convert_color_channel(p[1], p[3]);
            p[2] = convert_color_channel(tmp,  p[3]);
#else
            guint8 tmp = p[0];
            p[0] = convert_color_channel(p[1], tmp);
            p[1] = convert_color_channel(p[2], tmp);
            p[2] = convert_color_channel(p[3], tmp);
            p[3] = tmp;
#endif
        }
    }
}

//
// COPY_N_PASTE_ETALON_END из convert_bgra_to_rgba(), проект MPX/AudioSource
//

void AlignCairoVsPixbuf(RefPtr<Gdk::Pixbuf> pix, const Rect& rgn)
{
    ASSERT( pix->get_has_alpha() );

    Rect obj( PixbufBounds(pix) );
    obj = Intersection(obj, rgn);
    if( !obj.IsNull() )
    {
        int strd = pix->get_rowstride();
        RGBA::Pixel* dat = (RGBA::Pixel*)(pix->get_pixels() + strd*obj.top + obj.lft*4);
        RGBA::Pixel* cur;

        RGBA::Pixel::ClrType clr;
        for( int y=obj.top; cur=dat, y<obj.btm; y++, dat=(RGBA::Pixel*)((char*)dat+strd) )
            for( int x=obj.lft; x<obj.rgt; x++, cur++ )
            {
                // у Cairo и Pixbuf r и b поменяны местами
                clr = cur->red;
                cur->red  = cur->blue;
                cur->blue = clr;
            }
    }
}

void ConvertCairoVsPixbuf(RefPtr<Gdk::Pixbuf> pix, const Rect& rgn, bool from_cairo)
{
    ASSERT( pix->get_has_alpha() );

    Rect obj( PixbufBounds(pix) );
    obj = Intersection(obj, rgn);
    if( !obj.IsNull() )
    {
        int wdh = obj.Width();
        int hgt = obj.Height();
        RefPtr<Gdk::Pixbuf> rgn_buf = Gdk::Pixbuf::create_subpixbuf(pix, obj.lft, obj.top, wdh, hgt);
        
        if( from_cairo )
            ConvertCairoToPixbuf32(rgn_buf->get_pixels(), rgn_buf->get_rowstride(), wdh, hgt);
        else
            ConvertPixbufToCairo32(rgn_buf->get_pixels(), rgn_buf->get_rowstride(), wdh, hgt, 4);
    }
}

uint8_t CairoGetAlpha(uint8_t* pxl)
{
#ifdef HAS_LITTLE_ENDIAN
    return pxl[3];
#else
    return pxl[0];
#endif
}

void RGBA::CopyAlphaComposite(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src, bool mult)
{
    int wdh = src->get_width();
    int hgt = src->get_height();

    ASSERT( wdh == dst->get_width()  );
    ASSERT( hgt == dst->get_height() );
    ASSERT( src->get_n_channels() == 4 );
    ASSERT( dst->get_n_channels() == 4 );

    RGBA::Pixel* src_pix = (RGBA::Pixel*)src->get_pixels();
    int src_stride = src->get_rowstride();
    RGBA::Pixel* dst_pix = (RGBA::Pixel*)dst->get_pixels();
    int dst_stride = dst->get_rowstride();

    unsigned int tmp;
    unsigned char* lsrc_pix = (unsigned char*)src_pix;
    unsigned char* ldst_pix = (unsigned char*)dst_pix;

//     for( int y=0; y<hgt; y++, lsrc_pix += src_stride, ldst_pix += dst_stride )
//     {
//         src_pix = (RGBA::Pixel*)lsrc_pix;
//         dst_pix = (RGBA::Pixel*)ldst_pix;
//         for( int x=0; x<wdh; x++, src_pix++, dst_pix++ )
//         {
//             if( mult )
//                 MULT_DIV255(dst_pix->alpha, dst_pix->alpha, src_pix->alpha, tmp);
//             else
//                 dst_pix->alpha = src_pix->alpha;
//         }
//     }

#define PICTURE_LOOP( ACTION )   \
    for( int y=0; y<hgt; y++, lsrc_pix += src_stride, ldst_pix += dst_stride ) {  \
        src_pix = (RGBA::Pixel*)lsrc_pix;                                         \
        dst_pix = (RGBA::Pixel*)ldst_pix;                                         \
        for( int x=0; x<wdh; x++, src_pix++, dst_pix++ ) {                        \
            ACTION                                                                \
        }                                                                         \
    }                                                                             \
    /**/

    if( mult )
        PICTURE_LOOP( MULT_DIV255(dst_pix->alpha, dst_pix->alpha, src_pix->alpha, tmp); )
    else
        PICTURE_LOOP( dst_pix->alpha = src_pix->alpha; );
#undef PICTURE_LOOP
}

static int Round(int num, int align)
{
    int rest = num % align;
    return rest ? num - rest + align : num ;
}

static void SetRoundPixSizes(int wdh, int hgt, int& round_wdh, int& round_hgt, 
                             int div_sz, int rest_sz)
{
    ASSERT( div_sz > rest_sz );
    // чтобы каждый раз не создавать/удалять изображение,
    // будем менять при разнице большей, чем round_num (100) пикселов
    round_wdh = Round(wdh, div_sz) + rest_sz;
    round_hgt = Round(hgt, div_sz) + rest_sz;
}

bool ImageCanvas::UpdatePixbuf(const Point& sz, bool force_init)
{
    bool res = false;

    int round_wdh, round_hgt;
    SetRoundPixSizes(sz.x, sz.y, round_wdh, round_hgt, divSz, restSz);
    if( !canvPix || 
        (canvPix->get_width()  != round_wdh) ||
        (canvPix->get_height() != round_hgt) )
    {
        ClearPixbuf();
        canvPix = CreatePixbuf(round_wdh, round_hgt, hasAlpha);
        res = true;
    }

    if( res || force_init )
    {
        InitPixbuf();
        res = true;
    }
//     if( (canvSz.x != wdh) || canvSz.y != hgt )
//     {
//         res = true;
//         canvSz.x = wdh;
//         canvSz.y = hgt;
//     }
    return res;
}

RefPtr<Gdk::Pixbuf> MakeSubPixbuf(RefPtr<Gdk::Pixbuf> canv_pix, const Rect& rct)
{
    if( !canv_pix || rct.IsNull() )
        return RefPtr<Gdk::Pixbuf>();
    return Gdk::Pixbuf::create_subpixbuf(canv_pix, rct.lft, rct.top, rct.Width(), rct.Height());
}

RefPtr<Gdk::Pixbuf> MakeSubPixbuf(RefPtr<Gdk::Pixbuf> canv_pix, const Point& sz)
{
    return MakeSubPixbuf(canv_pix, Rect0Sz(sz));
}

RefPtr<Gdk::Pixbuf> VideoArea::FramePixbuf()
{
    return MakeSubPixbuf(CanvasPixbuf(), framPlc.Size());
}

void VideoArea::DrawCanvas(Gtk::Widget& wdt, const Rect& expose_rct)
{
    Rect drw_rct = Intersection(framPlc, expose_rct);
    if( !drw_rct.IsNull() )
    {
        Rect src_rct = drw_rct - framPlc.A();

        RefPtr<Gdk::Window> p_win = wdt.get_window();
        p_win->draw_pixbuf(wdt.get_style()->get_black_gc(), CanvasPixbuf(),
                           src_rct.lft, src_rct.top,
                           drw_rct.lft, drw_rct.top, drw_rct.Width(), drw_rct.Height(), 
                           Gdk::RGB_DITHER_NORMAL, 0, 0);
    }
}

void VideoArea::OnConfigure(const Point& win_sz, bool force_init)
{
    Rect new_plc = CalcFramPlc(win_sz);
    Point sz = new_plc.Size();

    bool is_update = UpdatePixbuf(sz, force_init) || (framPlc.Size() != sz);

    framPlc = new_plc;
    DoOnConfigure(is_update);
}

Rect FitIntoRect(const Point& win_sz, const Point& dst_dar, const Point& src_dar)
{
    DPoint asp = FitInto1(dst_dar, src_dar);
    Point sz(asp.x*win_sz.x, asp.y*win_sz.y);

    Point a((win_sz.x - sz.x)/2, (win_sz.y - sz.y)/2);
    return RectASz(a, sz);
}

Rect FitIntoRect(const Point& win_sz, const Point& obj_sz)
{
    return FitIntoRect(win_sz, win_sz, obj_sz);
}

Rect VideoArea::CalcFramPlc(const Point& win_sz)//int wdh, int hgt)
{
    return FitIntoRect(win_sz, GetAspectRadio());
}

bool DisplayArea::on_expose_event(GdkEventExpose* event)
{
    GetVA().DrawCanvas(*this, MakeRect(event->area));
    return true;
}

bool DisplayArea::on_configure_event(GdkEventConfigure* event)
{
    GetVA().OnConfigure(Point(event->width, event->height));
    return MyParent::on_configure_event(event);
}

namespace RGBA
{

//Pixel::Pixel(const Gdk::Color& clr):
//    red(FromGdkComponent(clr.get_red())), green(FromGdkComponent(clr.get_green())),
//    blue(FromGdkComponent(clr.get_blue())), alpha(MaxClr)
//{}

Pixel ColorToPixel(const Gdk::Color& clr)
{
    return Pixel(FromGdkComponent(clr.get_red()), FromGdkComponent(clr.get_green()),
                 FromGdkComponent(clr.get_blue()), Pixel::MaxClr);
}

unsigned int ToUint(const Gdk::Color& clr)
{
    return ColorToPixel(clr).ToUint();
}

gushort ToGdkComponent(unsigned char n)
{
    return (n << 8) | n;
}

Pixel::ClrType FromGdkComponent(gushort c)
{
    return c >> 8;
}

Gdk::Color PixelToColor(const Pixel& pxl)
{
    Gdk::Color res_clr;
    res_clr.set_rgb(ToGdkComponent(pxl.red), ToGdkComponent(pxl.green), ToGdkComponent(pxl.blue));
    return res_clr;
}


void AddAlpha(RefPtr<Gdk::Pixbuf>& pix)
{
    if( pix->get_n_channels() == 3 )
        pix = pix->add_alpha(false, 0, 0, 0);
}

Pixel& GetPixel(RefPtr<Gdk::Pixbuf> pix, const Point& pnt)
{
    ASSERT( pix->get_n_channels() == 4 );
    Pixel& pxl = *(Pixel*)((char*)pix->get_pixels() + 
                           pnt.y * pix->get_rowstride() + pnt.x * 4);
    return pxl;
}

void PutPixel(RefPtr<Gdk::Pixbuf> pix, const Point& pnt, const Pixel& pxl)
{
    GetPixel(pix, pnt) = pxl;
}

} // namespace RGBA

RefPtr<Gdk::Pixbuf> DataDirImage(const char* fname)
{
    return Gdk::Pixbuf::create_from_file(DataDirPath(fname));
}

