//
// mgui/project/menu-actions.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2010 Ilya Murav'jov
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

#include "menu-actions.h"

#include "menu-render.h"
#include "handler.h"

#include <mbase/obj_bind.h>
#include <mgui/render/editor.h>
#include <mgui/editor/render.h>
#include <mgui/text_obj.h>
#include <mgui/redivide.h>

namespace Project
{

static void OpenMenus(MenuList& ml)
{
    for( MenuList::Itr itr = ml.Beg(), end = ml.End(); itr != end; ++itr )
        OpenMenu(*itr);
}

void FillThumbnail(const Gtk::TreeIter& itr, RefPtr<MenuStore> ms, bool force_thumb = false)
{
    Menu mn = GetMenu(ms, itr);
    RefPtr<Gdk::Pixbuf> thumb_pix = itr->get_value(ms->columns.thumbnail);
    if( !thumb_pix )
    {
        thumb_pix = mn->GetData<MenuPack>().thumbPix;
        itr->set_value(ms->columns.thumbnail, thumb_pix);
    }

    // *
    MenuPack& mn_pack = UpdateMenuPack(mn);
    if( force_thumb )
        RenderThumbnail(mn_pack);
    // *
    if( mn_pack.thumbNeedUpdate || force_thumb )
        ms->row_changed(ms->get_path(itr), itr);
    mn_pack.thumbNeedUpdate = false;
}

void PublishMenu(const Gtk::TreeIter& itr, RefPtr<MenuStore> ms, Menu mn)
{
    (*itr)[ms->columns.menu] = mn;
    FillThumbnail(itr, ms);

    ReindexFrom(ms, itr);
}

// меню не должно зависеть от неоткрытых меню
void OpenPublishMenu(const Gtk::TreeIter& itr, RefPtr<MenuStore> ms, Menu mn)
{
    OpenMenu(mn);
    PublishMenu(itr, ms, mn);
}

void InsertMenuIntoBrowser(MenuBrowser& brw)
{
    RefPtr<MenuStore> ms = brw.GetMenuStore();
    // * subj
    Menu mn = MakeMenu(Size(ms));
    // отношение сторон
    mn->Params().GetAF() = IsMenuToBe4_3() ? af4_3 : af16_9 ;

    // * куда
    Gtk::TreeRow row = *InsertByPos(ms, GetCursor(brw));
    OpenPublishMenu(row, ms, mn);
    InvokeOnInsert(mn);

    // * фокус
    GoToPos(brw, ms->get_path(row));
}

// расчет областей отрисовки при изменении содержимого (первичного медиа)
class RegionEraserVis: public GuiObjVisitor
{
    public:
                       RegionEraserVis(Comp::Object* l_obj, bool del_link, bool for_poster);

                 void  Process();

        virtual  void  Visit(MenuRegion& mr);
        virtual  void  Visit(FrameThemeObj& fto);
        virtual  void  Visit(TextObj& txt);

    protected:
        Comp::Object* lObj;
                bool  delLink;
                bool  forPoster;

                Rect  plc;
              MenuMD* owner;
            MenuPack& mPack;
                       // рассчитать что перерисовать для данного меню
        virtual  void  CalcSubRegions(RectListRgn& lst) = 0;

void  ProcessImpl(bool exceed);
};

class PrimaryRegionEraserVis: public RegionEraserVis
{
    typedef RegionEraserVis MyParent;
    public:
                       PrimaryRegionEraserVis(Comp::Object* l_obj, bool del_link, bool for_poster)
                        : MyParent(l_obj, del_link, for_poster) {}

    protected:
        virtual  void  CalcSubRegions(RectListRgn& lst) { lst.push_back(plc); }
};

class MenuRegionEraserVis: public RegionEraserVis
{
    typedef RegionEraserVis MyParent;
    public:
                       MenuRegionEraserVis(Comp::Object* l_obj, const Point& mn_sz, 
                                           RectListRgn& r_lst)
                        : MyParent(l_obj, false, false), menuSz(mn_sz), rLst(r_lst) {}

    protected:
            const Point menuSz; // список измененных областей в меню
           RectListRgn& rLst;

