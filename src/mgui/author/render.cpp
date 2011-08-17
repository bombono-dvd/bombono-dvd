//
// mgui/author/render.cpp
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
#include "ffmpeg.h"

#include <mgui/render/editor.h> // CommonRenderVis
#include <mgui/project/menu-render.h>
#include <mgui/project/thumbnail.h> // Project::PrimaryShotGetter
#include <mgui/project/video.h>
#include <mgui/editor/text.h>   // EdtTextRenderer
#include <mgui/editor/bind.h>   // MBind::TextRendering
#include <mgui/redivide.h>
#include <mgui/sdk/browser.h>   // VideoStart
#include <mgui/sdk/ioblock.h>
#include <mgui/sdk/textview.h>  // AppendCommandText()
#include <mgui/ffviewer.h>

#include <mbase/project/table.h>
#include <mbase/project/theme.h> // FindThemePath()

#include <mlib/filesystem.h>
#include <mlib/read_stream.h> // ReadAllStream()
#include <mlib/sdk/system.h> // GetClockTime()
#include <mlib/gettext.h>

#include <boost/lexical_cast.hpp>

void IteratePendingEvents()
{
    while( Gtk::Main::instance()->events_pending() )
        Gtk::Main::instance()->iteration();
}

namespace Project 
{

struct ShiftPCB: public PixCanvasBuf
{
    Point shift;
    
 virtual RefPtr<Gdk::Pixbuf> Canvas();
};

RefPtr<Gdk::Pixbuf> ShiftPCB::Canvas()
{
    RefPtr<Gdk::Pixbuf> res;
    if( canvPix )
    {
        Point sz = PixbufSize(canvPix);
        Rect rct(shift, sz);
        ASSERT( rct.IsValid() );
        res = MakeSubPixbuf(canvPix, Rect(shift, sz));
    }
    return res;
}

static const char* SubPicTag(bool not_letterbox16_9)
{
    return not_letterbox16_9 ? AUTHOR_TAG : AUTHOR_LB_TAG;
}

PixCanvasBuf& GetTaggedPCB(Menu mn, const char* tag)
{
    bool is_lb_tag = strcmp(AUTHOR_LB_TAG, tag) == 0;

    PixCanvasBuf& pcb = !is_lb_tag ? mn->GetData<PixCanvasBuf>(tag) : mn->GetData<ShiftPCB>(tag) ;
    if( !pcb.Canvas() )
    {
        Point sz(mn->Params().Size());
        Rect plc(Rect0Sz(sz));
        if( is_lb_tag )
        {
            // сжимаем 4:3 до 16:9 = *3/4
            plc.btm = Round(3./4. * sz.y);
            static_cast<ShiftPCB&>(pcb).shift = Point(0, (sz.y - plc.btm)/2);
        }
        pcb.Set(CreatePixbuf(sz), Planed::Transition(plc, sz));

        pcb.DataTag() = tag;
    }
    return pcb;
}

PixCanvasBuf& GetAuthorPCB(Menu mn)
{
    return GetTaggedPCB(mn, AUTHOR_TAG);
}

class CommonMenuRVis: public CommonRenderVis
{
    typedef CommonRenderVis MyParent;
    public:
                  CommonMenuRVis(const char* pcb_tag, RectListRgn& r_lst): 
                      MyParent(r_lst), pcbTag(pcb_tag) {}

    protected:
        const char* pcbTag;

     virtual CanvasBuf& FindCanvasBuf(MenuRegion& menu_rgn);

void  RenderStatic(FrameThemeObj& fto);
};

CanvasBuf& CommonMenuRVis::FindCanvasBuf(MenuRegion& menu_rgn)
{
    MenuMD* mn = GetOwnerMenu(&menu_rgn);
    return GetTaggedPCB(mn, pcbTag); 
}

class SPRenderVis: public CommonMenuRVis
{
    typedef CommonMenuRVis MyParent;
    public:
                  SPRenderVis(RectListRgn& r_lst, xmlpp::Element* spu_node, bool not_letterbox16_9)
                    : MyParent(SubPicTag(not_letterbox16_9), r_lst), isSelect(false), spuNode(spu_node) {}

            void  SetSelect(bool is_select) { isSelect = is_select; }

         //virtual  void  VisitImpl(MenuRegion& menu_rgn);
         virtual  void  RenderBackground();
         virtual  void  Visit(TextObj& t_obj);
         virtual  void  Visit(FrameThemeObj& fto);

    protected:
            bool  isSelect;
  xmlpp::Element* spuNode;
};

void SPRenderVis::RenderBackground()
{
    //drw->SetForegroundColor(BLACK2_CLR);
    drw->SetForegroundColor(0);
    drw->Fill(cnvBuf->FrameRect());
}

class SubtitleWrapper
{
    public:
    SubtitleWrapper(TextObj& t_obj, const RGBA::Pixel& clr, EdtTextRenderer& rndr_) 
        :tObj(t_obj), origClr(t_obj.Color()), rndr(rndr_),
         pCont(rndr.PanLay()->get_context())
    { 
        tObj.SetColor(clr);
        rndr.ConvertPrecisely() = true;
        SetAntialias(Cairo::ANTIALIAS_NONE);
    }
   ~SubtitleWrapper() 
    { 
        tObj.SetColor(origClr); 
        rndr.ConvertPrecisely() = false;
        SetAntialias(Cairo::ANTIALIAS_DEFAULT);
    }

    protected:
                TextObj& tObj;
            RGBA::Pixel  origClr;

        EdtTextRenderer& rndr;  // из-за прозрачного фона требуется точный перевод Cairo <-> Pixbuf
  RefPtr<Pango::Context> pCont; // для субтитров испоьзуется <= 16 цветов,
     Cairo::FontOptions  fOpt;  // поэтому убираем сглаживание

