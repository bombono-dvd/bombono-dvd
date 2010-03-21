//
// mgui/render/rgba.cpp
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

#include <mgui/_pc_.h>

#include "rgba.h"

namespace RGBA
{

// plc - местоположение, куда отображаем картинку
// drw_rgn - та часть plc, которую хотим отобразить/отрисовать (не затрагивая остальное) 
void ScalePixbuf(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src,
            const Rect& plc, const Rect& drw_rgn)
{
    Point sz = plc.Size();
    Point drw_sz = drw_rgn.Size();

    src->scale(dst, drw_rgn.lft, drw_rgn.top, drw_sz.x, drw_sz.y, plc.lft, plc.top,
                 (double)sz.x/src->get_width(),
                 (double)sz.y/src->get_height(),
                 Gdk::INTERP_BILINEAR);
}

void AlphaCompositePixbuf(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src,
            const Rect& plc, const Rect& drw_rgn)
{
    Point sz = plc.Size();
    Point drw_sz = drw_rgn.Size();

    src->composite(dst, drw_rgn.lft, drw_rgn.top, drw_sz.x, drw_sz.y, plc.lft, plc.top,
                    (double)sz.x/src->get_width(),
                    (double)sz.y/src->get_height(),
                    Gdk::INTERP_BILINEAR, 255);
}

void Scale(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src, const Rect& plc)
{
    ScalePixbuf(dst, src, plc, plc);
}

void AlphaComposite(RefPtr<Gdk::Pixbuf> dst, RefPtr<Gdk::Pixbuf> src, const Rect& plc)
{
    AlphaCompositePixbuf(dst, src, plc, plc);
}

///////////////////////////////////////////////////////////
// RGBA::Drawer

Drawer::Drawer(): curX(0), curY(0)
{
    SetForegroundColor(0);
}

void Drawer::SetForegroundColor(const Pixel& clr)
{
    fillClr = clr;

    intClr = *(guint32*)&fillClr;
}

void Drawer::MoveTo(int to_x, int to_y)
{
    curX = to_x;
    curY = to_y;
}

static void NormalRange(int& beg, int& end)
{
    if( beg > end )
    {
        std::swap(beg, end);
        beg++;
        end++;
    }
}

// подготавливает данные для вызовов Drawer::*Impl()
struct RangeNormalizer
{
    Drawer& drw;
       int& toX;
       int& toY;

       int  endX;
       int  endY;

