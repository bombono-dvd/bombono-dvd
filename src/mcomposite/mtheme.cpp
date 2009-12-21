//
// mtheme.cpp
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

#include <mlib/tech.h>

#include <mbase/project/theme.h>
#include <mbase/composite/component.h>

#include "mtheme.h"
#include "mutils.h"

static Magick::Geometry MakeGeometry(int wdh, int hgt, bool is_aspect = true)
{
    Magick::Geometry res(wdh, hgt);
    res.aspect(is_aspect);

    return res;
}

static void LoadThemeFrame(Magick::Image& img, const std::string& path, Rect& plc)
{
    img.read(path.c_str());
    Point sz = plc.Size();
    img.zoom( MakeGeometry(sz.x, sz.y) );
}

// загрузить рамку из директории
void LoadTheme(FrameThemeObj& fto)
{
//     if( !thm_path && !*thm_path )
//         return;
//
//     // убираем '/' в конце, если нужно
//     thmPath = thm_path;
//     if( thmPath[thmPath.length()-1] == '/' )
//         thmPath.resize(thmPath.length()-1);
//
//     std::string path(thmPath);
//     path += "/frame.png";
// //     framImg.read(path);
//     LoadThemeFrame(framImg, path.c_str(), mdPlc);
//
//     path  = thmPath;
//     path += "/vframe.png";
// //     vframImg.read(path);
//     LoadThemeFrame(vframImg, path.c_str(), mdPlc);

    Rect md_plc = fto.Placement();
    fs::path pth = Project::FindThemePath(Project::ThemeOrDef(fto.Theme()));
    LoadThemeFrame(FrameImg(fto),  (pth / "frame.png").string(),  md_plc);
    LoadThemeFrame(VFrameImg(fto), (pth / "vframe.png").string(), md_plc);
}

namespace Img {
MagickLib::Image* ZoomImage(const MagickLib::Image *image,const unsigned long columns,
  const unsigned long rows, MagickLib::ExceptionInfo *exception);
}

void ZoomImage(Magick::Image& dst, const Magick::Image& src, int width, int height)
{
    MagickLib::ExceptionInfo exceptionInfo;
    MagickLib::GetExceptionInfo( &exceptionInfo );

    MagickLib::Image* newImage = MagickLib::ZoomImage( src.constImage(), width, height,
    //MagickLib::Image* newImage = Img::ZoomImage( src.constImage(), width, height,
                 &exceptionInfo);

    dst.replaceImage( newImage );
    Magick::throwException( exceptionInfo );
}

// // отобразить на холсте элемент
// void Composition::FramedObj::Composite(Magick::Image& canv_img, const Magick::Image& obj_img)
// {
//     Magick::Image img;
//     Point sz = mdPlc.Size();
//     ZoomImage(img, obj_img, sz.x, sz.y);
//
//     // сначала скопируем маску в изображение,
//     // которое накладываем, используя опять же composite(CopyOpacityCompositeOp)
//     // 1 включаем прозрачность
//     img.matte(true);
//     // 2 копируем прозрачность с рамки
//     img.composite(vframImg, 0, 0, MagickLib::CopyOpacityCompositeOp);
//
//     // 3 накладываем результат c альфа-блэндингом (OverCompositeOp)
//     canv_img.composite(framImg, mdPlc.lft, mdPlc.top, MagickLib::OverCompositeOp);
//     canv_img.composite(img, mdPlc.lft, mdPlc.top, MagickLib::OverCompositeOp);
// }

struct ThemePair_
{
    Magick::Image  framImg;
    Magick::Image  vframImg;
};

Magick::Image& FrameImg(FrameThemeObj& fto)
{
    return fto.GetData<ThemePair_>().framImg;
}

Magick::Image& VFrameImg(FrameThemeObj& fto)
{
    return fto.GetData<ThemePair_>().vframImg;
}

void Composite(Magick::Image& canv_img, const Magick::Image& obj_img, FrameThemeObj& fto)
{
    Magick::Image img;
    Rect mdPlc = fto.Placement();
    Point sz   = mdPlc.Size();
    ZoomImage(img, obj_img, sz.x, sz.y);

    // сначала скопируем маску в изображение,
    // которое накладываем, используя опять же composite(CopyOpacityCompositeOp)
    // 1 включаем прозрачность
    img.matte(true);
    // 2 копируем прозрачность с рамки
    img.composite(VFrameImg(fto), 0, 0, MagickLib::CopyOpacityCompositeOp);

    // 3 накладываем результат c альфа-блэндингом (OverCompositeOp)
    canv_img.composite(FrameImg(fto), mdPlc.lft, mdPlc.top, MagickLib::OverCompositeOp);
    canv_img.composite(img, mdPlc.lft, mdPlc.top, MagickLib::OverCompositeOp);
}

