//
// mmedia.cpp
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

#include <mbase/composite/component.h>
#include "mmedia.h"

using namespace Planed;

namespace Composition {

StillPictMedia::StillPictMedia(const char* img_path) 
{
    stillImg.read(img_path);
}

YuvMedia::YuvMedia(const char* fpath)
    : MyYuvContext(-1)
{
    // желающие сами проверят, успешно ли открыли
    inFd = open(fpath, O_RDONLY);
}

bool YuvMedia::Begin()
{
    bool res = MyYuvContext::Begin();
    if( res )
    {
        GetMovieInfo(mInfo, (MyYuvContext&)(*this));
        InitImage(tmpImg, mInfo.Size().x, mInfo.Size().y);
    }
    return res;
}

void YuvMedia::MakeImage()
{
    int wdh = MyYuvContext::Width();
    int hgt = MyYuvContext::Height();
    YuvContextIter iter(*this);

    TransferToImg(tmpImg, wdh, hgt, iter);
    //return tmpImg;
}

MpegMedia::MpegMedia(io::stream* strm, SeqDemuxer* dmx)
    : mPlyr(strm, dmx)
{
}

void MpegMedia::MakeImage()
{
    Point sz = mInfo.Size();
    MpegDecodec::PlanesType buf = mPlyr.decodec.Planes();

    Plane y_p(sz.x, sz.y, buf[0]);
    Plane u_p(mInfo.uSz.x, mInfo.uSz.y, buf[1]);
    Plane v_p(mInfo.vSz.x, mInfo.vSz.y, buf[2]);
    YuvContextIter iter(y_p, u_p, v_p);

    TransferToImg(tmpImg, sz.x, sz.y, iter);
    //return tmpImg;
}

bool MpegMedia::Begin()
{
    bool res = mPlyr.NextSeq();
    if( res )
    {
        if( !GetMovieInfo(mInfo, mPlyr.decodec) )
            return false;

        InitImage(tmpImg, mInfo.Size().x, mInfo.Size().y);
    }
    return res;
}

bool MpegMedia::NextFrame()
{
    return mPlyr.NextImage();
}

Media* GetMedia(MediaObj& mobj)
{
    return mobj.GetData<MediaStrategy>().GetMedia();
}

void SetMedia(MediaObj& mobj, Media* md)
{
    return mobj.GetData<MediaStrategy>().SetMedia(md);
}

} // namespace Composition

void InitImage(Magick::Image& img, int wdh, int hgt)
{
    // 0 - выделение ресурсов для Image - AllocateImage() - 
    //     уже сделано в конструкторе Image
    
    // 1 - установить размеры и тип
    img.size( Magick::Geometry(wdh, hgt) );
    // чтобы были единственными владельцами - специфика Magick++, не Magick
    img.modifyImage();
    img.type(MagickLib::TrueColorType); // неявный перевод в RGBColorspace

#if (MagickLibVersion < 0x030000) || ((MagickLibVersion >= 0x100000) && (MagickLibVersion < 0x300000))
// см. configure.ac: из-за превышения 9 номер интерфейса (major interface) увелич. в 10 раз
#define GM2_ABI
#endif

    // с интерфейса >= 3 (версия 1.3) инициализация проходит при первом обращении Pixels::set|getConst()
#ifdef GM2_ABI
    // 2 - выделить и инициализировать ресурсы для хранения собственно
    //     данных и кеша представлений
    MagickLib::OpenCache(img.image(), MagickLib::IOMode);
#endif
}

void TransferToImg(Magick::Image& tmp_img, int wdh, int hgt,
                           YuvContextIter& iter, bool convert_to_rgb)
{
    // 1 так как записано в YCbCr
    if( convert_to_rgb )
        tmp_img.image()->colorspace = MagickLib::YCbCrColorspace; // YUVColorspace;

    // 2 перевод в Image
    //MagickLib::PixelPacket* p = tmp_img.setPixels(0, 0, wdh, hgt);
    Magick::Pixels view(tmp_img);
    MagickLib::PixelPacket* p = view.set(0, 0, wdh, hgt);
    ASSERT(p);

    for( int y=0; y<hgt; y++, iter.YAdd() )
    {
        iter.XSet(0);
        for( int x=0; x<wdh; x++, iter.XAdd() )
        {
            // очень медленно так
            //Magick::Color color( iter.yIter.constRes(), iter.uIter.constRes(), iter.vIter.constRes() );
            //*p = color;
            p->red   = iter.yIter.constRes();
            p->green = iter.uIter.constRes();
            p->blue  = iter.vIter.constRes();
            p++;
        }
    }
    // 3
    //tmp_img.syncPixels();
    view.sync();
    // 4 перевод в RGB
    // :TODO: попробовать все делать в YCbCr, так быстрее получится
    tmp_img.colorSpace(MagickLib::RGBColorspace);
}

// указатель на один из цветов пикселя
typedef MagickLib::Quantum MagickLib::PixelPacket::*PPComponent;

