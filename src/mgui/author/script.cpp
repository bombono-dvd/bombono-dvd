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
#include <mgui/project/handler.h>
#include <mgui/prefs.h>
#include <mgui/gettext.h>

#include <mbase/project/table.h>
#include <mbase/resources.h>

#include <mdemux/util.h> // Mpeg::SecToHMS()

#include <mlib/filesystem.h>
#include <mlib/string.h>
#include <mlib/sdk/logger.h>
#include <mlib/regex.h>

namespace Project 
{

static void AddFormatAttr(xmlpp::Element* node)
{
    node->set_attribute("format", IsPALProject() ? "pal" : "ntsc");
}

static xmlpp::Element* AddVideoTag(xmlpp::Element* node, bool is_4_3 = true)
{
    xmlpp::Element* vnode = node->add_child("video");
    AddFormatAttr(vnode);
    vnode->set_attribute("aspect", is_4_3 ? "4:3" : "16:9");
    if( !is_4_3 )
        // в режиме 16:9 ресурсы (36) тратятся на 3 режима: основной (widescreen),
        // letterbox и pan&scan; последний дефакто не используется + кол-во
        // кнопок для меню увеличиваем до 16; см. также
        // http://forum.videohelp.com/threads/298871-More-than-12-buttons-with-dvdauthor
        // :TRICKY: нужно для меню, для видео факт неочевиден
        vnode->set_attribute("widescreen", "nopanscan");

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
int& GetAuthorNumber(VideoItem vi)
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
    VideoMD* owner = obj.owner;
    int v_num = GetAuthorNumber(owner);
    // title 1 всегда равно title 1 chapter 1 (первая ячейка всегда создавется), и максим. кол-во адресуемых 
    // номеров <= кол-во bmd-глав+1; если 2 главы слишком близки к друг другу (внутри одного vobu=область между
    // двумя NAV-пакетами, ~0.5 секунды), то dvdauthor их "склеивает" в одну => кол-во номеров уменьшается
    // Потому: для удоства пользователей даем создавать нулевую главу, разрешая это здесь 
    // (однако доп. нулевые главы будут приводить к ошибке Cannot jump to chapter N ... only M exist)
    int c_num = ChapterPosInt(&obj) + (owner->List()[0]->chpTime ? 2 : 1) ;
    res = (str::stream() << "jump title " << v_num << " chapter " << c_num << ";").str();
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

bool HasButtonLink(Comp::MediaObj& m_obj)
{
    std::string targ_str;
    return Project::HasButtonLink(m_obj, targ_str);
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
    boost_foreach( Comp::MediaObj* m_obj, Author::AllMediaObjs(mn) )
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

static void AddSubpicStream(xmlpp::Element* sub_node, const char* id, const char* mode)
{
    xmlpp::Element* strm_node = sub_node->add_child("stream");
    strm_node->set_attribute("id", id);
    strm_node->set_attribute("mode", mode);
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
        if( !IsMenuToBe4_3() )
        {
            // Jim Taylor, DVD Demustified - для нормального отображения меню 16:9
            // нужны отдельные субтитры для widescreen и letterbox
            xmlpp::Element* sub_node  = menus_node->add_child("subpicture");
            AddSubpicStream(sub_node, "0", "widescreen");
            AddSubpicStream(sub_node, "1", "letterbox");
        }
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
    ClearTaggedData(mn, AUTHOR_LB_TAG);
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

// ffmpeg выводит статистику первого создаваемого файла каждые полсекунды,
// см. print_report() (при verbose=1, по умолчанию)
// Формат размера: "size=%8.0fkB"
re::pattern FFmpegSizePat( "size= *"RG_NUM"kB"); 
// Формат длительности: "time=%0.2f"
re::pattern FFmpegDurPat( "time="RG_FLT);

typedef boost::function<double(double)> PercentFunctor;

struct Job
{
        int  coeff;
    io::pos  transDone;
    io::pos  transVal;
        int  outIdx;
        
       GPid  pid;
  ptr::shared<OutErrBlock> oeb;
         sigc::connection  watchConn;
        
    Job(int coeff_, int out_idx, io::pos trans_val)
        : coeff(coeff_), transDone(), outIdx(out_idx), transVal(trans_val), pid(NO_HNDL)
    { ASSERT( coeff > 0 ); }
};

typedef std::list<Job> JobList;

struct JobData
{
        JobList  jLst;
            int  todoIdx;
    
        io::pos  transDone;
        io::pos  transTotal;
    std::string  outDir;
       ExitData  lastED;
    std::string  exception; // проброс исключений из gtk_main()
           
    JobData(): todoIdx(0), transDone(0), transTotal(0) {}
};

static double CalcTransPercent(double cur_dur, Job& job, JobData& jd, double full_dur)
{
    //return (std::min(sz * 1024, trans_val) + trans_done)/double(trans_total);
    //return (std::min(cur_dur/full_dur, 1.) * trans_val + trans_done)/double(trans_total);
    job.transDone = std::min(cur_dur/full_dur, 1.) * job.transVal;
    double res = 0.;
    boost_foreach( Job& job, jd.jLst )
        res += job.transDone;
    return (res + jd.transDone)/double(jd.transTotal);
}

static void OnTranscodePrintParse(const char* dat, int sz, const PercentFunctor& fnr)
{
    re::match_results what;
    // лучше вычислять не по выданному размеру, а по времени, так как итоговый размер
    // вычисляется по битрейту (и обычно завышен для страховки), а длительность практически не
    // меняется
    //if( re::search(std::string(dat, sz), what, FFmpegSizePat) )
    //{
    //    // :KLUDGE: вроде и нецелые могут быть, но precision=0
    //    // как-то не в тему
    //    // У пользователей случалось вообще не число, потому гасим возможные исключения
    //    io::pos sz;
    //    if( Str::GetType(sz, what.str(1).c_str()) )
    //    {
    //        double per = fnr(sz);
    //        Author::SetStageProgress(per);
    //    }
    //}
    if( re::search(std::string(dat, sz), what, FFmpegDurPat) )
    {
        double dur;
        if( ExtractDouble(dur, what) )
        {
            double per = fnr(dur);
            Author::SetStageProgress(per);
        }
    }
}

static void ToCout(const char* dat, int sz, const std::string& prefix)
{
    io::cout << prefix << std::string(dat, sz);
}

ReadReadyFnr DetailsAppender(const std::string& print_cmd, const ReadReadyFnr& add_fnr,
                             const std::string& prefix)
{
    return TextViewAppender(PrintCmdToDetails(print_cmd), add_fnr, prefix);
}

static void OnJobEnd(GPid pid, int status, JobData& jd);
static bool UpdateJobs(JobData& jd)
{
    int max_wl = Prefs().maxCPUWorkload;
    // в промежутке [1, max]
    max_wl = std::min(std::max(1, max_wl), MaxCPUWorkload());
    
    JobList& jl = jd.jLst;
    int cnt = boost::distance(AllTransVideos());
    // отрисовываем префикс, чтоб разделять выводы; исходя из
    // приоритета паралелльного выполнения над многопоточностью,
    // таково условие нужности префикса
    bool multiple_out = (max_wl > 1) && (cnt > 1);
        
    int wl = 0;
    boost_foreach( Job& job, jl )
        wl += job.coeff;
    // пока применяем простую практику - максимальную нагрузку не могут снизить в процессе
    wl = max_wl - wl;
    ASSERT( wl >= 0 );

    int new_cnt = std::min(wl, cnt-jd.todoIdx);
    if( new_cnt > 0 )
    {
        int div = wl / new_cnt;
        int rest  = wl % new_cnt;
        for( int i=0; i<new_cnt; i++, rest-- )
        {
            // мин. неиспользуемое значение
            int out_idx = 0;
            for( ; ; out_idx++ )
            {
                bool idx_found = true;
                boost_foreach( Job& job, jl )
                    if( job.outIdx == out_idx )
                    {
                        idx_found = false;
                        break;
                    }
                if( idx_found )
                    break;
            }
            
            static bool no_ffmpeg_threads = false;
            static RPData nft_rp;
            if( ReadPref("no_ffmpeg_threads", nft_rp) )
                no_ffmpeg_threads = PrefToBool(nft_rp.val);
            
            int coeff = rest > 0 ? div+1 : div;
            if( no_ffmpeg_threads )
                coeff = 1;
            VideoItem vi = AllTransVideos().advance_begin(jd.todoIdx+i).front();

            std::string src_fname = SrcFilename(vi);
            std::string dst_fname = DVDCompliantFilename(vi, jd.outDir);
            // :TRICKY: отдельные видеофайлы подгоняем под DVD-формат исходя из них самих
            // (а не под одну "гребенку" проекта), несмотря на то, что запись в один VTS видео
            // разных форматов вроде как противоречит спекам. Причина: вроде и так работает (на
            // доступных мне железных и софтовых плейерах), а если будут проблемы - лучше мультиформат
            // реализовать
            RTCache& rtc = GetRTC(vi);
            AutoDVDTransData atd(Is4_3(vi));
            atd.asd = rtc.asd;
            atd.asd.audioNum = OutAudioNum(atd.asd);
            atd.threadsCnt = coeff;

            DVDTransData td = GetRealTransData(vi);
            td.ctmFFOpt = vi->transDat.ctmFFOpt;

            std::string ffmpeg_cmd = FFmpegToDVDTranscode(src_fname, dst_fname, atd, IsPALProject(), td);
            jl.push_back(Job(coeff, out_idx, CalcTransSize(vi, td.vRate)));
            Job& new_job = jl.back();

            PercentFunctor fnr = bb::bind(&CalcTransPercent, _1, b::ref(new_job), b::ref(jd), rtc.duration);
            ReadReadyFnr rr_fnr = bb::bind(&OnTranscodePrintParse, _1, _2, fnr);

            int out_err[2];
            GPid pid = Spawn(0, ffmpeg_cmd.c_str(), out_err, true);
            new_job.pid = pid;
            
            std::string prefix;
            if( multiple_out )
                prefix = boost::format("%1%: ") % out_idx % bf::stop;
            ReadReadyFnr oeb_fnr;
            std::string print_ffcmd = prefix + ffmpeg_cmd;
            if( Execution::ConsoleMode::Flag )
            {
                io::cout << print_ffcmd << io::endl;
                oeb_fnr = bb::bind(&ToCout, _1, _2, prefix);
            }
            else
                oeb_fnr = DetailsAppender(print_ffcmd, rr_fnr, prefix);
            new_job.oeb = new OutErrBlock(out_err, oeb_fnr);

            new_job.watchConn = Glib::signal_child_watch().connect(bb::bind(&OnJobEnd, _1, _2, b::ref(jd)), pid);
        }
        jd.todoIdx += new_cnt;
    }
    
    return !jl.empty();
}

// если создавали процесс с признаком ожидания, то нужен StopAndWait(), даже
// если результат процесса не нужен:
// - Posix: одного Stop() недостаточно,- waitpid()/освобождение ресурсов обязательно;
//   :TODO: как прекратить сделить, если уже не нужно ничего от процесса
// - Win32: освобождение хэндла процесса
void StopAndWait(GPid pid)
{
    Execution::Stop(pid);
    WaitForExit(pid);
}

// :TRICKY: отвественная функция - халатность недостима
// (пропуск исключений наверх)
static void StopJobPool(JobData& jd) throw()
{
    JobList& jl = jd.jLst;
    // закрываем остальные, неважно что работающие
    boost_foreach( Job& job, jl )
    {
        job.watchConn.disconnect();
        StopAndWait(job.pid);
    }
    jl.clear();
    
    Gtk::Main::quit();
}

static void OnJobEnd(GPid pid, int status, JobData& jd)
{
    try
    {
        bool is_found = false;
        JobList& jl = jd.jLst;
        for( JobList::iterator it = jl.begin(), end = jl.end(); it != end; ++it )
        {
            Job& job = *it;
            if( job.pid == pid )
            {
                jd.transDone += job.transVal;
                jl.erase(it);
                is_found = true;
                break;
            }
        }
        ASSERT_RTL( is_found );

        jd.lastED = CloseProcData(pid, status);

        if( !jd.lastED.IsGood() )
            StopJobPool(jd);
        else if( !UpdateJobs(jd) )
            Gtk::Main::quit();
    }
    catch(const std::exception& exc )
    {
        // приходится пробрасывать исключение(я), так как GTK(C) не
        // не пропускает их через себя
        jd.exception = exc.what();
        StopJobPool(jd);
    }
}
    
static void AuthorImpl(const std::string& out_dir)
{
    AuthorSectionInfo((str::stream() << "Build DVD-Video in folder: " << out_dir).str());
    IteratePendingEvents();

    IndexVideosForAuthoring();
    Author::ExecState& es = Author::GetES();

    // * транскодирование
    //io::pos trans_done = 0, trans_total = ProjectStat().transSum;
    //boost_foreach( VideoItem vi, AllTransVideos() )
    //{
    //    std::string src_fname = SrcFilename(vi);
    //    std::string dst_fname = DVDCompliantFilename(vi, out_dir);
    //    // :TRICKY: отдельные видеофайлы подгоняем под DVD-формат исходя из них самих
    //    // (а не под одну "гребенку" проекта), несмотря на то, что запись в один VTS видео
    //    // разных форматов вроде как противоречит спекам. Причина: вроде и так работает (на
    //    // доступных мне железных и софтовых плейерах), а если будут проблемы - лучше мультиформат
    //    // реализовать
    //    RTCache& rtc = GetRTC(vi);
    //    AutoDVDTransData atd(Is4_3(vi));
    //    atd.audioNum  = OutAudioNum(rtc.audioNum);
    //    atd.srcAspect = rtc.dar;
    //
    //    DVDTransData td = GetRealTransData(vi);
    //    td.ctmFFOpt = vi->transDat.ctmFFOpt;
    //
    //    std::string ffmpeg_cmd = FFmpegToDVDTranscode(src_fname, dst_fname, atd, IsPALProject(), td);
    //    io::pos trans_val  = CalcTransSize(vi, td.vRate);
    //    PercentFunctor fnr = bb::bind(&CalcTransPercent, _1, trans_total, trans_done,
    //                                  trans_val, rtc.duration);
    //    RunFFmpegCmd(ffmpeg_cmd, bb::bind(&OnTranscodePrintParse, _1, _2, fnr));
    //
    //    trans_done += trans_val;
    //}
    Author::SetStage(Author::stTRANSCODE);
    {
        JobData jd;
        jd.transTotal = ProjectStat().transSum;
        jd.outDir     = out_dir;
        
        if( UpdateJobs(jd) )
        {
            ActionFunctor& stop_fnr = es.eDat.stopFnr;
            stop_fnr = bb::bind(&StopJobPool, b::ref(jd));
            Gtk::Main::run();
            stop_fnr.clear();
            
            if( jd.exception.size() )
                Author::Error(jd.exception);
            Author::CheckAbortByUser();
            Author::CheckAppED(jd.lastED, "ffmpeg");
        }
    }

    // * субтитры
    boost_foreach( VideoItem vi, AllVideos() )
        if( IsToAddSubtitles(vi) )
        {
            xmlpp::Document doc;
            xmlpp::Element* sp = doc.create_root_node("subpictures");
            // определяем версию, потому что:
            // 1) с 0.7.0 необходим доп. атрибут "format"
            // 2) парсер dvdauthor не любит незнакомые ему атрибуты => spumux < 0.7 не работает
            std::string help_str;
            PipeOutput("spumux -h", help_str);
            static re::pattern spumux_version("DVDAuthor::spumux, version "RG_NUM"\\."RG_NUM"\\."RG_NUM"\\.\n");
            if( IsVersionGE(FindVersion(help_str, spumux_version, "spumux"), TripleVersion(0, 7, 0)) )
                AddFormatAttr(sp);

            xmlpp::Element* ts = sp->add_child("stream")->add_child("textsub");

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
            DVDDims dd = RequireVideoTC(vi) ? GetRealTransData(vi).dd : CalcDimsAuto(vi);
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
            RunExtCmd(spumux_cmd, "spumux");
        }

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

std::string DiscLabel()
{
    return GetBD().Label().get_text();
}

void FillSconsOptions(str::stream& scons_options, bool fill_def)
{
    std::string def_dvd_label, def_drive;
    double def_speed = 0;
    if( !Execution::ConsoleMode::Flag )
    {     
        def_dvd_label = DiscLabel();
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

void CheckAppED(const ExitData& ed, const char* app_name)
{
    if( !ed.IsGood() )
        Author::ApplicationError(app_name, ed);
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

void ApplicationError(const char* app_name, const std::string& reason)
{
    Author::Error(BF_("%1% failure: %2%") % app_name % reason % bf::stop);
}

void ApplicationError(const char* app_name, const ExitData& ed)
{
    ApplicationError(app_name, ExitDescription(ed));
}

void ExecuteSconsCmd(const std::string& out_dir, OutputFilter& of, 
                     Mode mod, const str::stream& scons_options)
{
    std::string cmd = "scons" + scons_options.str() + " " + SconsTarget(mod);
    ExitData ed = AsyncCall(out_dir.c_str(), cmd.c_str(), OF2RRF(of));
    if( of.firstError.size() )
        Error(of.firstError);
    if( !ed.IsGood() )
        //ApplicationError("", ed);
        Error(BF_("external command failure: %1%") % ExitDescription(ed) % bf::stop);
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

