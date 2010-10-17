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

#include <mgui/init.h>
#include <mgui/render/menu.h>
#include <mgui/editor/kit.h> // ClearLocalData()
#include <mgui/sdk/widget.h>
#include <mgui/sdk/textview.h>
#include <mgui/project/thumbnail.h> // Project::CalcAspectSize()
#include <mgui/gettext.h>

#include <mbase/project/table.h>
#include <mbase/resources.h>

#include <mdemux/util.h> // Mpeg::SecToHMS()

#include <mlib/filesystem.h>
#include <mlib/string.h>
#include <mlib/sdk/logger.h>

#include <boost/lexical_cast.hpp>

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

bool CheckAuthorMode::Flag = false;

CheckAuthorMode::CheckAuthorMode(bool turn_on): origVal(Flag) 
{ Flag = turn_on; }
CheckAuthorMode::~CheckAuthorMode()
{ Flag = origVal; }

// из-за того, что все медиа в одном браузере (включая картинки="не видео"),
// нельзя использовать естественную нумерацию для авторинга 
static int& GetAuthorNumber(VideoMD& obj)
{
    //return GetBrowserPath(&obj)[0]+1;
    return obj.GetData<int>(AUTHOR_TAG);
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

bool IndexForAuthoring(VideoItem vi, int idx)
{
    GetAuthorNumber(*vi) = idx+1; // индекс от 1
    return true;
}

static int IndexVideosForAuthoring()
{
    return ForeachVideo(bl::bind(&IndexForAuthoring, bl::_1, bl::_2));
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
        str = (str::stream() << "title " << GetAuthorNumber(*vi)).str();
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

void TargetCommandVis::Visit(VideoMD& obj)
{
    res = (str::stream() << "jump title " << GetAuthorNumber(obj) << ";").str(); 
}

void TargetCommandVis::Visit(VideoChapterMD& obj) 
{
    int v_num = GetAuthorNumber(*obj.owner);
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

static bool _GetFirstVideo(VideoItem& res, VideoItem vi, int)
{
    res = vi;
    return false;
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

bool HasButtonLink(Comp::MediaObj& m_obj, std::string& targ_str)
{
    bool res = false;
    if( MediaItem btn_target = m_obj.MediaItem() )
    {
        targ_str = MakeButtonJump(btn_target, false);
        res = !targ_str.empty();
    }
    return res;
}

int MenusCnt()
{
    return Size(GetAStores().mnStore);
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
        pgc_node->add_child("pre")->add_child_text(loop_menus);
    }

    xmlpp::Element* vob_node = pgc_node->add_child("vob");
    // название меню
    std::string m_dir = MenuAuthorDir(mn, i);
    vob_node->set_attribute("file", AppendPath(m_dir, "MenuSub.mpg"));
    if( IsMotion(mn) )
        // повторяем анимационное меню бесконечно
        pgc_node->add_child("post")->add_child_text("jump cell 1;");

    // действия кнопок
    MenuRegion::ArrType& lst = GetMenuRegion(mn).List();
    for( MenuRegion::Itr itr = lst.begin(), end = lst.end(); itr != end; ++itr )
        if( Comp::MediaObj* m_obj = dynamic_cast<Comp::MediaObj*>(*itr) )
        {
            std::string targ_str;    
            if( HasButtonLink(*m_obj, targ_str) )
            {
                xmlpp::Element* node = pgc_node->add_child("button");
                node->add_child_text(targ_str);
            }
        }
    return true;
}

typedef boost::function<std::string(VideoItem)> TitlePostCommand;

static std::string JumpNextTitle(VideoItem vi, int titles_cnt)
{
    int cur_num = GetAuthorNumber(*vi);
    ASSERT( (cur_num > 0) && (cur_num <= titles_cnt) );

    int next_num = (cur_num == titles_cnt) ? 1 : cur_num + 1;
    return "jump title " + boost::lexical_cast<std::string>(next_num) + ";";
}

static std::string AutoPostCmd(const std::string& jnt_cmd)
{
    return MenusCnt() ? std::string("call menu entry root;") : jnt_cmd ;
}

static bool ScriptTitle(xmlpp::Element* ts_node, VideoItem vi, int titles_cnt)
{
    xmlpp::Element* pgc_node = ts_node->add_child("pgc");
    xmlpp::Element* vob_node = pgc_node->add_child("vob");
    vob_node->set_attribute("file", GetFilename(*vi));
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

    xmlpp::Element* post_node = pgc_node->add_child("post");

    std::string jnt_cmd = JumpNextTitle(vi, titles_cnt);
    // чаще всего будет эта команда
    std::string post_cmd = AutoPostCmd(jnt_cmd);
    PostAction& pa = vi->PAction();
    switch( pa.paTyp )
    {
    case patAUTO:
        break;
    case patNEXT_TITLE:
        post_cmd = jnt_cmd;
        break;
    case patEXP_LINK:
        if( pa.paLink )
            post_cmd = MakeButtonJump(pa.paLink, true);
        break;
    default:
        //ASSERT_RTL(0);
        break; // не вылетаем на плохих проектах
    }
    post_node->add_child_text(post_cmd);

    return true;
}

static Menu GetFirstMenu()
{
    RefPtr<MenuStore> mn_store = GetAStores().mnStore;
    TSBegEnd menus_be          = BeginEnd(mn_store);
    return (menus_be.first != menus_be.second) ? GetMenu(mn_store, menus_be.first) : Menu();
}

VideoItem GetFirstVideo()
{
    VideoItem first_vi;
    ForeachVideo(bl::bind(&_GetFirstVideo, boost::ref(first_vi), bl::_1, bl::_2));
    return first_vi;
}

bool Is4_3(VideoItem first_vi)
{
    bool is_4_3 = true;
    if( first_vi )
    {
        Point asp = Project::CalcAspectSize(*first_vi);
        if( asp == Point(16, 9) )
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

void GenerateDVDAuthorScript(const std::string& out_dir)
{
    int titles_cnt = IndexVideosForAuthoring();
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
        xmlpp::Element* node = vmgm_node->add_child("fpc");
        node->add_child_text("jump menu entry title;");
        node = vmgm_node->add_child("menus");
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
        std::string init_cmd = "jump " + MakeFPTarget(fp) + ";";
        if( root_menu )
            // если есть меню (вообще есть), то инициализируем первое
            init_cmd = boost::format("g1 = %1%; %2%") % GetMenuANum(root_menu) % init_cmd % bf::stop;
        title_entry->add_child("pre")->add_child_text(init_cmd);
    }
    // основная часть
    {
        xmlpp::Element* tts_node   = root_node->add_child("titleset");
        // * меню
        xmlpp::Element* menus_node = tts_node->add_child("menus");
        AddVideoTag(menus_node, IsMenuToBe4_3());
        ForeachMenu(bl::bind(&ScriptMenu, menus_node, root_menu, bl::_1, bl::_2));
        // * список разделов (titles)
        xmlpp::Element* ts_node = tts_node->add_child("titles");
        AddVideoTag(ts_node, Is4_3(first_vi));

        ForeachVideo(bl::bind(&ScriptTitle, ts_node, bl::_1, titles_cnt));
    }
    doc.write_to_file_formatted(AppendPath(out_dir, "DVDAuthor.xml"));
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
        using namespace boost;
        if( CheckAuthorMode::Flag )
            ForeachMenu(lambda::bind(&RestrictGetCanvasBufMenu, lambda::_1, is_on));
    }
};

bool AuthorDVD(const std::string& out_dir)
{
    ASSERT( fs::is_directory(out_dir) );
    ASSERT( fs::is_empty_directory(out_dir) );
    ASSERT( HaveFullAccess(out_dir) );

    // для тестов авторинга - не даем пользоваться GetCanvasBuf(),
    // так как он для интерактива
    RestrictGetCanvasBuf rgcb;
    bool res = true;
    try
    {
        AuthorSectionInfo((str::stream() << "Build DVD-Video in folder: " << out_dir).str());
        IteratePendingEvents();

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
        if( !CheckAuthorMode::Flag )
        {
            if( es.mode != Author::modRENDERING )
            {
                Author::BuildDvdOF of;
                ExitData ed = Author::ExecuteSconsCmd(out_dir, of, es.mode, scons_options);
    
                if( !ed.IsGood() )
                    throw std::runtime_error(BF_("external command failure: %1%") % ExitDescription(ed) % bf::stop);
            }
        }
        else
        {
            ////int pid = gnome_execute_shell(out_dir.c_str(), "scons totem");
            //int pid = Spawn(out_dir.c_str(), "scons totem");
            //ASSERT(pid > 0);
        }
    }
    catch (const std::exception& err)
    {
        Author::GetES().exitDesc = err.what();
        Author::Info(std::string("ERR: ") + err.what(), false);
        Author::Info("Stop authoring!\n", false);
        res = false;
    }

    // очищаем все авторинговые данные
    using namespace boost;
    ForeachVideo(lambda::bind(&AuthorClearVideo, lambda::_1));
    ForeachMenu(lambda::bind(&AuthorClearMenu, lambda::_1));

    if( res )
        AuthorSectionInfo("Authoring is successfully done.");
    return res;
}

} // namespace Project

namespace Author {

static void AppendToLog(const std::string& txt)
{
    if( Project::CheckAuthorMode::Flag )
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

void Error(const std::string& str)
{
    throw std::runtime_error(str);
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
    if( !Project::CheckAuthorMode::Flag )
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

ExitData ExecuteSconsCmd(const std::string& out_dir, OutputFilter& of, 
                         Mode mod, const str::stream& scons_options)
{
    std::string cmd = "scons" + scons_options.str() + " " + SconsTarget(mod);
    return ExecuteAsync(out_dir.c_str(), cmd.c_str(), of, &GetES().eDat.pid);
}

} // namespace Author

