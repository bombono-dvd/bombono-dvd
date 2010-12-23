//
// mgui/author/script.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009-2010 Ilya Murav'jov
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

#include "script.h"
#include "burn.h"
#include "ffmpeg.h"

#include <mgui/init.h>
#include <mgui/render/menu.h>
#include <mgui/editor/kit.h> // ClearLocalData()
#include <mgui/sdk/widget.h>
#include <mgui/sdk/textview.h>
#include <mgui/project/thumbnail.h> // Project::CalcAspectSize()
#include <mgui/project/video.h>
#include <mgui/gettext.h>

#include <mbase/project/table.h>
#include <mbase/resources.h>

#include <mdemux/util.h> // Mpeg::SecToHMS()

#include <mlib/filesystem.h>
#include <mlib/string.h>
#include <mlib/sdk/logger.h>

namespace Project 
{

static xmlpp::Element* AddVideoTag(xmlpp::Element* node, bool is_4_3 = true)
{
    ADatabase& db = AData();
    xmlpp::Element* vnode = node->add_child("video");
    vnode->set_attribute("format", db.PalTvSystem() ? "pal" : "ntsc");
    //Point aspect = db.GetDefMP().DisplayAspect();
    //vnode->set_attribute("aspect", (str::stream() << aspect.x << ":" << aspect.y).str());
    vnode->set_attribute("aspect", is_4_3 ? "4:3" : "16:9");

    return vnode;
}

typedef std::pair<Gtk::TreeIter, Gtk::TreeIter> TSBegEnd;
TSBegEnd BeginEnd(RefPtr<Gtk::TreeStore> store)
{
    Gtk::TreeModel::Children children = store->children();
    return std::make_pair(children.begin(), children.end());
}

// из-за того, что все медиа в одном браузере (включая картинки="не видео"),
// нельзя использовать естественную нумерацию для авторинга 
static int& GetAuthorNumber(VideoItem vi)
{
    //return GetBrowserPath(&obj)[0]+1;
    return vi->GetData<int>(AUTHOR_TAG);
}

int ForeachVideo(VideoFnr fnr)
{
    RefPtr<MediaStore> md_store = GetAStores().mdStore;
    TSBegEnd medias_be          = BeginEnd(md_store);
    int i = 0;
    for( Gtk::TreeIter itr = medias_be.first; itr != medias_be.second; ++itr )
        if( VideoItem vi = IsVideo(md_store->GetMedia(itr)) )
        {
            if( !fnr(vi, i) )
                break;
            i++;
        }
    return i;
}

typedef boost::function<bool(Menu, int)> MenuFnr;
void ForeachMenu(MenuFnr fnr)
{
    RefPtr<MenuStore> mn_store = GetAStores().mnStore;
    TSBegEnd menu_be           = BeginEnd(mn_store);
    int i = 0;
    for( Gtk::TreeIter itr = menu_be.first; itr != menu_be.second; ++itr )
    {
        Menu mn = GetMenu(mn_store, itr);
        if( !fnr(mn, i) )
            break;
        i++;
    }
}

static std::string MakeFPTarget(MediaItem mi)
{
    // Doc: "DVD-Video/2. The Unofficial DVD Specifications Guide/1. Guide - DVD2.2.5.pdf",
    // п. 3.2
    // Разные команды используются для перехода к меню и к разделу(title):
    // - для меню используется нумерация VTS - куда переходить (см. команду JumpSS);
    // - для раздела используется сквозная нумерация разделов на всем пространстве (см. команду JumpTT);
    //   в нашем случае (один VTS) нумерации для разделов совпадают
    std::string str;
    if( IsMenu(mi) )
        str = "titleset 1 menu entry root";
    else
    {
        VideoItem vi = IsVideo(mi);
        ASSERT( vi );
        str = (str::stream() << "title " << GetAuthorNumber(vi)).str();
    }
    return str;
}

class TargetCommandVis: public ObjVisitor
{
    public:
    std::string  res;
           bool  vtsDomain;
                
