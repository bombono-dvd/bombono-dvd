//
// mgui/project/mb-actions.cpp
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

#include "mb-actions.h"
#include "handler.h"
#include "thumbnail.h"
#include "mconstructor.h" // APROJECT_NAME

#include <mbase/project/table.h>
#include <mgui/timeline/service.h>
#include <mgui/trackwindow.h>
#include <mgui/img-factory.h>
#include <mgui/dialog.h>
#include <mgui/sdk/window.h>
#include <mgui/gettext.h>

#include <gtk/gtktreestore.h>
#include <mlib/sdk/logger.h>
#include <mlib/filesystem.h>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <strings.h> // strcasecmp()

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
    RefPtr<Gdk::Pixbuf> thumb_pix = itr->get_value(ms->columns.thumbnail);
    if( !thumb_pix )
    {
        // * серый фон
        Point thumb_sz = Calc4_3Size(SMALL_THUMB_WDH);
        thumb_pix = CreatePixbuf(thumb_sz);
        thumb_pix->fill(RGBA::ToUint(Gdk::Color("light grey")));

        itr->set_value(ms->columns.thumbnail, thumb_pix);
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
    (*itr)[ms->columns.media] = mi;

    PublishMediaVis vis(itr, ms);
    mi->Accept(vis);
}

static std::string MarkError(const std::string& val, bool not_error)
{
    if( not_error )
        return val;
    return "<span foreground=\"red\">" + val + "</span>";
}

static std::string MarkBoolError(bool val, bool not_error)
{
    return MarkError(std::string(val ? _("yes") : _("no")), not_error);
}


namespace { 

struct ErrorDesc
{
           bool  res;
    std::string  outStr;
    std::string  descStr;
    
    ErrorDesc(): res(true) {}
};

static void SetImportError(ErrorDesc& ed, bool is_good, const std::string& out_str, 
                           const std::string& desc_str = std::string())
{
    if( !is_good && ed.res )
    {
        ed.res = false;
        ed.descStr = desc_str; // внимание на первую ошибку
    }

    #define M_SPS "\n   "
    ed.outStr += M_SPS + out_str;
    #undef M_SPS
}

static std::string FpsToStr(const Point& frate)
{
    return (str::stream() << (double)frate.x/frate.y).str();
}

static std::string TVTypeStr(bool is_ntsc)
{
    return std::string(is_ntsc ? "NTSC" : "PAL/SECAM");
}

void CheckVideoFormat(ErrorDesc& ed, const Mpeg::SequenceData& vid, bool is_ntsc)
{
    using namespace boost;
    //
    // Doc: DVD-Video/1. Mpucoder Specs/DVD/dvdmpeg.html
    // проверка подходимости для DVD
    // *
    int kbps = vid.bytRat/400;
    bool is_byte_rate_ok = kbps <= 9800; // Kbps
    SetImportError(ed, is_byte_rate_ok, 
                   std::string(_("Video bitrate")) + ":\t" + 
                   MarkError(lexical_cast<std::string>(kbps), is_byte_rate_ok) + " " + _("kbps"), 
                   _("Maximum data rate for video (9800 kbps) is exceeded."));

    const char* Descriptions[] = {
        N_("The %1% DVD-Video can accept MPEG-2 with resolutions: %2% only."),
        N_("The %1% DVD-Video can accept MPEG-2 with frame rate: %2% only."),
        N_("The %1% DVD-Video can accept MPEG-2 with aspects 4:3, 16:9 only.")
    };
    std::string tv_type = TVTypeStr(is_ntsc);


    // * (352x480, 352x288 - не по стандарту, см. Trac#9)
    Point ntsc_resolutions[] = { Point(720, 480), Point(704, 480), Point(352, 480), Point(352, 240) };
    Point pal_resolutions[]  = { Point(720, 576), Point(704, 576), Point(352, 576), Point(352, 288) };
    Point sz(vid.wdh, vid.hgt);
    bool sz_ok = false;
    std::string resol_list;
    Point* resolutions = is_ntsc ? ntsc_resolutions : pal_resolutions ;
    for( int i=0; i<(int)ARR_SIZE(ntsc_resolutions); i++ )
    {
        sz_ok = sz_ok || sz == resolutions[i];
        if( !resol_list.empty() )
            resol_list += ", ";
        resol_list += PointToStr(resolutions[i]);
    }   
    SetImportError(ed, sz_ok, 
                   std::string(_("Video size")) + ":   \t" + MarkError(PointToStr(sz), sz_ok),
                   BF_(Descriptions[0]) % tv_type % resol_list % bf::stop);

    // *
    Point frate(vid.framRat);
    // прогрессивность не учитываем, потому что на DVD Demystified(VTS_18_1.VOB) не сработало
    //bool is_progr = vid.isProgr;
    bool frate_ok = false;
    std::string frate_list;
    if( is_ntsc )
    {
        frate_ok = frate == Point(24, 1) || frate == Point(30000, 1001);
        frate_list = FpsToStr(Point(24, 1)) + ", " + FpsToStr(Point(30000, 1001));
    }
    else
    {
        frate_ok   = frate == Point(25, 1);
        frate_list = FpsToStr(Point(25, 1));
    }
    SetImportError(ed, frate_ok, 
                   std::string(_("Frame rate")) + ":   \t" + 
                   MarkError(FpsToStr(frate), frate_ok) + " " + _("fps"),
                   BF_(Descriptions[1]) % tv_type % frate_list % bf::stop);

    // *
    bool is_aspect_ok = vid.sarCode == af4_3 || vid.sarCode == af16_9;
    Point aspect = vid.SizeAspect();
    std::string aspect_str = (str::stream() << aspect.x << ':' << aspect.y).str();
    SetImportError(ed, is_aspect_ok, 
                   std::string(_("Aspect ratio")) + ": \t" + MarkError(aspect_str, is_aspect_ok),
                   BF_(Descriptions[2]) % tv_type % bf::stop);

    // * 
    bool no_delay = !vid.lowDelay;
    // Translators: Low delay is very tech term and can be left as is.
    SetImportError(ed, no_delay, "\"Low delay\":    \t" + MarkBoolError(!no_delay, no_delay));
    ed.outStr += "\n";
}

} // namespace 