            RangeNormalizer(Drawer& d, int& to_x, int& to_y);
           ~RangeNormalizer();
};

RangeNormalizer::RangeNormalizer(Drawer& d, int& to_x, int& to_y)
    : drw(d), toX(to_x), toY(to_y)
{
    endX = toX;
    endY = toY;

    NormalRange(drw.curX, toX);
    NormalRange(drw.curY, toY);
}

RangeNormalizer::~RangeNormalizer()
{
    drw.MoveTo(endX, endY);
}

// нарисовать линию до
void Drawer::LineTo(int to_x, int to_y)
{
    ASSERT( (curX == to_x) || (curY == to_y) );

    RangeNormalizer rn(*this, to_x, to_y);

    LineToImpl(to_x, to_y);
}

// закрасить прямоугольник до
void Drawer::RectTo(int to_x, int to_y)
{
    RangeNormalizer rn(*this, to_x, to_y);

    RectToImpl(to_x, to_y);
}

// очертить прямоугольник до
void Drawer::FrameRectTo(int to_x, int to_y)
{
    FrameRectToImpl(to_x, to_y);
    MoveTo(to_x, to_y);
}

void Drawer::FrameRectToImpl(int to_x, int to_y)
{
    int old_x = curX;
    int old_y = curY;

    LineTo(to_x, old_y);
    LineTo(to_x, to_y);
    LineTo(old_x, to_y);
    LineTo(old_x, old_y);
}

bool Drawer::CheckFromLessTo(int to_x, int to_y)
{
    return (curX <= to_x) && (curY <= to_y);
}

PixelDrawer::PixelDrawer(RefPtr<Gdk::Pixbuf> canv_pix): canvPix(canv_pix), dat(0)
{
    ASSERT( canvPix->get_has_alpha() );

    dat = (Pixel*)canvPix->get_pixels();
    rowStrd = canvPix->get_rowstride();
}

Pixel* PixelDrawer::DataAt(int add)
{ 
    return (Pixel*)((char*)dat + add);
}

// :COMMENT: пока не требуется
// static void AlphaCompositePixel(Pixel& dst, Pixel& src)
// {
//     const int max_rgb = 255;
//     double a_src = src.alpha;
//     double a_dst = dst.alpha;
//
//     dst.red   = (unsigned char)((src.red*a_src + (max_rgb-a_src)*a_dst*dst.red/max_rgb)/max_rgb + 0.5);
//     dst.green = (unsigned char)((src.green*a_src + (max_rgb-a_src)*a_dst*dst.green/max_rgb)/max_rgb + 0.5);
//     dst.blue  = (unsigned char)((src.blue*a_src + (max_rgb-a_src)*a_dst*dst.blue/max_rgb)/max_rgb + 0.5);
//     dst.alpha = (unsigned char)((a_src + (max_rgb-a_src)*a_dst/max_rgb)/max_rgb + 0.5);
// }

void PixelDrawer::LineToImpl(int to_x, int to_y)
{
    ASSERT( CheckFromLessTo(to_x, to_y) );
    Pixel* beg = DataAt( CalcAdds(curX, curY) );
    if( curY == to_y )
    {
        int sz = to_x-curX;
        for( Pixel* end=beg+sz; beg<end; beg++ )
            // :TODO: можно сделать, когда со скоростью наложения будет все ясно
            //AlphaCompositePixel(*beg, fillClr);
            *(guint32*)beg = intClr;
    }
    else
    {
        int sz = to_y-curY;
        for( int i=0; i<sz; i++ )
        {
            // :TODO: --||--
            //AlphaCompositePixel(*beg, fillClr);
            *(guint32*)beg = intClr;
            beg = (Pixel*)((char*)beg + rowStrd);
        }
    }
}

void PixelDrawer::RectToImpl(int to_x, int to_y)
{
    ASSERT( CheckFromLessTo(to_x, to_y) );

    int beg_x = curX;
    int beg_y = curY;
    for( int cur_y=beg_y; cur_y<to_y; cur_y++ )
    {
        MoveTo(beg_x, cur_y);
        LineToImpl(to_x, cur_y);
    }
}

void RectListDrawer::LineToImpl(int to_x, int to_y)
{
    if( curY == to_y )
        rLst.push_back(Rect(curX, curY, to_x, to_y+1));
    else // curX == to_x
        rLst.push_back(Rect(curX, curY, to_x+1, to_y));
}

void RectListDrawer::RectToImpl(int to_x, int to_y)
{
    rLst.push_back(Rect(curX, curY, to_x, to_y));
}

RgnPixelDrawer::RgnPixelDrawer(RefPtr<Gdk::Pixbuf> canv_pix, RectListRgn* r_lst):
    MyParent(canv_pix), rLst(r_lst)
{ }

void RgnPixelDrawer::DrawWithFunctor(const Rect& plc_rct, DrwFunctor& drw_fnr)
{
    if( rLst )
    {
        Rect cut_rct;

        // Этот lambda-код эквивалентен стандартному, что ниже; в данном случае lambda
        // излишне использовать,- просто ради спортивного интереса 
        //using namespace boost;
        //std::for_each(rLst->begin(), rLst->end(), (
        //    lambda::var(cut_rct) = lambda::bind(&Intersection, lambda::_1, ref(plc_rct)),
        //    lambda::if_then( !lambda::bind(&Rect::IsNull, lambda::var(cut_rct)),
        //        lambda::bind(&DrwFunctor::operator(), ref(drw_fnr), lambda::var(cut_rct)) )
        //                                          ) );

        for( RLRIterCType cur = rLst->begin(), end = rLst->end(); cur != end; ++cur )
        {
            cut_rct = Intersection(plc_rct, *cur);
            if( !cut_rct.IsNull() )
                drw_fnr(cut_rct);
        }
    }
    else
        drw_fnr(plc_rct);
}

void RgnPixelDrawer::LineToImplRect(const RgnType& cut_rct, bool is_horiz)
{
    curX = cut_rct.lft;
    curY = cut_rct.top;
    MyParent::LineToImpl(is_horiz  ? cut_rct.rgt : cut_rct.lft, 
                         !is_horiz ? cut_rct.btm : cut_rct.top );
}

using namespace boost;

void RgnPixelDrawer::LineToImpl(int to_x, int to_y)
{
    bool is_horiz = curY == to_y;
    Rect lin_rct(curX, curY, is_horiz ? to_x : to_x+1, !is_horiz ? to_y : to_y+1);

    DrwFunctor drw_fnr = lambda::bind(&RgnPixelDrawer::LineToImplRect, this, lambda::_1, is_horiz);
    DrawWithFunctor(lin_rct, drw_fnr);
}

void RgnPixelDrawer::RectToImplRect(const RgnType& cut_rct)
{
    curX = cut_rct.lft;
    curY = cut_rct.top;
    MyParent::RectToImpl(cut_rct.rgt, cut_rct.btm);
}

void RgnPixelDrawer::RectToImpl(int to_x, int to_y)
{
    Rect rct(curX, curY, to_x, to_y);

    DrwFunctor drw_fnr = lambda::bind(&RgnPixelDrawer::RectToImplRect, this, lambda::_1);
    DrawWithFunctor(rct, drw_fnr);
}

void RgnPixelDrawer::ScalePixbuf(RefPtr<Gdk::Pixbuf> pix, const RgnType& plc)
{
    //DrwFunctor drw_fnr = bind(&Scale1, canvPix, pix, plc, _1);
    DrwFunctor drw_fnr = lambda::bind(&RGBA::ScalePixbuf, canvPix, pix, plc, lambda::_1);
    DrawWithFunctor(plc, drw_fnr);
}

void RgnPixelDrawer::CompositePixbuf(RefPtr<Gdk::Pixbuf> pix, const RgnType& plc)
{
    DrwFunctor drw_fnr = lambda::bind(&RGBA::AlphaCompositePixbuf, canvPix, pix, plc, lambda::_1);
    DrawWithFunctor(plc, drw_fnr);
}

//static void FillImpl(RefPtr<Gdk::Pixbuf> canv_pix, const Rect& rct, int clr)
//{
//    RefPtr<Gdk::Pixbuf> ch_pix = Gdk::Pixbuf::create_subpixbuf(canv_pix, rct.lft, rct.top,
//                                                               rct.Width(), rct.Height());
//    ch_pix->fill(clr);
//}

void RgnPixelDrawer::Fill(const RgnType& plc)
{
    //DrwFunctor drw_fnr = bl::bind(&FillImpl, canvPix, lambda::_1, fillClr.ToUint());
    //DrawWithFunctor(plc, drw_fnr);
    MoveTo(plc.lft, plc.top);
    RectTo(plc.rgt, plc.btm);
}

} // namespace RGBA

