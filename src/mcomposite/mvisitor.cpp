//
// mvisitor.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007 Ilya Murav'jov
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

#include <Magick++.h>

#include <mbase/composite/component.h>

#include "mvisitor.h"
#include "mmedia.h"
#include "mtheme.h"

namespace Composition {

void MovieVisitor::Visit(FrameThemeObj& obj)
{
    GetMedia(obj)->Accept(*this);
}

void MovieVisitor::Visit(SimpleOverObj& obj)
{
    GetMedia(obj)->Accept(*this);
}

} // namespace Composition

ImgComposVis::ImgComposVis(int wdh, int hgt) : isEnd(false), grp(0)
{
    InitImage(canvImg, wdh, hgt);
}

void ImgComposVis::Visit(FrameThemeObj& obj)
{
    MyParent::Visit(obj);

    Composite(canvImg, Comp::GetMedia(obj)->GetImage(), obj);
}

void ImgComposVis::Visit(SimpleOverObj& obj)
{
    MyParent::Visit(obj);

    const Magick::Image& img = Comp::GetMedia(obj)->GetImage();
    // 1 масштабируем, если нужно
    const Rect& rct = obj.Placement(); 
    Point sz = rct.Size();
    if( sz != Point(img.columns(), img.rows()) )
    {
        Magick::Image tmp_img;
        ZoomImage(tmp_img, img, sz.x, sz.y);

        canvImg.composite(tmp_img, rct.lft, rct.top);
    }
    else
        canvImg.composite(img, rct.lft, rct.top);
}

void ImgComposVis::Visit(Comp::MovieMedia& mm)
{
//     if( !isEnd )
//         isEnd = !mm.NextFrame();
    if( !isEnd && mm.NextFrame() )
        mm.MakeImage();
    else
        isEnd = true;
}

void ImgComposVis::First()
{
    isEnd = false;
    curNum = 0;
    if( !IsDone() )
        grp->Accept(*this);
}

void ImgComposVis::Next()
{
    FrameCounter::operator++();
    if( !IsDone() )
        grp->Accept(*this);
}

bool ImgComposVis::IsDone() const
{
    return isEnd || FrameCounter::IsDone();
}

static bool operator ==(const y4m_ratio_t& r1, const y4m_ratio_t& r2)
{
    return (r1.d == r2.d) && (r1.n == r2.n);
}

void DoBeginVis::Visit(Comp::MovieMedia& mm)
{
    if( isGood && (&mm != basMedia) )
    {
        isGood = mm.Begin();
        // частота кадров должна совпадать
        if( !(y4m_si_get_framerate(&patInfo.streamInfo) ==
              y4m_si_get_framerate(&mm.Info().streamInfo)) )
            isGood = false;
    }
}

void DoBeginVis::Visit(FrameThemeObj& obj)
{
    MyParent::Visit(obj);

    ::LoadTheme(obj);
}

bool DoBeginVis::Begin()
{
    // 1 базовое видео
    isGood = false;
    if( basMedia && basMedia->Begin() )
    {
        patInfo = basMedia->Info();
        isGood = true;
    }

    if( !isGood )
    {
        // плохое видео
        io::cerr << "Bad base movie!";
        //usage(argv[0]);
        return false;
    }

    // 2 мелочи
    // и размеры фона по нему берем
    Point sz = patInfo.Size();
    bckSoo->SetPlacement( Rect(0, 0, sz.x, sz.y) );

    // 3 проверяем остальное видео
    lstObj.Accept(*this);

    return isGood;
}