    void SetAntialias(Cairo::Antialias alias_type)
    {
        // не меняем сглаживание, потому что:
        // 1) все равно приходится дискретизировать, DiscreteByAlpha()
        // 2) побочный эффект - в некоторых случаях настроек шрифтов (системным образом,
        //  точно не определил) помимо антиалиасинга изменяется ширина результата рендеринга,
        //  что выражается визуально, в расхождении текста и его субкартинки (в dvd-меню)
        //  => ошибка; об этом же говорит справка к pango_cairo_context_set_font_options()
        //  (не знаю, что конкретно на это влияет,- один из атрибутов _cairo_font_options)
        return; 
        fOpt.set_antialias(alias_type);
        pCont->set_cairo_font_options(fOpt);
    }
};

static void ScriptButton(xmlpp::Element* spu_node, bool is_select, const Rect& plc,
                         CanvasBuf* cnv_buf)
{
    if( !is_select )
    {
        xmlpp::Element* btn_node = spu_node->add_child("button");
        // :KLUDGE: должны быть четными
        Rect rct(plc);
        // для letterbox & 16:9
        if( ShiftPCB* s_pcb = dynamic_cast<ShiftPCB*>(cnv_buf) )
            rct += s_pcb->shift;
        btn_node->set_attribute("x0", boost::lexical_cast<std::string>(rct.lft));
        btn_node->set_attribute("y0", boost::lexical_cast<std::string>(rct.top));
        btn_node->set_attribute("x1", boost::lexical_cast<std::string>(rct.rgt));
        btn_node->set_attribute("y1", boost::lexical_cast<std::string>(rct.btm));
    }
}

// :TODO: см. задание "DiscreteAlpha"
void DiscreteByAlpha(RefPtr<Gdk::Pixbuf> pix, const RGBA::Pixel& main_clr) //, int clr_cnt)
{
    int wdh = pix->get_width();
    int hgt = pix->get_height();
    RGBA::Pixel* pix_dat = (RGBA::Pixel*)pix->get_pixels();
    int stride = pix->get_rowstride();

    //uchar portion = main_clr.alpha/clr_cnt;
    unsigned char* pix_row = (unsigned char*)pix_dat;
    for( int y=0; y<hgt; y++, pix_row += stride )
    {
        pix_dat = (RGBA::Pixel*)pix_row;
        for( int x=0; x<wdh; x++, pix_dat++ )
        {
            //if( alpha != main_clr.alpha )
            //    alpha -= alpha%portion;
            //if( alpha != 0 ) 
                //alpha = main_clr.alpha;
            RGBA::Pixel& pxl = *pix_dat;
            if( pxl.alpha != 0 )
                pxl = main_clr;
        }
    }
}

static RGBA::Pixel GetSPColor(Comp::MediaObj& obj, bool activated_clr)
{
    //return activated_clr ? SELECT_CLR : HIGH_CLR;
    SubpicturePalette& pal = GetOwnerMenu(&obj)->subPal;
    return activated_clr ? pal.actClr : pal.selClr ;
}

void SPRenderVis::Visit(TextObj& t_obj)
{
    std::string targ_str;    
    if( HasButtonLink(t_obj, targ_str) )
    {
        RGBA::Pixel clr = GetSPColor(t_obj, isSelect);
        // меняем на лету цвет и др. параметры
        SubtitleWrapper sw(t_obj, clr, FindTextRenderer(t_obj, *cnvBuf));
        Make(t_obj).Render();
        // все равно приходится явно дискретизировать, потому что подчеркивание, например,
        // добавляет еще несколько цветов (особенно если есть пересечение его с буквой)
        Rect plc = CalcRelPlacement(t_obj.Placement());
        DiscreteByAlpha(MakeSubPixbuf(drw->Canvas(), plc), clr);

        ScriptButton(spuNode, isSelect, plc, cnvBuf);
    }
}

static RefPtr<Gdk::Pixbuf> CopyScale(RefPtr<Gdk::Pixbuf> src, const Point& sz)
{
    RefPtr<Gdk::Pixbuf> pix = CreatePixbuf(sz);
    RGBA::Scale(pix, src);
    return pix;
}

void SPRenderVis::Visit(FrameThemeObj& fto)
{
    std::string targ_str;    
    if( HasButtonLink(fto, targ_str) )
    {
        // как и в случае текста, для субтитров DVD главное - не перебрать ограничение
        // кол-ва используемых цветов (<=16); поэтому подгоняем рамку по месту, а
        // не наоборот (чтоб обойти без финального скалирования) 
        Rect plc = CalcRelPlacement(fto.Placement());
        Point sz(plc.Size());
        RefPtr<Gdk::Pixbuf> obj_pix = CreatePixbuf(sz); //PixbufSize(td.vFrameImg));
        uint clr = GetSPColor(fto, isSelect).ToUint();
        obj_pix->fill(clr);

        RefPtr<Gdk::Pixbuf> alpha_pix;
        if( IsIconTheme(fto) )
            alpha_pix = GetThemeIcon(fto, sz);
        else
        {
            const Editor::ThemeData& td = Editor::ThemeCache::GetTheme(fto.ThemeName());
            if( fto.hlBorder )
            {
                fs::path brd_fname = Project::FindThemePath(fto.ThemeName()) / "highlight_border.png";
                if( fs::exists(brd_fname) )
                    alpha_pix = CopyScale(Gdk::Pixbuf::create_from_file(brd_fname.string()), sz);
                else
                {    
                    alpha_pix = CopyScale(td.frameImg, sz);
                    RGBA::ApplyPicture(alpha_pix, CopyScale(td.vFrameImg, sz), RGBA::potDELETE_ALPHA);
                }
            }
            else
                alpha_pix = CopyScale(td.vFrameImg, sz);
        }

        RGBA::CopyAlphaComposite(obj_pix, alpha_pix, true);
        DiscreteByAlpha(obj_pix, clr);

        RGBA::RgnPixelDrawer::DrwFunctor drw_fnr = bb::bind(&RGBA::CopyArea, drw->Canvas(), obj_pix, plc, _1);
        drw->DrawWithFunctor(plc, drw_fnr);

        ScriptButton(spuNode, isSelect, plc, cnvBuf);
    }
}

std::string MakeMenuPath(const std::string& out_dir, Menu mn, int i)
{
    return AppendPath(out_dir, MenuAuthorDir(mn, i));
}

static void SaveMenuPicture(const std::string& mn_dir, const std::string fname, 
                            RefPtr<Gdk::Pixbuf> cnv_pix)
{
    cnv_pix->save(AppendPath(mn_dir, fname), "png");
}

void InitTextForTaggedPCB(Menu mn, const char* tag)
{
    SimpleInitTextVis titv(GetTaggedPCB(mn, tag));
    GetMenuRegion(mn).Accept(titv);
}

static void IterateAuthoringEvents()
{
    IteratePendingEvents();

    Author::CheckAbortByUser();
}

static int WorkCnt = 0;
static int DoneCnt = 0;
static void PulseRenderProgress()
{
    if( Execution::ConsoleMode::Flag )
        return;

    DoneCnt++;
    ASSERT( WorkCnt && (DoneCnt <= WorkCnt) );
    Author::SetStageProgress(DoneCnt/(double)WorkCnt);

    IterateAuthoringEvents();
}

void MakeSubpictures(Menu mn, bool not_letterbox16_9, const std::string& mn_dir)
{
    const char* data_tag = SubPicTag(not_letterbox16_9);
    InitTextForTaggedPCB(mn, data_tag);

    MenuRegion& m_rgn   = GetMenuRegion(mn);
    PixCanvasBuf&  cnv_buf = GetTaggedPCB(mn, data_tag);
    RefPtr<Gdk::Pixbuf> cnv_pix = cnv_buf.Source();

    RectListRgn rct_lst;
    rct_lst.push_back(cnv_buf.FramePlacement());
    
    const char* highlight_fname = not_letterbox16_9 ? "MenuHighlight.png" : "MenuHighlightLB.png" ;
    const char* select_fname    = not_letterbox16_9 ? "MenuSelect.png"    : "MenuSelectLB.png" ;
    const char* spumux_fname    = not_letterbox16_9 ? "Menu.xml" : "MenuLB.xml" ;
    
    xmlpp::Document doc;
    xmlpp::Element* root_node = doc.create_root_node("subpictures");
    xmlpp::Element* spu_node = root_node->add_child("stream")->add_child("spu");
    spu_node->set_attribute("start", "00:00:00.0");
    spu_node->set_attribute("end",   "00:00:00.0");
    spu_node->set_attribute("highlight", highlight_fname);
    spu_node->set_attribute("select",    select_fname);
    // используем "родной" прозрачный цвет в png
    //std::string trans_color = ColorToString(BLACK2_CLR);
    //spu_node->set_attribute("transparent", trans_color);
    spu_node->set_attribute("force",       "yes");
    spu_node->set_attribute("autoorder",   "rows");

    // * активация
    SPRenderVis r_vis(rct_lst, spu_node, not_letterbox16_9);
    m_rgn.Accept(r_vis);
    SaveMenuPicture(mn_dir, highlight_fname, cnv_pix);
    doc.write_to_file_formatted(AppendPath(mn_dir, spumux_fname));

    // * выбор
    r_vis.SetSelect(true);
    m_rgn.Accept(r_vis);
    SaveMenuPicture(mn_dir, select_fname, cnv_pix);
}

bool RenderSubPictures(const std::string& out_dir, Menu mn, int i, 
                       std::iostream& menu_list)
{
    // :WARN: правильно ли обращаемся (на питоне) к файловой системе не в utf8?
    std::string mn_dir = MenuAuthorDir(mn, i, false);
    menu_list << "'''" << mn_dir << "''',\n"; 
    mn_dir = AppendPath(out_dir, ConvertPathFromUtf8(mn_dir));
    fs::create_directory(mn_dir);
    WorkCnt++;

    MakeSubpictures(mn, true, mn_dir);
    if( !IsMenuToBe4_3() )
        MakeSubpictures(mn, false, mn_dir);
    
    // остальное
    //fs::copy_file(SConsAuxDir()/"menu_SConscript", fs::path(mn_dir)/"SConscript");
    std::string str = ReadAllStream(SConsAuxDir()/"menu_SConscript");
    bool is_motion  = mn->MtnData().isMotion;
    str = boost::format(str) % (is_motion ? "True" : "False") % bf::stop;
    WriteAllStream(fs::path(mn_dir)/"SConscript", str);

    // для последующего рендеринга
    SetCBDirty(GetAuthorPCB(mn));

    return true;
}

class MenuRenderVis: public CommonMenuRVis
{
    typedef CommonMenuRVis MyParent;
    public:
                  MenuRenderVis(RectListRgn& r_lst): MyParent(AUTHOR_TAG, r_lst) {}