                    TargetCommandVis(bool is_vts): vtsDomain(is_vts) {}
            
    virtual   void  Visit(MenuMD& obj);
    virtual   void  Visit(VideoMD& obj);
    virtual   void  Visit(VideoChapterMD&);

};

static int GetMenuANum(Menu mn)
{
    return LocalPath(mn.get())[0]+1;
}

void TargetCommandVis::Visit(MenuMD& obj)
{
    res = boost::format("g1 = %1%; %2% menu entry root;") % GetMenuANum(&obj) % (vtsDomain ? "call" : "jump") % bf::stop; 
}

static std::string JumpTitleCmd(int num)
{
    return boost::format("jump title %1%;") % num % bf::stop; 
}

void TargetCommandVis::Visit(VideoMD& obj)
{
    res = JumpTitleCmd(GetAuthorNumber(&obj)); 
}

void TargetCommandVis::Visit(VideoChapterMD& obj) 
{
    int v_num = GetAuthorNumber(obj.owner);
    // :TODO: title 1 всегда равно title 1 chapter 1; при этом dvdauthor не воспринимает 
    // главы "вблизи нуля" (<0.27секунд; задание - посмотреть точно), поэтому нужна предварительная
    // перенумерация, как с видео
    res = (str::stream() << "jump title " << v_num << " chapter " << ChapterPosInt(&obj) + 2 << ";").str();
}

static std::string MakeButtonJump(MediaItem mi, bool vts_domain)
{
    TargetCommandVis vis(vts_domain);
    mi->Accept(vis);
    return vis.res;
}

std::string MenuAuthorDir(Menu mn, int idx, bool cnv_from_utf8)
{
    // нужно безопасное имя для директории
#ifdef BOOST_WINDOWS
    char sep = '\\';
#else
    char sep = '/';
#endif
    std::string name = mn->mdName;
    for( int i=0; i<(int)name.size(); i++ )
        if( name[i] == sep )
            name[i] = '_';
    if( !fs::native(name) )
        name = "Menu";

    std::string fname = (str::stream() << idx+1 << "." << name).str();
    return cnv_from_utf8 ? ConvertPathFromUtf8(fname) : fname ;
}

int TitlesCount()
{
    return boost::distance(AllVideos());
}

static std::string PlayAllCmd()
{
    if( TitlesCount() == 0 )
        Author::Error("There is no video for \"Play All\" function.");
    return boost::format("g2 = 1; %1%") % JumpTitleCmd(1) % bf::stop;
}

bool HasButtonLink(Comp::MediaObj& m_obj, std::string& targ_str)
{
    bool res = true;

    if( m_obj.PlayAll() )
        targ_str = PlayAllCmd();
    else if( MediaItem btn_target = m_obj.MediaItem() )
    {
        targ_str = MakeButtonJump(btn_target, false);
        res = !targ_str.empty();
    }
    else
        res = false;
    return res;
}

int MenusCnt()
{
    return Size(GetAStores().mnStore);
}

static bool IsEndVideo(int cur_num)
{
    return cur_num == TitlesCount();
}

static std::string JumpNextTitle(VideoItem vi)
{
    int cur_num = GetAuthorNumber(vi);
    ASSERT( (cur_num > 0) && (cur_num <= TitlesCount()) );

    int next_num = IsEndVideo(cur_num) ? 1 : cur_num + 1;
    return JumpTitleCmd(next_num);
}

static std::string AutoPostCmd(const std::string& jnt_cmd)
{
    return MenusCnt() ? std::string("call menu entry root;") : jnt_cmd ;
}

static void AddChildWithText(xmlpp::Element* node, const char* node_name,
                             const std::string& body)
{
    node->add_child(node_name)->add_child_text(body);
}

static void AddPostCmd(xmlpp::Element* pgc_node, MediaItem mi)
{
    VideoItem vi = IsVideo(mi);
    Menu mn      = IsMenu(mi);

    ASSERT_RTL( vi || mn );
    // VTS domain
    bool is_video = vi;
    const PostAction& pa = is_video ? vi->PAction() : mn->MtnData().pAct ;

    std::string jnt_cmd;
    // авто-команда
    std::string auto_cmd;
    if( is_video )
    {
        jnt_cmd = JumpNextTitle(vi);
        // чаще всего будет эта команда
        auto_cmd = AutoPostCmd(jnt_cmd);
    }
    else
        auto_cmd = "jump cell 1;"; // Loop

    std::string post_cmd;
    switch( pa.paTyp )
    {
    case patAUTO:
        post_cmd = auto_cmd;
        break;
    case patNEXT_TITLE:
        ASSERT( is_video );
        post_cmd = jnt_cmd;
        break;
    case patEXP_LINK:
        if( pa.paLink )
            post_cmd = MakeButtonJump(pa.paLink, is_video);
        break;
    case patPLAY_ALL:
        post_cmd = PlayAllCmd();
        break;
    default:
        //ASSERT_RTL(0);
        break; // не вылетаем на плохих проектах
    }
    if( post_cmd.empty() ) // пока "Stop" нет, и непонятно, как лучше его сделать
        post_cmd = auto_cmd;

    if( is_video )
    {
        // реализация Play All
        std::string prefix("g2 = 0;");
        if( !IsEndVideo(GetAuthorNumber(vi)) )
            prefix = boost::format("if(g2 == 1) {%1%}") % jnt_cmd % bf::stop;

        post_cmd = prefix + " " + post_cmd;
    }

    AddChildWithText(pgc_node, "post", post_cmd);
}

static bool ScriptMenu(xmlpp::Element* menus_node, Menu root_menu, Menu mn, int i)
{
    xmlpp::Element* pgc_node = menus_node->add_child("pgc");
    if( root_menu == mn )
        pgc_node->set_attribute("entry", "root");

    // <pre>
    {
        int num = GetMenuANum(mn);
        int next_num = (i+1 < MenusCnt()) ? num+1 : 1 ;
        std::string loop_menus = boost::format("if(g1 != %1%) {jump menu %2%;}") % num % next_num % bf::stop;
        AddChildWithText(pgc_node, "pre", loop_menus);
    }

    xmlpp::Element* vob_node = pgc_node->add_child("vob");
    // название меню
    std::string m_dir = MenuAuthorDir(mn, i);
    vob_node->set_attribute("file", AppendPath(m_dir, "MenuSub.mpg"));
    if( IsMotion(mn) )
        AddPostCmd(pgc_node, mn);

    // действия кнопок
    MenuRegion::ArrType& lst = GetMenuRegion(mn).List();
    for( MenuRegion::Itr itr = lst.begin(), end = lst.end(); itr != end; ++itr )
        if( Comp::MediaObj* m_obj = dynamic_cast<Comp::MediaObj*>(*itr) )
        {
            std::string targ_str;    
            if( HasButtonLink(*m_obj, targ_str) )
                AddChildWithText(pgc_node, "button", targ_str);
        }
    return true;
}

static std::string SrcFilename(VideoItem vi)
{
    return GetFilename(*vi);
}

static std::string PrefixCnvPath(VideoItem vi, const std::string& out_dir)
{
    std::string dst_fname = boost::format("%1%.%2%") % GetAuthorNumber(vi) 
        % fs::path(SrcFilename(vi)).leaf() % bf::stop;
    return AppendPath(out_dir, dst_fname);
}

// путь для скрипта dvdauthor
static std::string DVDCompliantFilename(VideoItem vi, const std::string& out_dir)
{
    std::string dst_fname = SrcFilename(vi);
    if( RequireTranscoding(vi) )
        dst_fname = PrefixCnvPath(vi, out_dir) + ".mpg";
    return dst_fname;
}

static bool IsToAddSubtitles(VideoItem vi)
{
    return !vi->subDat.pth.empty();
}

static std::string DVDFilename(VideoItem vi, const std::string& out_dir)
{
    std::string dst_fname = DVDCompliantFilename(vi, out_dir);
    if( IsToAddSubtitles(vi) )
        dst_fname = PrefixCnvPath(vi, out_dir) + ".sub.mpg";
    return dst_fname;
}

static bool ScriptTitle(xmlpp::Element* ts_node, VideoItem vi, const std::string& out_dir)
{
    xmlpp::Element* pgc_node = ts_node->add_child("pgc");
    if( IsToAddSubtitles(vi) )
    {
        int sub_num = vi->subDat.defShow ? 64 : 0 ;
        AddChildWithText(pgc_node, "pre", boost::format("subtitle = %1%;") % sub_num % bf::stop);
    }

    xmlpp::Element* vob_node = pgc_node->add_child("vob");
    vob_node->set_attribute("file", DVDFilename(vi, out_dir));
    // список глав
    bool is_empty = true;
    std::string chapters;
    for( VideoMD::Itr itr = vi->List().begin(), end = vi->List().end(); itr != end; ++itr )
    {
        if( is_empty )
            is_empty = false;
        else
            chapters += ", ";

        chapters += Mpeg::SecToHMS((*itr)->chpTime);
    }
    if( !is_empty )
        vob_node->set_attribute("chapters", chapters);

    AddPostCmd(pgc_node, vi);
    return true;
}

static Menu GetFirstMenu()
{
    RefPtr<MenuStore> mn_store = GetAStores().mnStore;
    TSBegEnd menus_be          = BeginEnd(mn_store);
    return (menus_be.first != menus_be.second) ? GetMenu(mn_store, menus_be.first) : Menu();
}

//static bool _GetFirstVideo(VideoItem& res, VideoItem vi, int)
//{
//    res = vi;
//    return false;
//}

VideoItem GetFirstVideo()
{
    VideoItem first_vi;
    //ForeachVideo(bb::bind(&_GetFirstVideo, boost::ref(first_vi), _1, _2));
    boost_foreach( VideoItem vi, AllVideos() )
    {
        first_vi = vi;
        break;
    }
    return first_vi;
}

bool Is4_3(VideoItem first_vi)
{
    bool is_4_3 = true;
    if( first_vi )
    {
        Point asp = Project::CalcAspectSize(*first_vi);
        // с погрешностью 0.1
        if( (double)asp.x/asp.y > (4 + 0.1)/3. )
            is_4_3 = false;
    }
    return is_4_3;
}

static bool Is4_3(MenuParams& prms)
{
    return prms.GetAF() != af16_9;
}

// ограничение: глобальная настройка для всех меню
bool IsMenuToBe4_3()
{
    bool is_menu_4_3 = Is4_3(AData().GetDefMP()); // по умолчанию, если все пусто
    if( Menu mn = GetFirstMenu() )
        is_menu_4_3 = Is4_3(mn->Params());
    else if( VideoItem f_vi = GetFirstVideo() )
        is_menu_4_3 = Is4_3(f_vi);
    return is_menu_4_3;
}

//bool IndexForAuthoring(VideoItem vi, int idx)
//{
//    GetAuthorNumber(vi) = idx+1; // индекс от 1
//    return true;
//}

// 2 цели:
// - в первую очередь, для скрипта dvdauthor
// - для нумерации при транскодировании
void IndexVideosForAuthoring()
{
    int idx = 1; // индекс от 1
    boost_foreach( VideoItem vi, AllVideos() )
        GetAuthorNumber(vi) = idx++;
}

void GenerateDVDAuthorScript(const std::string& out_dir)
{
    // индексируем видео в titles
    //IndexVideosForAuthoring();
    //ForeachVideo(bb::bind(&IndexForAuthoring, _1, _2));
    //IndexVideosForAuthoring();

    ADatabase& db = AData();
    AStores&   as = GetAStores();

    RefPtr<MenuStore> mn_store = as.mnStore;
    TSBegEnd menus_be          = BeginEnd(mn_store);

    MediaItem fp   = db.FirstPlayItem();
    Menu root_menu = GetFirstMenu();
    VideoItem first_vi = GetFirstVideo();

    if( !fp )
    {
        // первое же меню или видео
        fp = root_menu ? (MediaItem)root_menu : (MediaItem)first_vi ;

        if( fp )
        {
            std::string fp_name = fp->TypeString() + " \"" + fp->mdName + "\"";
            Author::Warning("There is no First-Play media. Assume it is " + fp_name + ".");
        }
        else
            Author::Error(_("There is no media (video or menu)."));
    }
    else // fp != 0
    {
        if( Menu mn = IsMenu(fp) )
            root_menu = mn;
    }
    ASSERT( fp );

    // запись dvdauthor-скрипта
    xmlpp::Document doc;
    xmlpp::Element* root_node = doc.create_root_node("dvdauthor");
    // глобальная секция
    {
        xmlpp::Element* vmgm_node = root_node->add_child("vmgm");
        vmgm_node->add_child_comment("First Play");
        AddChildWithText(vmgm_node, "fpc", "jump menu entry title;");

        xmlpp::Element* node = vmgm_node->add_child("menus");
        // из-за того, что в VMG мы не устанавливаем ни одно видео, то
        // надо явно инициализировать атрибуты (см. функцию BuildAVInfo()
        // в dvdauthor):
        // 1) должна быть хоть одна pgc в VMG - тогда BuildAVInfo() вызовется;
        //    см. ниже пустую "title"-pgc;
        // 2) атрибут format = ntsc/pal
        // 3) обязательно ставим явно resolution, иначе BuildAVInfo() запишет
        //    мусор (f..ff = -1) во все атрибуты.
        AddVideoTag(node)->set_attribute("resolution", "720xfull");
        // :KLUDGE: зачем нужно?
        node->add_child_comment("copy-n-paste?");
        xmlpp::Element* sub_node = node->add_child("subpicture");
        sub_node->set_attribute("lang", "EN");

        // кнопка "Title" = кнопка "Menu"
        xmlpp::Element* title_entry = node->add_child("pgc");
        title_entry->set_attribute("entry", "title");
        // g1 - номер текущего меню
        // g2 - признак Play All
        std::string init_cmd = "g2 = 0; jump " + MakeFPTarget(fp) + ";";
        if( root_menu )
            // если есть меню (вообще есть), то инициализируем первое
            init_cmd = boost::format("g1 = %1%; %2%") % GetMenuANum(root_menu) % init_cmd % bf::stop;
        AddChildWithText(title_entry, "pre", init_cmd);
    }
    // основная часть
    {
        xmlpp::Element* tts_node   = root_node->add_child("titleset");
        // * меню
        xmlpp::Element* menus_node = tts_node->add_child("menus");
        AddVideoTag(menus_node, IsMenuToBe4_3());
        ForeachMenu(bb::bind(&ScriptMenu, menus_node, root_menu, _1, _2));
        // * список разделов (titles)
        xmlpp::Element* ts_node = tts_node->add_child("titles");
        AddVideoTag(ts_node, Is4_3(first_vi));

        ForeachVideo(bb::bind(&ScriptTitle, ts_node, _1, out_dir));
    }
    SaveFormattedUTF8Xml(doc, AppendPath(out_dir, "DVDAuthor.xml"));
}

static bool AuthorClearVideo(VideoItem vi)
{
    vi->Clear(AUTHOR_TAG);
    return true;
}

void ClearTaggedData(Menu mn, const char* tag)
{
    mn->Clear(tag);
    Editor::ClearLocalData(GetMenuRegion(mn), tag);
}

static bool AuthorClearMenu(Menu mn)
{
    ClearTaggedData(mn, AUTHOR_TAG);
    return true;
}

static void CopyRootFile(const std::string& fname, const std::string& out_dir)
{
    fs::copy_file(SConsAuxDir()/fname, fs::path(out_dir)/fname);
}

void AuthorSectionInfo(const std::string& str)
{
    Author::Info("\n#", false);
    Author::Info((str::stream() << "# " << str).str(), false);
    Author::Info("#\n", false);
}

fs::path SConsAuxDir()
{
    return fs::path(GetDataDir())/"scons_authoring";
}

static bool RestrictGetCanvasBufMenu(Menu mn, bool is_on)
{
    MenuRegion& m_rgn = GetMenuRegion(mn);
    CanvasBuf*& cnv = mn->GetData<CanvasBuf*>("RestrictGetCanvasBuf");

    ASSERT( bool(cnv) != is_on );
    if( is_on )
    {
        cnv = &m_rgn.GetCanvasBuf();
        m_rgn.SetCanvasBuf(0);
    }
    else
    {
        m_rgn.SetCanvasBuf(cnv);
        mn->Clear("RestrictGetCanvasBuf");
    }
    return true;
}

struct RestrictGetCanvasBuf
{
    RestrictGetCanvasBuf() { Do(true); }
   ~RestrictGetCanvasBuf() { Do(false); }

