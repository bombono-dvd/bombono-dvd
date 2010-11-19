//
// mgui/project/mb-actions.cpp
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

#include "mb-actions.h"
#include "add.h"

#include "handler.h"
#include "thumbnail.h"

#include <mgui/timeline/dvdmark.h>
#include <mgui/timeline/mviewer.h>
#include <mgui/trackwindow.h>
#include <mgui/img-factory.h>
#include <mgui/prefs.h>

#include <mbase/project/table.h>
#include <gtk/gtktreestore.h>

namespace Project
{

struct EmblemNameVis: public ObjVisitor
{
    std::string str;
    void  Visit(StillImageMD&)   { str = "emblems/stock_graphic-styles.png"; } //camera-photo.png"; }
    void  Visit(VideoMD&)        { str = "emblems/media-playback-start_mini.png"; } //video-x-generic.png"; }
    void  Visit(VideoChapterMD&) { str = "emblems/stock_about.png"; } //stock_navigator-open-toolbar.png"; }
    //void  Visit(AudioMD&)        { str = "emblems/audio-x-generic.png"; }

    static std::string Make(MediaItem mi)
    {
        EmblemNameVis vis;
        mi->Accept(vis);

        ASSERT( !vis.str.empty() );
        return vis.str;
    }
};

void FillThumbnail(const Gtk::TreeIter& itr, RefPtr<MediaStore> ms, Media& md)
{
    Gtk::TreeModelColumn<RefPtr<Gdk::Pixbuf> > thumb_cln = MediaStore::Fields().thumbnail;
    RefPtr<Gdk::Pixbuf> thumb_pix = itr->get_value(thumb_cln);
    if( !thumb_pix )
    {
        // * серый фон
        Point thumb_sz = Calc4_3Size(SMALL_THUMB_WDH);
        thumb_pix = CreatePixbuf(thumb_sz);
        thumb_pix->fill(RGBA::ToUint(Gdk::Color("light grey")));

        itr->set_value(thumb_cln, thumb_pix);
    }

    Point thumb_sz(PixbufSize(thumb_pix));
    // *
    RefPtr<Gdk::Pixbuf> cache_pix = GetCalcedShot(&md);
    RGBA::Scale(thumb_pix, cache_pix, FitIntoRect(thumb_sz, PixbufSize(cache_pix)));

    // * эмблемы
    StampEmblem(thumb_pix, EmblemNameVis::Make(&md));
    StampFPEmblem(&md, thumb_pix);
    //row[ms->columns.thumbnail] = thumb_pix;
    ms->row_changed(ms->get_path(itr), itr);
}

void FillThumbnail(RefPtr<MediaStore> ms, StorageItem si)
{
    FillThumbnail(ms->get_iter(GetBrowserPath(si)), ms, *si.get());
}

class PublishMediaVis: public ObjVisitor
{
    public:
                    PublishMediaVis(const Gtk::TreeIter& itr_, RefPtr<MediaStore> ms_)
                        : lctItr(itr_), ms(ms_) {}

              void  Visit(StillImageMD& obj);
              void  Visit(VideoMD& obj);

    protected:
          const Gtk::TreeIter& lctItr;
            RefPtr<MediaStore> ms;
};

void PublishMediaVis::Visit(StillImageMD& /*obj*/)
{
    ReindexFrom(ms, lctItr);
}

void PublishMediaVis::Visit(VideoMD& obj)
{
    ReindexFrom(ms, lctItr);

    for( VideoMD::Itr itr = obj.List().begin(), end = obj.List().end(); itr != end ; ++itr )
        PublishMedia(ms->append(lctItr->children()), ms, *itr);
}

void PublishMedia(const Gtk::TreeIter& itr, RefPtr<MediaStore> ms, MediaItem mi)
{
    FillThumbnail(itr, ms, *mi);
    (*itr)[MediaStore::Fields().media] = mi;

    PublishMediaVis vis(itr, ms);
    mi->Accept(vis);
}

class ViewMediaVis: public ObjVisitor
{
    public:
            BoolTLFunctor  vFnr;