         virtual RefPtr<Gdk::Pixbuf> CalcBgShot();
         virtual  void  Visit(FrameThemeObj& fto);
};

RefPtr<Gdk::Pixbuf> GetPrimaryAuthoredShot(MediaItem mi, const Point& sz = Point())
{
    // :TODO: если потребуется кэширование - см. задание КпА (кэширование при авторинге)
    return Project::PrimaryShotGetter::Make(mi, sz);
}

RefPtr<Gdk::Pixbuf> MenuRenderVis::CalcBgShot()
{
    return GetPrimaryAuthoredShot(menuRgn->BgRef());
}

class FTOAuthorData: public HiQuData
{
    typedef HiQuData MyParent;
    public:
                   FTOAuthorData(DataWare& dw): MyParent(dw) {}
    protected:

 virtual      RefPtr<Gdk::Pixbuf> CalcSource(Project::MediaItem mi, const Point& sz);
};

CanvasBuf& UpdateAuthoredMenu(Menu mn)
{
    MenuRegion& m_rgn   = GetMenuRegion(mn);
    CanvasBuf&  cnv_buf = GetAuthorPCB(mn);

    RectListRgn& rct_lst = cnv_buf.RenderList();
    if( rct_lst.size() )
    {
        RgnListCleaner lst_cleaner(cnv_buf);
        MenuRenderVis r_vis(rct_lst);
        m_rgn.Accept(r_vis);
    }
    return cnv_buf;
}

// аналог GetRenderedShot()
RefPtr<Gdk::Pixbuf> GetAuthoredMenu(Menu mn)
{
    return UpdateAuthoredMenu(mn).FramePixbuf();
}

RefPtr<Gdk::Pixbuf> GetAuthoredLRS(ShotFunctor s_fnr)
{
    static int RecurseDepth = 0;
    return GetLimitedRecursedShot(s_fnr, RecurseDepth);
}

RefPtr<Gdk::Pixbuf> FTOAuthorData::CalcSource(Project::MediaItem mi, const Point& sz)
{
    RefPtr<Gdk::Pixbuf> pix;
    if( Menu mn = IsMenu(mi) )
    {
        ShotFunctor s_fnr = bb::bind(&GetAuthoredMenu, mn);
        pix = MakeCopyWithSz(GetAuthoredLRS(s_fnr), sz);
    }
    else
        pix = GetPrimaryAuthoredShot(mi, sz);
    return pix;
}

void CommonMenuRVis::RenderStatic(FrameThemeObj& fto)
{
    FTOAuthorData& pix_data = fto.GetData<FTOAuthorData>(AUTHOR_TAG);
    DoRenderFTO(drw.get(), fto, pix_data, cnvBuf->Transition());
}

void MenuRenderVis::Visit(FrameThemeObj& fto)
{
    RenderStatic(fto);
}

//
// Функционал анимационных меню
// 

static bool IsToBeMoving(MediaItem mi)
{
    return mi && (IsVideo(mi) || IsChapter(mi));
}

static Rect RealPosition(Comp::MediaObj& obj, const Planed::Transition& trans)
{
    return AbsToRel(trans, obj.Placement());
}

static void FFmpegError(const ExitData& ed)
{
    //Author::ErrorByED(FFmpegErrorTemplate(), ed);
    Author::ApplicationError("ffmpeg", ed);
}

static void FFmpegError(const std::string& msg)
{
    Author::ApplicationError("ffmpeg", msg);
}

static std::string errno2str()
{
    namespace bs = boost::system;
#if BOOST_MINOR_VERSION >= 44
    const bs::error_category& s_cat = bs::system_category();
#else
    const bs::error_category& s_cat = bs::system_category; // = bs::get_system_category();
#endif 
    return bs::error_code(errno, s_cat).message();
}

static void WriteAsPPM(int fd, RefPtr<Gdk::Pixbuf> pix, TrackBuf& buf)
{
    int wdh = pix->get_width();
    int hgt = pix->get_height();
    int stride     = pix->get_rowstride();
    int channels   = pix->get_n_channels();
    guint8* pixels = pix->get_pixels();

    // операция записи в канал очень дорогая, поэтому собираем PPM
    // в буфер и за раз все записываем

    // не пользуемся TrackBuf::End(), TrackBuf::Append(), ...
    buf.Reserve(30); // для заголовка
    snprintf(buf.Beg(), 30, "P6\n%d %d\n255\n", wdh, hgt);
    // длина данных PPM: заголовок + все пикселы * 3 байта
    int h_sz = strlen(buf.Beg());
    int sz   = h_sz + 3 * wdh * hgt;
    buf.Reserve(sz);

    char* beg = buf.Beg();
    char* cur = beg + h_sz;
    for( int row = 0; row < hgt; row++, pixels += stride )
    {
        guint8* p = pixels;
        for( int col = 0; col < wdh; col++, p += channels, cur += 3 )
        {
            // еще оптимизации:
            // - memcpy(, 3)
            // - запись внахлест memcpy(, 4) (неясно, быстрее ли)
            cur[0] = p[0];
            cur[1] = p[1];
            cur[2] = p[2];
        }
    }

    ASSERT( cur - beg == sz );
    //checked_writeall(fd, beg, sz);
    if( !writeall(fd, beg, sz) )
        FFmpegError(errno2str());
}

static Point DVDAspect(bool is4_3)
{
    return is4_3 ? Point(4, 3) : Point(16,9);
}

AutoDVDTransData::AutoDVDTransData(bool is4_3_): is4_3(is4_3_), 
    srcAspect(DVDAspect(is4_3_)), audioNum(1) {}

static int PadSize(int free_space)
{
    // ffmpeg требует четных по размеру матов
    return (free_space/2) & ~0x1;
}

bool IsVersionGE(const TripleVersion& big_v, const TripleVersion& v)
{
    bool cmp_res;
    if( CompareComponent(v.major, big_v.major, cmp_res) ||
        CompareComponent(v.minor, big_v.minor, cmp_res) || 
        CompareComponent(v.micro, big_v.micro, cmp_res)    )
        return cmp_res;

    // версии равны
    return true;
}

static std::string FFSizeOption(const Point& sz)
{
    return boost::format("-s %1%x%2%") % sz.x % sz.y % bf::stop;
}

static std::string FFImgSizeOption(const Point& sz, int val, bool is_left_right)
{
    Point img_sz = is_left_right ? Point(sz.x - 2*val, sz.y) : Point(sz.x, sz.y - 2*val);
    return FFSizeOption(img_sz);
}

static std::string FFPadFilterSet(const Point& sz, int val, bool is_left_right)
{
    int x = 0, y = 0;
    if( is_left_right )
        x = val;
    else
        y = val;

    return boost::format("%1% -vf pad=%2%:%3%:%4%:%5%:black") 
        % FFImgSizeOption(sz, val, is_left_right) % sz.x % sz.y % x % y % bf::stop;
}

static std::string FFSizeSet(const Point& sz, int val, 
                                  bool is_left_right, bool is_vf)
{
    std::string sz_str;
    if( is_vf )
        sz_str = FFPadFilterSet(sz, val, is_left_right);
    else
    {
        const char* fmt = is_left_right ? "%1% -padleft %2% -padright %2%" : "%1% -padtop %2% -padbottom %2%" ;
        sz_str = boost::format(fmt) % FFImgSizeOption(sz, val, is_left_right) % val % bf::stop;
    }
    return sz_str;
}

static void AppendOpts(std::string& opts, const std::string& append_args)
{
    if( !append_args.empty() )
        opts += " " + append_args;
}

std::string FFmpegToDVDArgs(const std::string& out_fname, const AutoDVDTransData& atd, bool is_pal,
                            const DVDTransData& td)
{
    // * предполагается после создания команды ее вызов => требуется
    // проверка такой возможности
    TripleVersion filter_ver = CheckFFDVDEncoding().avfilter;
    // вместо -padX использовать -vf pad=w:h:x:y:black
    bool is_vf = IsVersionGE(filter_ver, TripleVersion(1, 20, 0));

    // *
    const char* target =  is_pal ? "pal" : "ntsc" ;
    Point DVDDimension(DVDDims dd, bool is_pal);
    Point sz = DVDDimension(td.dd, is_pal);

    // * битрейт
    std::string bitrate_str;
    int vrate = td.vRate;
    ASSERT( vrate >= 0 );
    if( vrate )
        bitrate_str = boost::format("-b %1%k ") % vrate % bf::stop;
    bitrate_str += boost::format("-ab %1%k ") % TRANS_AUDIO_BITRATE % bf::stop;

    // * соотношение
    bool is_4_3 = atd.is4_3;
    // * размеры
    Point img_sz = FitIntoRect(sz, DVDAspect(is_4_3), atd.srcAspect).Size();
    std::string sz_str = FFSizeOption(sz);
    if( img_sz.x < sz.x )
    {
        // справа и слева
        int val = PadSize(sz.x - img_sz.x);
        if( val )
            sz_str = FFSizeSet(sz, val, true, is_vf);
    }
    else if( img_sz.y < sz.y )
    {
        // сверху и снизу
        int val = PadSize(sz.y - img_sz.y);
        if( val )
            sz_str = FFSizeSet(sz, val, false, is_vf);
    }
    // * дополнительные аудио
    std::string add_audio_str;
    for( int i=0; i<(atd.audioNum-1); i++ )
        // ffmpeg обнуляет audio_codec_name после каждого -newaudio и -i
        add_audio_str += " -acodec ac3 -newaudio";

    // * доп. опции
    std::string add_opts("-mbd rd -trellis 2 -cmp 2 -subcmp 2");
    {
        AppendOpts(add_opts, PrefContents("ffmpeg_options"));
        AppendOpts(add_opts, td.ctmFFOpt);
    }

    return boost::format("-target %1%-dvd -aspect %2% %3% %4%-y %7% %5%%6%")
        % target % (is_4_3 ? "4:3" : "16:9") % sz_str
        % bitrate_str % FilenameForCmd(out_fname) % add_audio_str % add_opts % bf::stop;
}

std::string FFmpegToDVDArgs(const std::string& out_fname, bool is_4_3, bool is_pal)
{
    // для меню - всегда полноразмерное
    DVDTransData td;
    td.dd = dvd720;
    return FFmpegToDVDArgs(out_fname, is_4_3, is_pal, td);
}

std::string FFmpegPostArgs(const std::string& out_fname, bool is_4_3, bool is_pal, 
                           const std::string& a_fname, double a_shift)
{
    std::string a_input = "-an"; // без аудио
    if( !a_fname.empty() )
    {
        std::string safe_a_fname = FilenameForCmd(a_fname);
        int idx = -1;
        // в последних версиях ffmpeg (черт побери) поменяли отображение
        // входных потоков на выходные, в итоге возникают проблемы с указанием
        // аудио из видеофайла; в итоге:
        // - если первым идет -i <только видео>, то он перекрывается вторым 
        //   -i <video&audio>; потому нужна явная карта, -map
        // - если в обратном порядке, то вообще не работает, :TODO: 
        {
            // :TRICKY: статус не проверяем,- такой способ все равно
            // считается "ошибочным" (но стандарт де факто для получения инфо
            // о файле через ffmpeg)
            std::string a_info;
            PipeOutput("ffmpeg -i " + safe_a_fname, a_info);

            static re::pattern audio_idx("Stream #"RG_NUM"\\."RG_NUM".*Audio:");

            re::match_results what;
            // флаг означает, что перевод строки не может быть точкой
            if( re::search(a_info, what, audio_idx, boost::match_not_dot_newline) )
                idx = boost::lexical_cast<int>(what.str(2));
            else
                FFmpegError(BF_("no audio stream in %1%") % safe_a_fname % bf::stop);
        }
        ASSERT( idx >= 0 );
        // каждая опция -map задает соответ. одному выходному потоку входной поток
        std::string map = boost::format("-map 0:0 -map 1:%1%") % idx % bf::stop;

        std::string shift; // без смещения
        if( a_shift )
            shift = boost::format("-ss %.2f ") % a_shift % bf::stop;
        a_input = boost::format("%2%-i %1% %3%") % safe_a_fname % shift % map % bf::stop; 
    }

    return a_input + " " + FFmpegToDVDArgs(out_fname, is_4_3, is_pal);
}

#define MOTION_MENU_TAG "Motion Menus"

class MotionMenuRVis: public CommonMenuRVis
{
    typedef CommonMenuRVis MyParent;
    public:
                  MotionMenuRVis(RectListRgn& r_lst): MyParent(MOTION_MENU_TAG, r_lst) {}

