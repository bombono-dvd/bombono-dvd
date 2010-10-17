//
// mgui/project/thumbnail.cpp
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

#include "thumbnail.h"
#include "menu-browser.h"

#include <mgui/render/menu.h>
#include <mgui/sdk/browser.h>
#include <mgui/img-factory.h>

#include <mbase/project/table.h>
#include <mbase/project/colormd.h>

#include <mlib/sdk/logger.h>
#include <math.h> // sqrt()


namespace Project
{

// 
// PrimaryShotGetter
// 

void PrimaryShotGetter::Visit(StillImageMD& obj)
{
    pixExt = new ImgFilePE(GetFilename(obj));
}

struct BrowserCache
{
        ptr::weak_intrusive<VideoMD> curVI;
                    Mpeg::FwdPlayer  player;

        BrowserCache() { RGBOpen(player); }
};

static Mpeg::FwdPlayer& OpenCachePlayer(VideoItem vmd)
{
    BrowserCache& cache = AData().GetData<BrowserCache>();
    if( (cache.curVI.lock() != vmd) || !cache.player.IsOpened() )
    {
        cache.curVI = vmd;
        CheckOpen(cache.player, GetFilename(*vmd));
    }
    return cache.player;
}

//RefPtr<Gdk::Pixbuf> GetVideoFrame(VideoItem vmd, double time, const Point& sz)
//{
//    Mpeg::FwdPlayer& player = OpenCachePlayer(vmd);
//    RefPtr<Gdk::Pixbuf> pix = CreatePixbuf(sz);
//    return GetFrame(pix, time, player);
//}

const double VIDEO_THUMB_TIME = 0.;

VideoStart GetVideoStart(MediaItem mi)
{
    VideoStart res;

    if( VideoItem vi = IsVideo(mi) )
    {
        res.first  = vi;
        res.second = VIDEO_THUMB_TIME;
    }
    else if( ChapterItem ci = IsChapter(mi) )
    {
        res.first  = ci->owner;
        res.second = ci->chpTime;
    }
    else
        ASSERT(0);
    return res;
}

VideoPE::VideoPE(VideoStart vs): plyr(OpenCachePlayer(vs.first)), time(vs.second) {}

static VideoPE* CreateVideoPE(Media& md)
{
    return new VideoPE(GetVideoStart(&md));
}

void PrimaryShotGetter::Visit(VideoMD& obj)
{
    pixExt = CreateVideoPE(obj);
}

void PrimaryShotGetter::Visit(VideoChapterMD& obj)
{
    pixExt = CreateVideoPE(obj);
}

void PrimaryShotGetter::Visit(ColorMD& obj)
{
    pixExt = new Color11PE(obj.Color());
}

PixbufExtractor PrimaryShotGetter::Make(MediaItem mi)
{
    PixbufExtractor ext;
    if( mi )
    {
        PrimaryShotGetter vis;
        mi->Accept(vis);
        ext = vis.pixExt;
    }
    if( !ext )
        ext = new BlackPE;
    return ext;
}

//
// GetCacheShot()
// 

class ThumbSizeCalcer: public ObjVisitor
{
    public:
    Point sz;