void DrawGrabFrame(RGBA::Drawer& drw, const Rect& plc_rct)
{
    Rect rct(plc_rct);
    rct.lft -= 1; // рисуем вокруг
    rct.top -= 1;

    const int frame_space = 1; // отступ от объекта
    rct.lft -= frame_space;
    rct.top -= frame_space;
    rct.btm += frame_space;
    rct.rgt += frame_space;

    const int frame_clr =  0x50ff7bff; // - зеленые цвета с одним оттенком = 135
    const int square_clr = 0x1da93fff; //
    drw.SetForegroundColor(frame_clr);

    // 1 рамка
    drw.MoveTo(rct.lft, rct.top);
    drw.FrameRectTo(rct.rgt, rct.btm);

    // 2 "ручки"-прямоугольники
    Point sz = rct.Size();
    int dot_tbl[9][2] =
    {
        {0, 0},      {sz.x/2, 0},      {sz.x, 0},
        {0, sz.y/2}, {sz.x/2, sz.y/2}, {sz.x, sz.y/2},
        {0, sz.y},   {sz.x/2, sz.y},   {sz.x, sz.y}
    };

    for( int x=0; x<3; x++ )
        for( int y=0; y<3; y++ )
        {
            if( x == 1 && y == 1 ) // середину не рисуем
                continue;

            int c_x = rct.lft + dot_tbl[x+y*3][0];
            int c_y = rct.top + dot_tbl[x+y*3][1];

            const int E_SZ = 2; // половина стороны квадратика
            drw.SetForegroundColor(frame_clr);
            drw.MoveTo(c_x-E_SZ,   c_y-E_SZ);
            // прибавляем 1 чтобы рисовалось симметрично
            drw.RectTo(c_x+E_SZ+1, c_y+E_SZ+1);

            const int I_SZ = 1;
            drw.SetForegroundColor(square_clr);
            drw.MoveTo(c_x-I_SZ,   c_y-I_SZ);
            drw.RectTo(c_x+I_SZ+1, c_y+I_SZ+1);
        }
}