        virtual  void  CalcSubRegions(RectListRgn& lst);
};

RegionEraserVis::RegionEraserVis(Comp::Object* l_obj, bool del_link, bool for_poster)
    :lObj(l_obj), owner(GetOwnerMenu(lObj)), mPack(owner->GetData<MenuPack>()),
     delLink(del_link), forPoster(for_poster)
{}

void RegionEraserVis::Visit(MenuRegion& mr)
{
    ASSERT( !forPoster );
    plc = mr.GetCanvasBuf().FrameRect();
    if( mPack.editor )
        ResetBackgroundImage(mr);
        
    if( delLink )
        mr.BgRef().ClearLink();
}

void RegionEraserVis::Visit(FrameThemeObj& fto)
{
    CommonMediaLink& lnk = GetFTOLink(fto, forPoster);
    if( lnk.Link() == MIToDraw(fto) )
    {
        // * добавляем область
        plc = Planed::AbsToRel(mPack.thRgn.Transition(), fto.Placement());
        // * обнуляем
        Editor::FTOData& dat = mPack.editor ? fto.GetData<FTOInterPixData>() : 
            (Editor::FTOData&)fto.GetData<FTOThumbData>() ;
        dat.ClearPix();
    }

    if( delLink )
        lnk.ClearLink();
}

void RegionEraserVis::Visit(TextObj& txt)
{
    if( delLink )
        txt.MediaItem().ClearLink();
}

void UpdateMenuRegionObject(Comp::Object* obj, const Point& menu_sz, RectListRgn& lst)
{
    MenuRegionEraserVis vis(obj, menu_sz, lst);
    //PrimaryRegionEraserVis vis(obj, false);
    vis.Process();
}

void EraseLinkedMenus(MenuPack& mp)
{
    using namespace boost;
    CanvasBuf& cb = mp.thRgn.GetCanvasBuf();
    ForeachLinked(mp.Owner(), lambda::bind(&UpdateMenuRegionObject, 
                                           boost::lambda::_1, boost::cref(cb.Size()), 
                                           boost::ref(cb.RenderList())));
}

void RegionEraserVis::ProcessImpl(bool exceed)
{
    if( exceed )
        return;

    AcceptOnlyObject(lObj, *this);
    if( !plc.IsNull() )
    {
        RectListRgn& r_lst = mPack.thRgn.GetCanvasBuf().RenderList();
        CalcSubRegions(r_lst);
        if( r_lst.size() )
        {
            // рекурсивно вызываем обновление
            EraseLinkedMenus(mPack);
        }
    }
}

void RegionEraserVis::Process()
{
    using namespace boost;
    static int RecurseDepth = 0;
    LimitedRecursiveCall<void>( lambda::bind(&RegionEraserVis::ProcessImpl, this, lambda::_1), RecurseDepth );
}

void MenuRegionEraserVis::CalcSubRegions(RectListRgn& lst)
{
    MapRectList(lst, rLst, plc, menuSz);
}

class MenuStoreVis: public ObjVisitor
{
    public:
        RefPtr<MenuStore>  menuStore;

                    MenuStoreVis(RefPtr<MenuStore> mn_store):
                        menuStore(mn_store) {}
};

class MenuStoreOnChangeVis: public MenuStoreVis
{
    typedef MenuStoreVis MyParent;
    public:

                    MenuStoreOnChangeVis(RefPtr<MenuStore> mn_store):
                        MyParent(mn_store) {}