    void Do(bool is_on)
    {
        if( Execution::ConsoleMode::Flag )
            ForeachMenu(bb::bind(&RestrictGetCanvasBufMenu, _1, is_on));
    }
};

static void CheckHomeSpumuxFont()
{
    char* home_dir = getenv("HOME");
    if( !home_dir )
        Error("Cannot find HOME directory!");

    fs::path font_path = fs::path(home_dir)/"/.spumux/FreeSans.ttf";
    if( !fs::exists(font_path) )
    {
        std::string err_str;
        if( !CreateDirs(font_path.branch_path(), err_str) )
            Error(err_str.c_str());
        fs::copy_file(DataDirPath("copy-n-paste/FreeSans.ttf"), font_path);
    }
}

std::string FFmpegToDVDTranscode(const std::string& src_fname, const std::string& dst_fname,
                                 const AutoDVDTransData& atd, bool is_pal, const DVDTransData& td)
{
    return boost::format("ffmpeg -i %1% %2%") % FilenameForCmd(src_fname) 
        % FFmpegToDVDArgs(dst_fname, atd, is_pal, td) % bf::stop;
}

static void AuthorImpl(const std::string& out_dir)
{
    AuthorSectionInfo((str::stream() << "Build DVD-Video in folder: " << out_dir).str());
    IteratePendingEvents();

    IndexVideosForAuthoring();

    void RunExtCmd(const std::string& cmd);
    // * транскодирование
    Author::SetStage(Author::stTRANSCODE);
    boost_foreach( VideoItem vi, AllTransVideos() )
    {
        CheckFFDVDEncoding();
        std::string src_fname = SrcFilename(vi);
        std::string dst_fname = DVDCompliantFilename(vi, out_dir);
        // :TRICKY: отдельные видеофайлы подгоняем под DVD-формат исходя из них самих
        // (а не под одну "гребенку" проекта), несмотря на то, что запись в один VTS видео
        // разных форматов вроде как противоречит спекам. Причина: вроде и так работает (на
        // доступных мне железных и софтовых плейерах), а если будут проблемы - лучше мультиформат
        // реализовать
        RTCache& rtc = GetRTC(vi);
        AutoDVDTransData atd(Is4_3(vi));
        atd.audioNum  = OutAudioNum(rtc.audioNum);
        atd.srcAspect = rtc.dar;
        std::string ffmpeg_cmd = FFmpegToDVDTranscode(src_fname, dst_fname, atd, IsPALProject(), 
                                                      GetRealTransData(vi));
        RunExtCmd(ffmpeg_cmd);
    }

    // * субтитры
    boost_foreach( VideoItem vi, AllVideos() )
        if( IsToAddSubtitles(vi) )
        {
            xmlpp::Document doc;
            xmlpp::Element* ts = doc.create_root_node("subpictures")->add_child("stream")->add_child("textsub");

            SubtitleData& dat = vi->subDat;
            ts->set_attribute("filename", dat.pth);
            ts->set_attribute("font", "FreeSans.ttf");
            //ts->set_attribute("force", "no");

            // :KLUDGE:
            // софтовые плейеры Xine, Vlc и Totem хотят размеров, соответ. 
            // самому видео (Totem вообще падает иначе), а вот мой железячный плейер 
            // не любит ничего кроме 720xfull и 28.0,- 
            // решил в пользу софтовых ради удобства разработки, но лучше бы
            // разобраться (хорошо, что не 720xfull - редкость)
            DVDDims dd = RequireTranscoding(vi) ? GetRealTransData(vi).dd : CalcDimsAuto(vi);
            // чуть меньше делаем размеры
            Point movie_sz = DVDDimension(dd) - Point(2, 2);
            ts->set_attribute("movie-width",  Int2Str(movie_sz.x));
            ts->set_attribute("movie-height", Int2Str(movie_sz.y));
            double font_sz = 28.;
            switch( dd )
            {
            case dvd352s:
                font_sz = 14.;
                break;
            case dvd352:
                font_sz = 21.;
                break;
            default:
                break;
            }
            ts->set_attribute("fontsize", Double2Str(font_sz));

            if( !dat.encoding.empty() )
                ts->set_attribute("characterset", dat.encoding);

            ts->set_attribute("horizontal-alignment", "center");
            ts->set_attribute("vertical-alignment",   "bottom");

            std::string xml_fname = PrefixCnvPath(vi, out_dir) + ".textsub.xml";
            SaveFormattedUTF8Xml(doc, xml_fname);
            
            CheckHomeSpumuxFont();
            std::string spumux_cmd = boost::format("spumux -m dvd %1% < %2% > %3%") 
                % FilenameForCmd(xml_fname) % FilenameForCmd(DVDCompliantFilename(vi, out_dir))
                % FilenameForCmd(DVDFilename(vi, out_dir)) % bf::stop;
            RunExtCmd(spumux_cmd);
        }

    Author::ExecState& es = Author::GetES();
    str::stream& settings = es.settings;
    settings.str("# coding: utf-8\n\n");
    // * рендерим меню и их скрипты
    AuthorMenus(out_dir);

    // *
    GenerateDVDAuthorScript(out_dir);

    // * вспомогательные файлы
    const char* fnames[] = { "SConstruct", "ADVD.py", "SConsTwin.py", "README" };
    for( int i=0; i<(int)ARR_SIZE(fnames); i++ )
        CopyRootFile(fnames[i], out_dir);
    // опции
    str::stream scons_options;
    Author::FillSconsOptions(scons_options, true);
    // ASettings.py
    io::stream settings_strm(AppendPath(out_dir, "ASettings.py").c_str(), iof::out);
    settings_strm << settings.rdbuf();
    settings_strm.close();

    // *
    if( !Execution::ConsoleMode::Flag )
    {
        if( es.mode != Author::modRENDERING )
        {
            Author::BuildDvdOF of;
            Author::ExecuteSconsCmd(out_dir, of, es.mode, scons_options);
        }
    }
    else
    {
        ////int pid = gnome_execute_shell(out_dir.c_str(), "scons totem");
        //int pid = Spawn(out_dir.c_str(), "scons totem");
    }
}

std::string AuthorDVD(const std::string& out_dir)
{
    ASSERT( fs::is_directory(out_dir) );
    ASSERT( fs::is_empty_directory(out_dir) );
    ASSERT( HaveFullAccess(out_dir) );

    // для тестов авторинга - не даем пользоваться GetCanvasBuf(),
    // так как он для интерактива
    RestrictGetCanvasBuf rgcb;
    std::string res = Author::SafeCall(bb::bind(&AuthorImpl, out_dir));

    // очищаем все авторинговые данные
    ForeachVideo(bb::bind(&AuthorClearVideo, _1));
    ForeachMenu(bb::bind(&AuthorClearMenu, _1));

    if( Author::IsGood(res) )
        AuthorSectionInfo("Authoring is successfully done.");
    else
    {
        Author::Info(std::string("ERR: ") + res, false);
        Author::Info("Stop authoring!\n", false);
    }
    return res;
}

} // namespace Project