    virtual  void  Visit(StillImageMD& obj);
    virtual  void  Visit(VideoMD& obj);
    virtual  void  Visit(VideoChapterMD& obj);
    virtual  void  Visit(MenuMD& obj);
};

const int MENU_ITEM_AREA = 325*245;
const int ORIG_ITEM_AREA = BIG_THUMB_WDH*BIG_THUMB_WDH*3/4;

Point CalcProportionSize(const Point& sz, double etalon_square)
{
    // вычисляем размеры кэша
    double mult = sqrt(sz.x*sz.y/etalon_square);
    return Point(Round(sz.x/mult), Round(sz.y/mult));
}

void ThumbSizeCalcer::Visit(StillImageMD& obj)
{
    bool true_ = gdk_pixbuf_get_file_info(GetFilename(obj).c_str(), &sz.x, &sz.y);
    ASSERT_OR_UNUSED( true_ );

    // вычисляем размеры кэша
    sz = CalcProportionSize(sz, ORIG_ITEM_AREA);
}

Point CalcAspectSize(VideoMD& vi)
{
    Mpeg::FwdPlayer& player = OpenCachePlayer(&vi);
    return Mpeg::GetAspectRatio(player);
}

void ThumbSizeCalcer::Visit(VideoMD& obj)
{
    sz = CalcProportionSize(CalcAspectSize(obj), ORIG_ITEM_AREA);
}

void ThumbSizeCalcer::Visit(VideoChapterMD& obj)
{
    Visit(*obj.owner);
}

void ThumbSizeCalcer::Visit(MenuMD& obj)
{
    sz = CalcProportionSize(obj.Params().DisplayAspect(), MENU_ITEM_AREA);
}

struct CacheShotStruct
{
    RefPtr<Gdk::Pixbuf> shot;
                  bool  isUpdated; // для первичных медиа - обновлен ли shot

                  CacheShotStruct(): isUpdated(false) {}
};

Point CalcThumbSize(MediaItem mi)
{
    ThumbSizeCalcer tsc;
    mi->Accept(tsc);
    return tsc.sz;
}

CacheShotStruct& GetCacheStruct(MediaItem mi)
{
    CacheShotStruct& cache = mi->GetData<CacheShotStruct>();
    if( !cache.shot )
    {
        Point sz = CalcThumbSize(mi);
        LOG_DBG << "GetCacheShot " << sz << io::endl;

        cache.shot = CreatePixbuf(sz);
    }
    return cache;
}

RefPtr<Gdk::Pixbuf> GetCacheShot(MediaItem mi)
{
    return GetCacheStruct(mi).shot;
}

RefPtr<Gdk::Pixbuf> GetPrimaryShot(MediaItem mi)
{
    if( !mi )
        return MakeColor11Image(BLACK_CLR);

    CacheShotStruct& cache = GetCacheStruct(mi);
    if( !cache.isUpdated )
    {
        PrimaryShotGetter::Make(mi)->Fill(cache.shot);
        cache.isUpdated = true;
    }
    return cache.shot;
}

static void UpdatePrimaryMediaShot(MediaItem mi, bool is_update)
{
    if( is_update )
        GetPrimaryShot(mi);
    else
        GetCacheStruct(mi).isUpdated = false;
}

class ShotUpdaterVis: public ObjVisitor
{
    public:
        RefPtr<Gdk::Pixbuf> shot;

                   ShotUpdaterVis(bool is_update): isUpdate(is_update) {}

    virtual  void  Visit(StillImageMD& obj)   { UpdatePrimaryMediaShot(&obj, isUpdate); }
    virtual  void  Visit(VideoMD& obj)        { UpdatePrimaryMediaShot(&obj, isUpdate); }
    virtual  void  Visit(VideoChapterMD& obj) { UpdatePrimaryMediaShot(&obj, isUpdate); }
    virtual  void  Visit(MenuMD& obj);
    virtual  void  Visit(ColorMD& obj)        { UpdatePrimaryMediaShot(&obj, isUpdate); }

