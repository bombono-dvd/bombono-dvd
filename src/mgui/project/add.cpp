//
// mgui/project/add.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

#include "add.h"

#include <mgui/ffviewer.h>
#include <mdemux/seek.h>

#include <mgui/init.h>
#include <mgui/dialog.h>
#include <mgui/sdk/window.h>
#include <mgui/gettext.h>

#include <mbase/project/handler.h>
#include <mbase/project/table.h>

#include <mlib/filesystem.h>

#include <mlib/regex.h>
#include <boost/lexical_cast.hpp>

#include <gtk/gtkversion.h>

#include <strings.h> // strcasecmp()

namespace Project
{

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
    int kbps = vid.bytRat/125; // *8/1000 : 1kbps = 1000bit/s
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

bool IsVideoDVDCompliant(const char* fname, std::string& err_string, bool& is_mpeg2)
{
    Mpeg::PlayerData pd;
    io::stream& strm     = pd.srcStrm;
    Mpeg::MediaInfo& inf = pd.mInf;

    bool res = false;
    if( !Mpeg::GetInfo(pd, fname) )
        err_string = inf.ErrorReason();
    else
    {
        is_mpeg2 = true; // дальше не проверяем тип - это точно видео
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

// определить тип файла и создать по нему соответствующее медиа
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

#define FFMPEG_IMPORT_POLICY 1
#ifdef FFMPEG_IMPORT_POLICY

    // Сейчас открытие с помощью CanOpenAsFFmpegVideo()/FFData считаем 
    // эталоном в плане принятия материалов; в частности:
    // - ошибка возвращается с темы, если не смогли открыть как изображение
    // - тема не должна открывать аудио или картинки, чтоб был правильный тип
    // Вроде как GdkPixbuf умел открывать видео (его первый кадр) и ради этого
    // был поставлен доп. заслон в виде must_be_video (сейчас не повторяется на
    // mpeg/m2v/..); если повторится, то улучшаем проверку (проверяем по кодеку 
    // у FFmpeg,- для картинок он "image2" не должен быть)
    int wdh, hgt;
    if( CanOpenAsFFmpegVideo(fname, err_string) )
        md = new VideoMD;
    else if( gdk_pixbuf_get_file_info(fname, &wdh, &hgt) )
        md = new StillImageMD;

    if( md )
        md->MakeByPath(fname);

#else // !FFMPEG_IMPORT_POLICY
    // 2 определение типа файла
    // Пока открываем только минимальное кол-во
    // типов медиа (картинки и MPEG2), потому простой
    // перебор
    // В идеале с помощью библиотеки libmagick можно
    // (утилита '/usr/bin/file') реализовать первичное 
    // определение типа файла, но это на практике не имеет большого смысла

    std::string fext = get_extension(pth);
    bool must_be_video = CaseIEqual(fext, "mpeg") || CaseIEqual(fext, "mpg") || 
                         CaseIEqual(fext, "m2v")  || CaseIEqual(fext, "vob");
    int wdh, hgt;
    std::string video_err_str;
    if( IsVideoDVDCompliant(fname, video_err_str, must_be_video) )
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
#endif // FFMPEG_IMPORT_POLICY

    return md;
}

// pth - куда вставлять; по выходу pth равен позиции вставленного
// insert_after - вставить после pth, по возможности
bool TryAddMedia(const char* fname, Gtk::TreePath& pth, std::string& err_str, 
                 bool insert_after = true)
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

// desc - метка происхождения, добавления
void TryAddMediaQuiet(const std::string& fname, const std::string& desc)
{
    std::string err_str;
    Gtk::TreePath pth;
    if( !TryAddMedia(fname.c_str(), pth, err_str) )
    {    
        LOG_ERR << "TryAddMediaQuiet error (" << desc << "): " << err_str << io::endl;
    }
}

static std::string StandFNameOut(const fs::path& pth)
{
    return "<span style=\"italic\" underline=\"low\">" + 
                    pth.leaf() + "</span>";
}

#if GTK_CHECK_VERSION(2,18,0)
#define LABEL_HAS_A_TAG
#endif

static void AddMediaError(const std::string& msg_str, const std::string& desc_str)
{
#ifdef LABEL_HAS_A_TAG
    MessageBoxWeb(msg_str, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, desc_str);
#else
    MessageBox(msg_str, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, desc_str);
#endif
}

void TryAddMedias(const Str::List& paths, MediaBrowser& brw,
                  Gtk::TreePath& brw_pth, bool insert_after)
{
    // * подсказка с импортом
    if( paths.size() )
    {
        const std::string fname = paths[0];
        fs::path pth(fname); 
        std::string leaf = pth.leaf();
        {
            static re::pattern dvd_video_vob("(VIDEO_TS|VTS_[0-9][0-9]_[0-9]).VOB", 
                                              re::pattern::perl|re::pattern::icase);
    
            if( re::match(leaf, dvd_video_vob) && 
                MessageBox(BF_("The file \"%1%\" looks like VOB from DVD.\nRun import?") % leaf % bf::stop,
                           Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL) == Gtk::RESPONSE_OK )
            {
                DVD::RunImport(*GetTopWindow(brw), pth.branch_path().string());
                return;
            }
        }

        std::string ext = get_extension(pth);
        const char* el_array[] = {"m2v", "mp2", "mpa", "ac3", "dts", "lpcm", 0 };
        bool res = false;
        for( const char** el = el_array; *el ; el++ )
            if( *el == ext )
            {
                res = true;
                break;
            }
        if( res && MessageBox(BF_("The file \"%1%\" looks like elementary stream and need to be muxed before using. Run muxing?") % leaf % bf::stop,
                              Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL) == Gtk::RESPONSE_OK )
        {
            MuxAddStreams(fname);
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
        bool one_error = (err_cnt == 1);
        std::string desc = one_error ? err_str : err_desc ;

#ifdef LABEL_HAS_A_TAG
        std::string online_tip("\n\n");
        online_tip += BF_("See more about preparing video for authoring in <a href=\"%1%\">online help</a>.") % 
            "http://www.bombono.org/Preparing_sources_for_DVD" % bf::stop;

        desc += online_tip;
#endif

        // :KLUDGE: хотелось использовать ngettext() для того чтоб в PO строки были рядом,
        // однако msgfmt для требует чтобы в обоих вариантах присутствовало одинаковое 
        // кол-во заполнителей 
        //boost::format frmt(ngettext("Can't add file \"%1%\".", "Can't add files:", err_cnt));
        if( one_error )
            AddMediaError(BF_("Can't add file \"%1%\".") % err_pth.leaf() % bf::stop, desc);
        else
            AddMediaError(_("Can't add files:"), desc);
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

void MediaBrowserAdd(MediaBrowser& brw, Gtk::FileChooser& fc)
{
    Str::List paths(GetFilenames(fc));
    // * куда
    Gtk::TreePath brw_pth = GetCursor(brw);
    ValidateMediaInsertionPos(brw_pth);

    TryAddMedias(paths, brw, brw_pth, true);
}

} // namespace Project