bool CanOpenAsVideo(const char* fname, std::string& err_string, bool& must_be_video)
{
    Mpeg::PlayerData pd;
    io::stream& strm     = pd.srcStrm;
    Mpeg::MediaInfo& inf = pd.mInf;

    bool res = false;
    if( !Mpeg::GetInfo(pd, fname) )
        err_string = inf.ErrorReason();
    else
    {
        must_be_video = true; // дальше не проверяем тип - это точно видео
        Mpeg::SequenceData& vid = inf.vidSeq;
        ErrorDesc ed;
        // * видео
        bool is_ntsc = !AData().PalTvSystem();
        CheckVideoFormat(ed, vid, is_ntsc);
        bool video_check = ed.res;

        // * мультиплексирование в формате DVD
        bool is_dvd_mux   = false;
        bool is_nav_found = false;
        const int pack_cnt = 8; // первые 8*2048=16Kb проверяем
        if( inf.endPos >= pack_cnt*DVD_PACK_SZ )
        {
            uint8_t buf[DVD_PACK_SZ];
            strm.seekg(0);

            is_dvd_mux = true;
            for( int i=0; i<pack_cnt && strm.read((char*)buf, DVD_PACK_SZ); )
            {
                if( !(buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[2] && 0xba) )
                {
                    is_dvd_mux = false;    
                    break;
                }

                //
                // COPY_N_PASTE_ETALON из dvdvob.c, http://dvdauthor.sourceforge.net/
                // 
                // Замечание - из-за недоступности стандарта DVD-VIDEO для нас стандарт - dvdauthor!
                // Более того, судя по первому замечанию в TODO для версии 0.6.14, - размер system header
                // может быть не равен 18 байтам (но сути это не меняет - должны принимать только то, что
                // сможет принять dvdauthor)
                if( buf[14] == 0 &&
                    buf[15] == 0 &&
                    buf[16] == 1 &&
                    buf[17] == 0xbb ) // system header
                {
                    if( buf[38] == 0 && buf[39] == 0 && buf[40] == 1 && buf[41] == 0xbf && // 1st private2
                        buf[1024] == 0 && buf[1025] == 0 && buf[1026] == 1 && buf[1027] == 0xbf ) // 2nd private2
                        is_nav_found = true;
                }

        		// пропускаем спец. pad-данные самого dvdauthor - не считаются 
        		// (добавляются при деавторинге, добавлении субтитров)
                if ( buf[14] == 0 && buf[15] == 0 && buf[16] == 1 && buf[17] == 0xbe && 
                     strcmp((char*)buf+20,"dvdauthor-data") == 0 )
                    ;
                else
                    i++;
            }
        }
        std::string dvd_mux_desc = _("<b>Bombono DVD</b> can use \"DVD-ready\" video only now."
            " Use muxing programs like \"mplex -f 8\" (from <b>mjpegtools</b>)," 
            " mencoder (from <b>mplayer</b>) or <b>transcode</b> to make your video ready for <b>Bombono DVD</b>.");
        SetImportError(ed, is_dvd_mux,
                       boost::format("%1%:    \t") % _("DVD packs") % bf::stop + MarkBoolError(is_dvd_mux, is_dvd_mux), dvd_mux_desc);
        SetImportError(ed, is_nav_found, 
                       boost::format("%1%:  \t") % _("NAV packets") % bf::stop + MarkBoolError(is_nav_found, is_nav_found), dvd_mux_desc);
        bool dvd_check = is_dvd_mux && is_nav_found;

        res = ed.res;
        if( !res )
        {
            err_string  = _("This video may not be added due to (errors in <span foreground=\"red\">red color</span>):");
            err_string += "\n<tt>" + ed.outStr + "</tt>";

            std::string desc_str = ed.descStr;
            if( !video_check && dvd_check )
            {
                ErrorDesc ed2;
                CheckVideoFormat(ed2, vid, !is_ntsc);
                if( ed2.res )
                {
                    // подскажем пользователю, что он ошибся форматом проекта
                    desc_str = BF_("This video has %1% type and can't be added to"
                        " current project of %2% type. Create new project from"
                        " menu \"Project->New Project\" with right type.") % 
                        TVTypeStr(!is_ntsc) % TVTypeStr(is_ntsc) % bf::stop;
                }
            }

            if( !desc_str.empty() )
                err_string += "\n\n" + desc_str;
        }
    }

    return res;
}