    protected:

            void  Visit(VideoMD& obj);
            void  Visit(VideoChapterMD& obj);
};

BoolTLFunctor GetViewerFunctor(MediaItem mi)
{
    ViewMediaVis vis;
    mi->Accept(vis);
    return vis.vFnr;
}

static bool ViewVideo(TrackLayout& layout, Project::VideoItem vd, int chp_pos)
{
    std::string err_str;
    bool res = OpenTrackLayout(layout, vd, err_str);
    if( res )
    {
        using namespace Timeline;
        // при первом открытии файла синхронизируем позиции глав
        bool& opened_before = vd->GetData<bool>("VideoOpenedBefore");
        if( !opened_before )
        {
            opened_before = true;
            VideoMD::ListType lst = vd->List();
            for( VideoMD::Itr itr = lst.begin(), end = lst.end(); itr != end; ++itr )
            {
                ChapterItem ci = *itr;
                GetMarkData(ci).pos = TimeToFrames(ci->chpTime, layout.FrameFPS());
            }
        }
        if( chp_pos >= 0 )
            layout.SetPos(GetMarkData(chp_pos).pos);
    }
    return res;
}

static BoolTLFunctor MakeViewFnr(VideoItem vi, int chp_pos)
{
    return bb::bind(&ViewVideo, _1, vi, chp_pos);
}

void ViewMediaVis::Visit(VideoMD& obj)
{
    vFnr = MakeViewFnr(&obj, -1);
}

void ViewMediaVis::Visit(VideoChapterMD& obj)
{
    int chp_pos = ChapterPosInt(&obj);
    vFnr = MakeViewFnr(obj.owner, chp_pos);
}

void ViewMedia(TrackLayout& layout, MediaItem mi)
{
    BoolTLFunctor fnr = GetViewerFunctor(mi);
    if( fnr && fnr(layout) )
        GrabFocus(layout);
}

GtkTreeIter* GetGtkTreeIter(const Gtk::TreeIter& itr)
{
    return const_cast<GtkTreeIter*>(itr.get_gobject_if_not_end());
}

// улучшенный Gtk::TreeStore::move() - по сути равен std::rotate()
void MoveRow(RefPtr<Gtk::TreeStore> ts, const Gtk::TreePath& src, const Gtk::TreePath& dst)
{
    GtkTreeIter* src_itr = GetGtkTreeIter(ts->get_iter(src));
    GtkTreeIter* dst_itr = GetGtkTreeIter(ts->get_iter(dst));

    if( src.back() < dst.back() )
        gtk_tree_store_move_after(ts->gobj(), src_itr, dst_itr);
    else
        gtk_tree_store_move_before(ts->gobj(), src_itr, dst_itr);
}

Gtk::TreePath GetChapterPath(VideoChapterMD& obj)
{
    Gtk::TreePath path = GetBrowserPath(obj.owner);
    ASSERT( !path.empty() );
    path.push_back(ChapterPosInt(&obj));

    return path;
}

//
// обработчики браузера
// 

class MediaStoreVis: public ObjVisitor
{
    public:
                        MediaStoreVis(RefPtr<MediaStore> ms): mdStore(ms) {}
    protected:
            RefPtr<MediaStore> mdStore;
};

class MediaStoreOnChangeVis: public MediaStoreVis
{
    public:
                        MediaStoreOnChangeVis(RefPtr<MediaStore> ms): MediaStoreVis(ms) {}
     virtual      void  Visit(VideoChapterMD& obj);
};

class MediaStoreOnDeleteVis: public MediaStoreVis
{
    public:
                        MediaStoreOnDeleteVis(RefPtr<MediaStore> ms): MediaStoreVis(ms) {}
     virtual      void  Visit(VideoChapterMD& obj);
};

class MediaStoreOnInsertVis: public MediaStoreVis
{
    public:
                        MediaStoreOnInsertVis(RefPtr<MediaStore> ms): MediaStoreVis(ms) {}
     virtual      void  Visit(VideoChapterMD& obj);
};

void MediaStoreOnChangeVis::Visit(VideoChapterMD& obj)
{
    Gtk::TreePath path = GetChapterPath(obj);
    int old_pos = obj.GetData<int>("ChapterMoveIndex");

    // * если индекс поменялся, то браузере позицию соответ. поменять
    if( path.back() != old_pos )
    {
        Gtk::TreePath old_path(path);
        old_path.back() = old_pos;
        MoveRow(mdStore, old_path, path);
    }

    // * обновляем миниатюру
    FillThumbnail(mdStore->get_iter(path), mdStore, obj);
}

void MediaStoreOnDeleteVis::Visit(VideoChapterMD& obj)
{
    if( !GetBrowserDeletionSign(&obj) )
    {
        Gtk::TreePath path = GetChapterPath(obj);
        mdStore->erase(mdStore->get_iter(path));
    }
}

void MediaStoreOnInsertVis::Visit(VideoChapterMD& obj)
{
    Gtk::TreeIter chp_itr;
    if( (int)GetList(&obj).size() == ChapterPosInt(&obj)+1 ) // вставили главу в самый конец
    {
        Gtk::TreePath path = GetBrowserPath(obj.owner);
        chp_itr = mdStore->append(mdStore->get_iter(path)->children());
    }
    else
    {
        chp_itr = mdStore->get_iter(GetChapterPath(obj));
        chp_itr = mdStore->insert(chp_itr);
    }

    ASSERT( chp_itr );
    PublishMedia(chp_itr, mdStore, &obj);
}

void RegisterMSHandlers(RefPtr<MediaStore> ms)
{
    RegisterOnInsert(new MediaStoreOnInsertVis(ms));
    RegisterOnChange(new MediaStoreOnChangeVis(ms));
    RegisterOnDelete(new MediaStoreOnDeleteVis(ms));
}

void PublishMediaStore(RefPtr<MediaStore> ms)
{
    MediaList& ml = AData().GetML();

    for( MediaList::Itr itr = ml.Beg(), end = ml.End(); itr != end; ++itr )
        PublishMedia(ms->append(), ms, *itr);
}

void OnBrowserRowActivated(MediaBrowser& brw, MediaActionFnr fnr, const Gtk::TreeModel::Path& path)
{
    RefPtr<MediaStore> ms = brw.GetMediaStore();
    Gtk::TreeIter itr     = ms->get_iter(path);
    fnr(ms->GetMedia(itr), itr);
}

void PackMBWindow(Gtk::HPaned& fcw_hpaned, Timeline::DAMonitor& mon, TrackLayout& layout, 
                  MediaBrowser& brw)
{
    ActionFunctor open_fnr = 
        PackFileChooserWidget(fcw_hpaned, bb::bind(&MediaBrowserAdd, boost::ref(brw), _2), false);

    Gtk::HPaned& hpaned = *Gtk::manage(new Gtk::HPaned);
    SetUpdatePos(hpaned, UnnamedPrefs().mdBrw1Wdh);
    fcw_hpaned.add2(hpaned);

    // *
    MediaActionFnr view_fnr = bb::bind(&ViewMedia, boost::ref(layout), _1);
    PackMediaBrowserAll(PackAlignedForBrowserTB(hpaned), brw, open_fnr, 
                        bb::bind(&DeleteMediaFromBrowser, boost::ref(brw)),
                        bb::bind(&ExecuteForMedia, boost::ref(brw), view_fnr));
    brw.signal_row_activated().connect( 
       bb::bind(&OnBrowserRowActivated, boost::ref(brw), view_fnr, _1) );

    // *
    hpaned.add2(PackMonitorIn(mon));
}

} // namespace Project