     virtual  void  Visit(VideoChapterMD& obj);
     virtual  void  Visit(MenuMD& obj);
};

static void RedrawMenus(RefPtr<MenuStore> mn_store)
{
    Gtk::TreeIter beg = mn_store->children().begin(), end = mn_store->children().end();
    // * переразбиваем области
    for( Gtk::TreeIter itr = beg; itr != end; ++itr )
    {
        MenuRegion& m_rgn = GetMenuRegion(GetMenu(mn_store, itr));
        ReDivideRects(GetRenderList(m_rgn), m_rgn);
    }

    // * обновить миниатюры меню (с отрисовкой каждого меню)
    for( Gtk::TreeIter itr = beg; itr != end; ++itr )
        FillThumbnail(itr, mn_store);
}

static void UpdateMenuObject(Comp::Object* obj, bool del_link, bool for_poster)
{
    PrimaryRegionEraserVis vis(obj, del_link, for_poster);
    vis.Process();
}

static void UpdateFTO(FrameThemeObj& fto, bool del_link)
{
    UpdateMenuObject(&fto, del_link, true);
}

static void UpdateMenusFor(MediaItem mi, bool del_link)
{
    ForeachLinked(mi, bl::bind(&UpdateMenuObject, bl::_1, del_link, false));
    ForeachWithPoster(mi, bl::bind(&UpdateFTO, bl::_1, del_link));
}

static void UpdateRedrawMenusFor(MediaItem mi, bool del_link, RefPtr<MenuStore> mn_store)
{
    UpdateMenusFor(mi, del_link);
    RedrawMenus(mn_store);
}

void MenuStoreOnChangeVis::Visit(VideoChapterMD& chp)
{
    UpdateRedrawMenusFor(&chp, false, menuStore);
}

void MenuStoreOnChangeVis::Visit(MenuMD& /*obj*/)
{
    // только перерисовать, потому что заполнение было сделано самим редактором
    RedrawMenus(menuStore);
}

class MenuStoreOnDeleteVis: public MenuStoreVis
{
    typedef MenuStoreVis MyParent;
    public:

                    MenuStoreOnDeleteVis(RefPtr<MenuStore> mn_store):
                        MyParent(mn_store) {}

     virtual  void  Visit(StillImageMD& obj)  { UpdateObject(obj); }
     virtual  void  Visit(VideoChapterMD& obj){ UpdateObject(obj); }
     virtual  void  Visit(VideoMD& obj);
     virtual  void  Visit(MenuMD& obj)        { UpdateObject(obj); }

     protected:
              void  UpdateObject(Media& obj);
};

static void ClearFirstPlayProperty(MediaItem mi)
{
    MediaItem& fp = AData().FirstPlayItem();
    if( fp == mi )
        fp = MediaItem(); // очистка
}

void MenuStoreOnDeleteVis::UpdateObject(Media& obj)
{
    MediaItem mi = &obj;
    ClearFirstPlayProperty(mi);

    UpdateRedrawMenusFor(mi, true, menuStore); 
}

void MenuStoreOnDeleteVis::Visit(VideoMD& vd)
{
    ClearFirstPlayProperty(&vd);

    UpdateMenusFor(&vd, true);
    for( VideoMD::Itr itr = vd.List().begin(), end = vd.List().end(); itr != end; ++itr )
        UpdateMenusFor(*itr, true);
    RedrawMenus(menuStore);
}

RefPtr<MenuStore> CreateEmptyMenuStore()
{
    RefPtr<MenuStore> ms(new MenuStore);

    RegisterOnChange(new MenuStoreOnChangeVis(ms));
    RegisterOnDelete(new MenuStoreOnDeleteVis(ms));
    return ms;
}

void PublishMenuStore(RefPtr<MenuStore> ms, MenuList& ml)
{
    // * сначала откроем все из-за рекурсивности
    OpenMenus(ml);

    for( MenuList::Itr itr = ml.Beg(), end = ml.End(); itr != end; ++itr )
        PublishMenu(ms->append(), ms, *itr);
}

RefPtr<MenuStore> CreateMenuStore(MenuList& ml)
{
    RefPtr<MenuStore> ms = CreateEmptyMenuStore();
    PublishMenuStore(ms, ml);
    return ms;
}

void RenderMenuSystem(Menu changed_mn, RectListRgn& rct_lst)
{
    Project::MenuPack& mp = changed_mn->GetData<MenuPack>();
    GetRenderList(mp.thRgn).swap(rct_lst);
    // *
    EraseLinkedMenus(mp);
    // *
    InvokeOnChange(changed_mn);
}

} // namespace Project

struct CutByFrame
{
    Rect& framRct;
	  CutByFrame(Rect& fram_rct): framRct(fram_rct) {}

    void  operator()(Rect& rct)
	  {
	      rct = Intersection(rct, framRct);
	  }
};

bool ReDivideRects(RectListRgn& rct_lst, MenuRegion& menu_rgn)
{
    Rect fram_rct = menu_rgn.GetCanvasBuf().FrameRect(); //Rect0Sz(menu_rgn.FramePlacement().Size());
    std::for_each(rct_lst.begin(), rct_lst.end(), CutByFrame(fram_rct));

    ReDivideRects(rct_lst);
    return rct_lst.size() != 0;
}