inline bool CaseIEqual(const std::string &s1, const std::string &s2)
{
    return strcasecmp(s1.c_str(), s2.c_str()) == 0;
}

StorageItem CreateMedia(const char* fname, std::string& err_string)
{
    StorageItem md;

    // 1 общая проверка
    fs::path pth(fname);
    if( !fs::exists(pth) )
    {
    	err_string = _("File doesn't exist.");
    	return md;
    }
    if( fs::is_directory(pth) )
    {
    	err_string = _("Folders can't be added.");
    	return md;
    }

    //
    // 2 определение типа файла
    // Пока открываем только минимальное кол-во
    // типов медиа (картинки и MPEG2), потому простой
    // перебор
    // :TODO: переделать; с помощью библиотеки libmagick
    // (утилита '/usr/bin/file') реализовать первичное 
    // определение типа файла
    //

    std::string fext = get_extension(pth);
    bool must_be_video = CaseIEqual(fext, "mpeg") || CaseIEqual(fext, "mpg") || 
                         CaseIEqual(fext, "m2v")  || CaseIEqual(fext, "vob");
    int wdh, hgt;
    std::string video_err_str;
    if( CanOpenAsVideo(fname, video_err_str, must_be_video) )
    {
        // 2.1 видео
    	md = new VideoMD;
        md->MakeByPath(fname);
    }
    else if( !must_be_video && gdk_pixbuf_get_file_info(fname, &wdh, &hgt) )
    {
        // 2.2 картинка
        md = new StillImageMD;
        md->MakeByPath(fname);
    }

    if( !md )
    {
        // по расширению выводим наиболее вероятную ошибку
        err_string  = must_be_video ? video_err_str : _("Unknown file type.") ;
    }
    return md;
}

static void OnVideoView(TrackLayout& layout, VideoMD* vd, int chp_pos)
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