////////////////////////////////////////
// Оптимизация деления
enum ChromaDivType
{
    OneDiv,  // делим на 1
    TwoDiv,  // 2
    FourDiv, // 4
    EightDiv,// 8
    DivDiv   // остальные
};

template<ChromaDivType DivType>
inline int DivFunc(int src, int div)
{
    return src/div;
}
template<>
inline int DivFunc<OneDiv>(int src, int)
{
    return src;
}
template<>
inline int DivFunc<TwoDiv>(int src, int)
{
    return src >> 1;
}
template<>
inline int DivFunc<FourDiv>(int src, int)
{
    return src >> 2;
}
template<>
inline int DivFunc<EightDiv>(int src, int)
{
    return src >> 3;
}
////////////////////////////////////////

template<ChromaDivType DivType>
static void CopyImageToPlaneT(PlaneIter& p_iter, const MagickLib::PixelPacket* img_buf, 
                               PPComponent ppc, int chroma_cnt)
{
    int x_add = p_iter.xIter.quant;
    int y_add = p_iter.yIter.quant;
    //int chroma_cnt = x_add*y_add;

    int x_sz = p_iter.plane.x;
    int y_sz = p_iter.plane.y;

    int wdh = x_sz*x_add;
    unsigned char* p_dst = p_iter.plane.data;
    const MagickLib::PixelPacket* p_src = img_buf;

    // один ряд во внутреннем цикле прибавляется
    int to_next_row = wdh*(y_add-1);
    for( int y=0; y<y_sz; y++, p_src+=to_next_row ) //wdh*(y_add-1) )
    {
        for( int x=0; x<x_sz; x++, p_src+=x_add )
        {
            // собираем среднее арифметическое по прямоугольнику x_add*y_add
            int res = 0;
            const MagickLib::PixelPacket* r_src = p_src;
            for( int y_i=0; y_i<y_add; y_i++, r_src+=wdh )
                for( int x_i=0; x_i<x_add; x_i++ )
                    // 
                    res += r_src[x_i].*ppc;

            *p_dst++ = DivFunc<DivType>(res, chroma_cnt);//res/chroma_cnt;
        }
    }
}

static void CopyImageToPlane(PlaneIter& p_iter, const MagickLib::PixelPacket* img_buf,
                               PPComponent ppc)
{
    int x_add = p_iter.xIter.quant;
    int y_add = p_iter.yIter.quant;
    int chroma_cnt = x_add*y_add;
    // выбираем наиболее быстрый вариант
    switch( chroma_cnt )
    {
    case 1:
        CopyImageToPlaneT<OneDiv>(p_iter, img_buf, ppc, chroma_cnt);
        break;
    case 2:
        CopyImageToPlaneT<TwoDiv>(p_iter, img_buf, ppc, chroma_cnt);
        break;
    case 4:
        CopyImageToPlaneT<FourDiv>(p_iter, img_buf, ppc, chroma_cnt);
        break;
    case 8:
        CopyImageToPlaneT<EightDiv>(p_iter, img_buf, ppc, chroma_cnt);
        break;
    default:
        CopyImageToPlaneT<DivDiv>(p_iter, img_buf, ppc, chroma_cnt);
        break;
    }
}

static void CopyImageToPlanes(YuvContextIter& iter, const MagickLib::PixelPacket* img_buf)
{
    // Y
    CopyImageToPlane(iter.yIter, img_buf, &MagickLib::PixelPacket::red);
    // U
    CopyImageToPlane(iter.uIter, img_buf, &MagickLib::PixelPacket::green);
    // V
    CopyImageToPlane(iter.vIter, img_buf, &MagickLib::PixelPacket::blue);
}

void CopyImageToPlanes(OutYuvContext& out_cont, const Magick::Image& img)
{
    int wdh = out_cont.Width();
    int hgt = out_cont.Height();
    if( (wdh != (int)img.columns()) || (hgt != (int)img.rows()) )
        Error("StrmWriteImage() : not compliant image!");

    //const MagickLib::PixelPacket* p = img.getConstPixels(0, 0, wdh, hgt);
    // представление требует неконстантного изображения
    Magick::Pixels view(const_cast<Magick::Image&>(img));
    const MagickLib::PixelPacket* p = view.getConst(0, 0, wdh, hgt);
    ASSERT(p);

    YuvContextIter iter(out_cont);
    CopyImageToPlanes(iter, p);
}

static void StrmWriteImageImpl(OutYuvContext& out_strm, const Magick::Image& img)
{
    CopyImageToPlanes(out_strm, img);

    // записываем
    out_strm.PutFrame();
}

void StrmWriteImage(OutYuvContext& out_strm, const Magick::Image& img)
{
    if( img.colorSpace() != Magick::YCbCrColorspace )
    {
        Magick::Image tmp_img(img);
        // после создания по Image реально новый Image не создается
        // а увеличивается кол-во ссылок, поэтому вызываем modifyImage()
        tmp_img.modifyImage();
        tmp_img.colorSpace(Magick::YCbCrColorspace);

        StrmWriteImageImpl(out_strm, tmp_img);
    }
    else
        StrmWriteImageImpl(out_strm, img);
}