    public:
        bool isUpdate;
};

void ShotUpdaterVis::Visit(MenuMD& obj)
{ 
    if( isUpdate )
        shot = GetRenderedShot(&obj);
    else
        SetMenuDirty(&obj);
}

RefPtr<Gdk::Pixbuf> GetCalcedShotImpl(MediaItem mi)
{
    if( !mi )
        return MakeColor11Image(BLACK_CLR);

    ShotUpdaterVis upd_vis(true);
    mi->Accept(upd_vis);
    return upd_vis.shot ? upd_vis.shot : GetCacheShot(mi) ;
}

RefPtr<Gdk::Pixbuf> GetCalcedShot(MediaItem mi)
{
    using namespace boost;
    return GetInteractiveLRS( lambda::bind(&GetCalcedShotImpl, mi) );
}

void SetDirtyCacheShot(MediaItem mi)
{
    ShotUpdaterVis upd_vis(false);
    mi->Accept(upd_vis);
}

RefPtr<Gdk::Pixbuf> ImgFilePE::MakeImage(const Point& sz)
{
    RefPtr<Gdk::Pixbuf> img;
    if( !IsNullSize(sz) )
        img = Gdk::Pixbuf::create_from_file(imgFName, sz.x, sz.y, false);
    else
        img = Gdk::Pixbuf::create_from_file(imgFName);
    return img;
}

PixbufSource ImgFilePE::Make(const Point& sz)
{
    return PixbufSource(MakeImage(sz), false);
}

void ImgFilePE::Fill(RefPtr<Gdk::Pixbuf>& pix)
{
    if( pix )
    {
        Point sz = PixbufSize(pix);
        //RGBA::Scale(pix, img);
        MakeImage(sz)->copy_area(0, 0, sz.x, sz.y, pix, 0, 0);
    }
    else
        pix = MakeImage(Point());
}

static PixbufSource FormPixbufSource(const Point& sz, RefPtr<Gdk::Pixbuf> src, bool read_only)
{
    if( IsNullSize(sz) || (sz == PixbufSize(src)) )
        return PixbufSource(src, read_only);
    else
    {
        RefPtr<Gdk::Pixbuf> pix = CreatePixbuf(sz);
        RGBA::Scale(pix, src);
        return PixbufSource(pix, false);
    }
}

static void FillPixbuf(RefPtr<Gdk::Pixbuf>& pix, RefPtr<Gdk::Pixbuf> src, bool read_only)
{
    if( pix )
    {
        Point sz = PixbufSize(src);
        if( sz == PixbufSize(pix) )
            src->copy_area(0, 0, sz.x, sz.y, pix, 0, 0);
        else    
            RGBA::Scale(pix, src);
    }
    else
        pix = PixbufSource(src, read_only).RWPixbuf();
}

PixbufSource ImagePE::Make(const Point& sz)
{
    return FormPixbufSource(sz, origPix, readOnly);
}

void ImagePE::Fill(RefPtr<Gdk::Pixbuf>& pix)
{
    FillPixbuf(pix, origPix, readOnly);
}

PixbufSource VideoPE::Make(const Point& sz)
{
    RefPtr<Gdk::Pixbuf> img = GetRawFrame(time, plyr);
    if( img )
        return FormPixbufSource(sz, img, true);
    else
    {
        img = CreatePixbuf(sz);
        FillEmpty(img);
        return PixbufSource(img, false);
    }
}

void VideoPE::Fill(RefPtr<Gdk::Pixbuf>& pix)
{
    RefPtr<Gdk::Pixbuf> img = GetRawFrame(time, plyr);
    if( img )
        FillPixbuf(pix, img, true);
    else
        FillEmpty(pix);
}

PixbufSource Color11PE::Make(const Point& sz)
{
    RefPtr<Gdk::Pixbuf> pix = CreatePixbuf(sz);
    Fill(pix);
    return PixbufSource(pix, false);
}

void Color11PE::Fill(RefPtr<Gdk::Pixbuf>& pix)
{
    if( !pix )
        pix = Make11Image();
    pix->fill(clr);
}

void StampFPEmblem(MediaItem mi, RefPtr<Gdk::Pixbuf> pix)
{
    if( AData().FirstPlayItem() != mi )
        return;
    bool is_menu = IsMenu(mi);
    RefPtr<Gdk::Pixbuf> emblem = GetFactoryImage(is_menu ? "copy-n-paste/HelixPlayer_Activity-watch-listen/28.png" 
                                                         : "copy-n-paste/HelixPlayer_Activity-watch-listen/16.png");
    CheckEmblem(pix, emblem);

    // местоположение
    Point e_sz(PixbufSize(emblem));
    Point p_sz(PixbufSize(pix));
    Rect plc = RectASz(Point(2, is_menu ? (p_sz.y-e_sz.y)/2 : 2), e_sz);
    RGBA::AlphaComposite(pix, emblem, plc);
}

} // namespace Project