static ViewMediaVis::Fnr MakeViewFnr(VideoItem vi, int chp_pos)
{
    using namespace boost;
    TLFunctor a_fnr = lambda::bind(&OnVideoView, lambda::_1, vi.get(), chp_pos);
    return lambda::bind(&OpenTrackLayout, lambda::_1, vi, a_fnr);
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
    ViewMediaVis::Fnr fnr = ViewMediaVis::GetViewerFunctor(mi);
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

bool TryAddMedia(const char* fname, Gtk::TreePath& pth, std::string& err_str,
                 bool insert_after)
{
    bool res = false;
    if( StorageItem md = CreateMedia(fname, err_str) )
    {
        res = true;
        LOG_INF << "Insert Media!" << io::endl;

        RefPtr<MediaStore> ms = GetAStores().mdStore;
        Gtk::TreeIter itr = InsertByPos(ms, pth, insert_after);
        PublishMedia(itr, ms, md);
        InvokeOnInsert(md);

        pth = ms->get_path(itr);
    }
    return res;
}

static std::string StandFNameOut(const fs::path& pth)
{
    return "<span style=\"italic\" underline=\"low\">" + 
                    pth.leaf() + "</span>";
}

void TryAddMedias(const Str::List& paths, MediaBrowser& brw,
                  Gtk::TreePath& brw_pth, bool insert_after)
{
    // * подсказка с импортом
    if( paths.size() )
    {
        fs::path pth(paths[0]); 
        std::string leaf = pth.leaf();
        std::string::const_iterator start = leaf.begin(), end = leaf.end();
        static boost::regex dvd_video_vob("(VIDEO_TS|VTS_[0-9][0-9]_[0-9]).VOB", 
                                          boost::regex::perl|boost::regex::icase);

        if( boost::regex_match(start, end, dvd_video_vob) && 
            MessageBox(BF_("The file \"%1%\" looks like VOB from DVD.\nRun import?") % leaf % bf::stop,
                       Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL) == Gtk::RESPONSE_OK )
        {
            DVD::RunImport(*GetTopWindow(brw), pth.branch_path().string());
            return;
        }
    }

    // когда одна ошибка
    std::string err_str;
    fs::path err_pth;
    int err_cnt = 0;
    // куда переходим в браузере
    Gtk::TreePath goto_path;
    std::string err_desc;
    for( Str::List::const_iterator itr = paths.begin(), end = paths.end(); 
         itr != end; ++itr )
    {
        const std::string& fpath = *itr;
        std::string ConvertPathToUtf8(const std::string& path);
        fs::path pth = ConvertPathToUtf8(fpath);

        bool is_exist = false;
        // * проверяем, есть ли такой уже
        RefPtr<MediaStore> ms = brw.GetMediaStore();
        for( MediaStore::iterator itr = ms->children().begin(), end = ms->children().end();
             itr != end; ++itr )
        {
            StorageItem si = GetAsStorage(ms->GetMedia(itr));
            if( fs::equivalent(pth, si->GetPath()) )
            {
                // только переходим к нему
                brw.get_selection()->select(GetBrowserPath(si));
                is_exist = true;
                break;
            }
        }
        if( is_exist )
            continue;

        bool res = TryAddMedia(fpath.c_str(), brw_pth, err_str, insert_after);
        if( res )
        {
            insert_after = true; // вставляем друг за другом
            goto_path    = brw_pth;
        }
        else
        {
            err_pth = pth;
            err_cnt++;

            const int max_show_errors = 2+1;
            if( err_cnt < max_show_errors )
            {
                if( !err_desc.empty() )
                    err_desc += "\n\n";
                err_desc += StandFNameOut(pth);
                if( !err_str.empty() )
                    err_desc += "\n" + err_str;
            }
            else
            {
                err_desc += (err_cnt == max_show_errors) ? std::string("\n\n") + _("Also:") + " " : std::string(", ") ;
                err_desc += StandFNameOut(pth);
            }
        }
    }

    if( err_cnt )
    {
        std::string online_tip("\n\n");
        online_tip += BF_("See more about preparing video for authoring in <a href=\"%1%\">online help</a>.") % 
            "http://www.bombono.org/Preparing_sources_for_DVD" % bf::stop;

        // :KLUDGE: хотелось использовать ngettext() для того чтоб в PO строки были рядом,
        // однако msgfmt для требует чтобы в обоих вариантах присутствовало одинаковое 
        // кол-во заполнителей 
        //boost::format frmt(ngettext("Can't add file \"%1%\".", "Can't add files:", err_cnt));
        if( err_cnt == 1 )
            MessageBoxWeb(BF_("Can't add file \"%1%\".") % err_pth.leaf() % bf::stop, 
                          Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, err_str + online_tip);
        else
            MessageBoxWeb(_("Can't add files:"), Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, err_desc + online_tip);
    }

    if( !goto_path.empty() )
        GoToPos(brw, goto_path);
}

Str::List GetFilenames(Gtk::FileChooser& fc)
{
    // я знаю, что это жутко неэффективно (2+1 копирования), но не актуально
    typedef Glib::SListHandle<Glib::ustring> UList;
    UList ulist = fc.get_filenames();

    Str::List list;
    std::copy(ulist.begin(), ulist.end(), std::back_inserter(list));
    return list;
}

static void MediaBrowserAdd(MediaBrowser& brw, const char* /*fname*/,
                            Gtk::FileChooser& fc)
{
    Str::List paths(GetFilenames(fc));
    // * куда
    Gtk::TreePath brw_pth = GetCursor(brw);
    ValidateMediaInsertionPos(brw_pth);

    TryAddMedias(paths, brw, brw_pth, true);
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
        PackFileChooserWidget(fcw_hpaned, bl::bind(&MediaBrowserAdd, boost::ref(brw), bl::_1, bl::_2), false);

    Gtk::HPaned& hpaned = *Gtk::manage(new Gtk::HPaned);
    hpaned.set_position(BROWSER_WDH);
    fcw_hpaned.add2(hpaned);

    // *
    MediaActionFnr view_fnr = bl::bind(&ViewMedia, boost::ref(layout), bl::_1);
    PackMediaBrowserAll(PackAlignedForBrowserTB(hpaned), brw, open_fnr, 
                        bl::bind(&DeleteMediaFromBrowser, boost::ref(brw)),
                        bl::bind(&ExecuteForMedia, boost::ref(brw), view_fnr));
    brw.signal_row_activated().connect( 
       bl::bind(&OnBrowserRowActivated, boost::ref(brw), view_fnr, bl::_1) );

    // *
    hpaned.add2(PackMonitorIn(mon));
}

} // namespace Project