namespace Author {

static void AppendToLog(const std::string& txt)
{
    if( Execution::ConsoleMode::Flag )
        LOG_INF << txt << io::endl;
    else
        AppendText(GetES().detailsView, txt + "\n");
}

void Warning(const std::string& str)
{
    AppendToLog("WARN: " + str);
}

void Info(const std::string& str, bool add_info_sign)
{
    AppendToLog((add_info_sign ? "INFO: " : "") + str);
}

std::string SconsTarget(Mode mode)
{
    std::string res;
    switch( mode )
    {
    case modFOLDER:     
        res = "";
        break;
    case modDISK_IMAGE: 
        res = "dvd.iso";
        break;
    case modBURN:       
        res = "burn";
        break;
    default:
        ASSERT_RTL(0);
    }
    return res;
}

// for_cmd - для командной строки/для скрипта ASettings.py
static void AddSconsOptions(str::stream& strm, bool for_cmd, const std::string& dvd_label, 
                            const std::string& dvd_drive, double dvd_speed)
{
    // :TODO: не знаю как передавать символы ' и ", и в командную строку,
    // и в скрипт
    std::string clean_label;
    for( int i=0; i<(int)dvd_label.size(); i++ )
        clean_label += (dvd_label[i] != '\'') ? dvd_label[i] : '`';

    char sep = for_cmd ? ' ' : '\n' ;
    if( for_cmd )
        strm << sep;
    strm << "DVDLabel='" << clean_label << "'" << sep;
    strm << "DVDDrive='" << dvd_drive   << "'" << sep;
    strm << "DVDSpeed="  << dvd_speed;
    if( !for_cmd )
        strm << sep << sep;
}

void FillSconsOptions(str::stream& scons_options, bool fill_def)
{
    std::string def_dvd_label, def_drive;
    double def_speed = 0;
    if( !Execution::ConsoleMode::Flag )
    {     
        BurnData& bd = GetBD();
        def_dvd_label = bd.Label().get_text();
        if( IsBurnerSetup(def_drive) )
            def_speed = GetBurnerSpeed();
    }
    if( fill_def )
        AddSconsOptions(GetES().settings, false, def_dvd_label, def_drive, def_speed);
    AddSconsOptions(scons_options, true, def_dvd_label, def_drive, def_speed);
}

void CheckAbortByUser()
{
    if( Author::GetES().eDat.userAbort )
        throw std::runtime_error("User Abortion"); // строка реально не нужна - сработает userAbort
}

ExitData AsyncCall(const char* dir, const char* cmd, const ReadReadyFnr& fnr)
{
    ExitData ed = ExecuteAsync(dir, cmd, fnr, &GetES().eDat.pid);
    CheckAbortByUser();

    return ed;
}

ReadReadyFnr OF2RRF(OutputFilter& of)
{
    return bb::bind(&Author::OutputFilter::OnGetLine, &of, _1, _2, _3);
}

void Error(const std::string& str)
{
    throw std::runtime_error(str);
}

void Error(const std::string& msg, const std::string& reason)
{
    Error(boost::format(msg) % reason % bf::stop);
}

void ErrorByED(const std::string& msg, const ExitData& ed)
{
    Error(msg, ExitDescription(ed));
}

void ExecuteSconsCmd(const std::string& out_dir, OutputFilter& of, 
                     Mode mod, const str::stream& scons_options)
{
    std::string cmd = "scons" + scons_options.str() + " " + SconsTarget(mod);
    ExitData ed = AsyncCall(out_dir.c_str(), cmd.c_str(), OF2RRF(of));
    if( !ed.IsGood() )
        ErrorByED(_("external command failure: %1%"), ed);
}

std::string SafeCall(const ActionFunctor& fnr)
{
    std::string result;
    try
    {
        fnr();
    }
    catch (const std::exception& err)
    {
        result = err.what();
        ASSERT( !result.empty() );
    }
    return result;
}

} // namespace Author