         virtual  void  RenderBackground();
         virtual  void  Visit(FrameThemeObj& fto);
};

static bool IsMovingBack(MenuRegion& m_rgn)
{
    return IsToBeMoving(m_rgn.BgRef());
}

struct MotionTimer
{
    int  idx;
    // нарезаем изображения со своим fps (близким к стандартным 25 и 29,97),-
    // ffmpeg разберется как сделать из -target %5%-dvd
    static const int fps = 30;
    static const double shift; // не интегральный тип, потому в C++98 "приравнивать" нельзя 

            MotionTimer(): idx(0) {}

            // рендеринг первого кадра
      bool  IsFirst() { return idx == 0; }

    double  Time() { return idx * shift; }
};

const int MotionTimer::fps;
const double MotionTimer::shift = 1. / MotionTimer::fps;

static MotionTimer& GetMotionTimer(Menu mn)
{
    return mn->GetData<MotionTimer>(MOTION_MENU_TAG);
}

static std::string GetFilename(VideoStart vs)
{
    return GetFilename(*vs.first);
}

static RefPtr<Gdk::Pixbuf> GetMotionFrame(MediaItem ref, DataWare& src, MotionTimer& mt)
{
    VideoStart vs = GetVideoStart(ref);
    VideoViewer& plyr = src.GetData<VideoViewer>(MOTION_MENU_TAG);
    if( mt.IsFirst() )
        RGBOpen(plyr, GetFilename(vs));

    double tm = vs.second + mt.Time();
    return VideoPE(plyr, tm).Make(Point()).ROPixbuf();
}

static void LoadMotionFrame(RefPtr<Gdk::Pixbuf>& pix, MediaItem ref, DataWare& src,
                            MotionTimer& mt)
{
    RGBA::CopyOrScale(pix, GetMotionFrame(ref, src, mt));
}

void MotionMenuRVis::RenderBackground()
{
    Menu mn = GetOwnerMenu(menuRgn);
    MotionTimer& mt  = GetMotionTimer(mn);
    MediaItem bg_ref = menuRgn->BgRef();
    Rect plc = cnvBuf->FrameRect();

    if( IsToBeMoving(bg_ref) )
    {
        //// явно отображаем кадр в стиле монитора (DAMonitor),
        //// а не редактора, потому что:
        //// - область и так всю перерисовывать
        //// - самый быстрый вариант (без каких-либо посредников)
        ////drw->ScalePixbuf(pix, cnvBuf->FrameRect());
        //LoadMotionFrame(menu_pix, bg_ref, *mn, mt);
        DoRenderBackground(drw, GetMotionFrame(bg_ref, *mn, mt), menuRgn, plc);
    }
    else
    {
        RefPtr<Gdk::Pixbuf> static_menu_pix = GetAuthoredMenu(mn);
        if( mt.IsFirst() )
        {
            // в первый раз отразим статичный вариант,
            // потому что пока вообще пусто (а область рендеринга сужена
            // до анимационных элементов)
            //MyParent::RenderBackground();
            RGBA::Scale(cnvBuf->FramePixbuf(), static_menu_pix);
        }
        else
            drw->ScalePixbuf(static_menu_pix, plc);
    }
}

void MotionMenuRVis::Visit(FrameThemeObj& fto)
{
    MediaItem mi = MIToDraw(fto);
    if( IsToBeMoving(mi) && !IsIconTheme(fto) )
    {
        // напрямую делаем то же, что FTOData::CompositeFTO(),
        // чтобы явно видеть затраты
        const Editor::ThemeData& td = Editor::ThemeCache::GetTheme(fto.ThemeName());
        // нужен оригинал в размере vFrameImg
        RefPtr<Gdk::Pixbuf> obj_pix = td.vFrameImg->copy();
        LoadMotionFrame(obj_pix, mi, fto, GetMotionTimer(GetOwnerMenu(&fto)));

        RefPtr<Gdk::Pixbuf> pix = CompositeWithFrame(obj_pix, td);

        drw->CompositePixbuf(pix, CalcRelPlacement(fto.Placement()));
    }
    else
        RenderStatic(fto);
}

bool IsMotion(Menu mn)
{
    return mn->MtnData().isMotion;
}

static void SaveMenuPng(const std::string& mn_dir, Menu mn)
{
    SaveMenuPicture(mn_dir, "Menu.png", GetAuthoredMenu(mn));
}

static std::string MakeFFmpegPostArgs(const std::string& mn_dir, Menu mn)
{
    MotionData& mtn_dat = mn->MtnData();
    // аспект ставим как можем (а не из параметров меню) из-за того, что качество
    // авторинга важнее гибкости (все валим в один titleset)
    bool is_4_3 = IsMenuToBe4_3();
    std::string out_fname = AppendPath(mn_dir, "Menu.mpg");

    std::string a_fname;
    double a_shift = 0.;
    if( mtn_dat.isIntAudio )
    {
        if( MediaItem a_ref = mtn_dat.audioRef.lock() )
        {
            VideoStart vs = GetVideoStart(a_ref);
            a_fname = GetFilename(vs);
            a_shift = vs.second;
        }
    }
    else
        a_fname = mtn_dat.audioExtPath;
    return FFmpegPostArgs(out_fname, is_4_3, IsPALProject(), a_fname, a_shift);
}

double MenuDuration(Menu mn)
{
    double duration = mn->MtnData().duration;
    const int MAX_MOTION_DUR = 60 * 60; // адекватный теорет. максимум
    ASSERT_RTL( duration > 0 && duration <= MAX_MOTION_DUR );

    return duration;
}

static Gtk::TextView& PrintCmdToDetails(const std::string& cmd)
{
    Gtk::TextView& tv = Author::GetES().detailsView;
    AppendCommandText(tv, cmd);
    return tv;
}

void RunExtCmd(const std::string& cmd, const char* app_name, 
               const ReadReadyFnr& add_fnr)
{
    ExitData ed;
    if( Execution::ConsoleMode::Flag )
    {
        io::cout << cmd << io::endl;
        ed = System(cmd);
    }
    else
    {
        Gtk::TextView& tv = PrintCmdToDetails(cmd);
        ed = Author::AsyncCall(0, cmd.c_str(), TextViewAppender(tv, add_fnr));
    }
    if( !ed.IsGood() )
        Author::ApplicationError(app_name, ed);
}

void RunFFmpegCmd(const std::string& cmd, const ReadReadyFnr& add_fnr)
{
    RunExtCmd(cmd, "ffmpeg", add_fnr);
}

static void SaveStillMenuMpg(const std::string& mn_dir, Menu mn)
{
    // сохраняем Menu.png и рендерим из нее
    SaveMenuPng(mn_dir, mn);
    std::string img_fname = AppendPath(mn_dir, "Menu.png");

    std::string ffmpeg_cmd = boost::format("ffmpeg -t %3$.2f -loop_input -i \"%1%\" %2%") 
        % img_fname % MakeFFmpegPostArgs(mn_dir, mn) % MenuDuration(mn) % bf::stop;

    RunFFmpegCmd(ffmpeg_cmd);
}

FFmpegCloser::~FFmpegCloser()
{
    // 1 закрываем вход, чтобы разблокировать ffmpeg (несколько раз происходила взаимная 
    // блокировка с прекращением деятельности с обоих сторон,- судя по всему ffmpeg игнорировал
    // прерывание по EINTR и снова блокировался на чтении)
    ASSERT( inFd != NO_HNDL );
    close(inFd);
    
    // варианты остановить транскодирование сразу после окончания видео, см. ffmpeg.c:av_encode():
    // - через сигнал (SIGINT, SIGTERM, SIGQUIT)
    // - по достижению размера или времени (см. соответ. опции вроде -t); но тут возможна проблема
    //   блокировки/закрытия раньше, чем мы закончим посылку всех данных; да и время высчитывать тоже придется
    // - в момент окончания первого из источников, -shortest; не подходит, если аудио существенно длинней
    //   чем мы хотим записать
    Execution::Stop(pid);
    
    // 2 дождаться выхода и проверить статус (и только после этого закрываем выходы)
    ed = WaitForExit(pid);
}

static PixCanvasBuf& GetMotionPCB(Menu mn)
{
    return GetTaggedPCB(mn, MOTION_MENU_TAG);
}

PPMWriter::PPMWriter(int in_fd): inFd(in_fd) 
{ 
    pipeBuf.SetUnlimited(); 
}

void PPMWriter::Write(RefPtr<Gdk::Pixbuf> img)
{
    WriteAsPPM(inFd, img, pipeBuf);
}

static void PipeVideo(Menu mn, int in_fd)
{
    MenuRegion& m_rgn       = GetMenuRegion(mn);
    PixCanvasBuf& mtn_buf   = GetMotionPCB(mn);
    RefPtr<Gdk::Pixbuf> img = mtn_buf.FramePixbuf();
    ASSERT( img );

    PPMWriter ppm_writer(in_fd);
    double duration = MenuDuration(mn);

    MotionTimer& mt = GetMotionTimer(mn);
    ASSERT( mt.IsFirst() );
    for( ; mt.Time() < duration; mt.idx++ )
    {
        MotionMenuRVis r_vis(mtn_buf.RenderList());
        m_rgn.Accept(r_vis);

        //std::string fpath = "/dev/null"; //boost::format("../dvd_out/ppms/%1%.ppm") % i % bf::stop;
        //int fd = OpenFileAsArg(fpath.c_str(), false);
        ppm_writer.Write(img);
        //close(fd);

        if( !Execution::ConsoleMode::Flag )
            // вывод от ffmpeg
            IterateAuthoringEvents();
    }
}

static void CheckStrippedFFmpeg(const re::pattern& pat, const std::string& conts, 
                                const char* function)
{
    if( !re::search(conts, pat) )
    {
        // похоже в Ubuntu не собирают урезанную версию ffmpeg (по крайней мере с Karmic),
        // потому пока считаем редкой ошибкой -> не переводим
        FFmpegError(boost::format("stripped ffmpeg version is detected; please install full version (with %1%)")
                    % function % bf::stop);
    }
}

static bool CheckForCodecList(const std::string& conts)
{
    static re::pattern codecs_format("^Codecs:$");
    return re::search(conts, codecs_format);
}

static void CheckNoCodecs(bool res)
{
    if( !res )
        FFmpegError("no codec is found");
}

int AsInt(const re::match_results& what, int idx)
{
    return boost::lexical_cast<int>(what.str(idx));
}

TripleVersion FindVersion(const std::string& conts, const re::pattern& ver_pat,
                          const char* app_name, const char* target_name)
{
    re::match_results what;
    if( !re::search(conts, what, ver_pat) )
    {
        const char* target = target_name ? target_name : app_name ;
        Author::ApplicationError(app_name, boost::format("can't parse %1% version") % target % bf::stop);
    }
    return TripleVersion(AsInt(what, 1), AsInt(what, 2), AsInt(what, 3));
}

// conts - вывод ffmpeg -formats
FFmpegVersion TestFFmpegForDVDEncoding(const std::string& conts)
{
    // ищем версию libavfilter
    // пример: " libavfilter    0. 4. 0 / "
#define RG_PADNUM RG_SPS RG_NUM
    static re::pattern avfilter_version(RG_BW"libavfilter"RG_PADNUM"\\."RG_PADNUM"\\."RG_PADNUM" / ");
    FFmpegVersion ff_ver;
    ff_ver.avfilter = FindVersion(conts, avfilter_version, "ffmpeg", "libavfilter");

    CheckNoCodecs(CheckForCodecList(conts));

    static re::pattern dvd_format("^ .E dvd"RG_EW);
    CheckStrippedFFmpeg(dvd_format, conts, "dvd format");

    static re::pattern mpeg2video_codec("^ .EV... mpeg2video"RG_EW);
    CheckStrippedFFmpeg(mpeg2video_codec, conts, "mpeg2 video encoder");

    // по факту ffmpeg всегда использует ac3, однако mp2 тоже возможен
    static re::pattern ac3_codec("^ .EA... ac3"RG_EW);
    CheckStrippedFFmpeg(ac3_codec, conts, "ac3 audio encoder");

    return ff_ver;
}

FFmpegVersion CheckFFDVDEncoding()
{
    FFmpegVersion ff_ver;
    // наличие полного ffmpeg
    std::string ff_formats;
    ExitData ed = PipeOutput("ffmpeg -formats", ff_formats);
    // старые версии выходят с 1 для нерабочих режимов -formats, -help, ... (Hardy)
    bool is_good = ed.IsGood() || ed.IsCode(1);
    if( !is_good )
    {
        const char* msg = ed.IsCode(127) ? _("command not found") : "unknown error" ;
        FFmpegError(msg);
    }
    else
    {
        // * в новых версиях ffmpeg распечатка собственно кодеков выделена
        //  в опцию -codecs
        // * нормальной (сквозной) нумерации собственно ffmpeg не имеет (см. version.sh,-
        // может появиться UNKNOWN, SVN-/git-ревизия и собственно версия), потому
        // определяем по наличию строки "Codecs:"
        if( !CheckForCodecList(ff_formats) )
        {
            std::string ff_codecs;
            ExitData ed = PipeOutput("ffmpeg -codecs", ff_codecs);
            CheckNoCodecs(ed.IsGood());

            ff_formats += ff_codecs;
        }

        ff_ver = TestFFmpegForDVDEncoding(ff_formats);
    }
    return ff_ver;
}

bool RenderMainPicture(const std::string& out_dir, Menu mn, int i)
{
    Author::Info((str::stream() << "Rendering menu \"" << mn->mdName << "\" ...").str());
    const std::string mn_dir = MakeMenuPath(out_dir, mn, i);

    if( IsMotion(mn) )
    {
        if( mn->MtnData().isStillPicture )
            SaveStillMenuMpg(mn_dir, mn);
        else
        {
            // пока рендеринг всех меню идет независимо удаляем данные сразу по окончанию
            DtorAction clear_motion_data(bb::bind(&ClearTaggedData, mn, MOTION_MENU_TAG));

            MenuRegion& m_rgn     = GetMenuRegion(mn);
            PixCanvasBuf& mtn_buf = GetMotionPCB(mn);

            // 1 определяем область перерисовки на итерацию
            RectListRgn& rlr = mtn_buf.RenderList();
            if( IsMovingBack(m_rgn) )
                // вся область из-за фона
                rlr.push_back(mtn_buf.FrameRect());
            else
            {
                boost_foreach( Comp::Object* obj, m_rgn.List() )
                    if( FrameThemeObj* frame_obj = dynamic_cast<FrameThemeObj*>(obj) )
                        if( IsToBeMoving(MIToDraw(*frame_obj)) )
                            rlr.push_back(RealPosition(*frame_obj, mtn_buf.Transition()));
            }
            ReDivideRects(rlr);

            // рендеринг анимационного меню
            // 2 рендеринг -> ffmpeg
            if( rlr.size() )
            {
                // подготовка
                InitTextForTaggedPCB(mn, MOTION_MENU_TAG);

                // 1. Ставим частоту fps впереди -i pipe, чтобы она воспринималась для входного потока
                // (а для выходного сработает -target)
                // 2. И наоборот, для выходные параметры (-aspect, ...) ставим после всех входных файлов
                std::string ffmpeg_cmd = boost::format("ffmpeg -r %1% -f image2pipe -vcodec ppm -i pipe: %2%")
                    % MotionTimer::fps % MakeFFmpegPostArgs(mn_dir, mn) % bf::stop;

                ExitData ed;
                {
                    FFmpegCloser pc(ed);
                    int out_err[2];
                    pc.pid = Spawn(0, ffmpeg_cmd.c_str(), Execution::ConsoleMode::Flag ? 0 : out_err, true, &pc.inFd);
    
                    if( Execution::ConsoleMode::Flag )
                    {
                        double all_time = GetClockTime();

                        PipeVideo(mn, pc.inFd);

                        all_time = GetClockTime() - all_time;
                        io::cout << "Time to run: " << all_time << io::endl;
                        io::cout << "FPS: " << GetMotionTimer(mn).idx / all_time << io::endl;
                    }
                    else
                    {
                        Gtk::TextView& tv = PrintCmdToDetails(ffmpeg_cmd);
                        // закрываем выходы после окончания работы ffmpeg, иначе тот грохнется
                        // по SIGPIPE
                        //OutErrBlock oeb(out_err, TextViewAppender(tv));
                        pc.oeb = new OutErrBlock(out_err, TextViewAppender(tv));

                        PipeVideo(mn, pc.inFd);
                    }
    
                }

                if( !ed.IsGood() )
                    // ffmpeg завершает с кодом 255 при посылке сигнала, ffmpeg.c:av_exit(int ret)
                    if( !ed.IsCode(255) )
                        FFmpegError(ed);
            }
            else
                SaveStillMenuMpg(mn_dir, mn);
        }
    }
    else
        SaveMenuPng(mn_dir, mn);

    PulseRenderProgress();
    return true;
}

void AuthorMenus(const std::string& out_dir)
{
    Author::SetStage(Author::stRENDER);
    WorkCnt = 1; // включая рендеринг субтитров
    DoneCnt = 0;

    Author::Info("Rendering menu subtitles ...");
    std::iostream& menu_list = Author::GetES().settings;
    menu_list << "Is4_3 = " << (IsMenuToBe4_3() ? 1 : 0) << "\n";
    // список меню
    menu_list << "List = [\n";
    // за один проход можно сделать и подготовку, и рендеринг вспомог. данных
    // * подготовка к рендерингу
    // * создание слоев/субтитров выбора и скрипта
    //ForeachMenu(lambda::bind(&SetAuthorControl, lambda::_1, lambda::_2));
    ForeachMenu(bb::bind(&RenderSubPictures, boost::ref(out_dir), _1, _2, boost::ref(menu_list)));
    menu_list << "]\n" << io::endl;
    PulseRenderProgress();

    // * отрисовка основного изображения
    ForeachMenu(bb::bind(&RenderMainPicture, boost::ref(out_dir), _1, _2));
}

} // namespace Project


