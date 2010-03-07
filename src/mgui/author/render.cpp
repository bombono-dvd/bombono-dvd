//
// mgui/author/render.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009 Ilya Murav'jov
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
#include "execute.h"

#include <mgui/render/editor.h> // CommonRenderVis
#include <mgui/project/menu-render.h>
#include <mgui/project/thumbnail.h> // Project::PrimaryShotGetter
#include <mgui/editor/text.h>   // EdtTextRenderer
#include <mgui/editor/bind.h>   // MBind::TextRendering

#include <mbase/project/table.h>

#include <mlib/filesystem.h>
#include <mlib/string.h>

#include <boost/lexical_cast.hpp>

void IteratePendingEvents()
{
    while( Gtk::Main::instance()->events_pending() )
        Gtk::Main::instance()->iteration();
}

namespace Project 
{

PixCanvasBuf& GetAuthorPCB(Menu mn)
{
    PixCanvasBuf& pcb = mn->GetData<PixCanvasBuf>(AUTHOR_TAG);
    if( !pcb.Canvas() )
    {
        Point sz(mn->Params().Size());
        pcb.Set(CreatePixbuf(sz), Planed::Transition(Rect0Sz(sz), sz));
        pcb.DataTag() = AUTHOR_TAG;
    }
    return pcb;
}

PixCanvasBuf& GetAuthorPCB(MenuRegion& menu_rgn)
{
    return GetAuthorPCB(GetOwnerMenu(&menu_rgn));
}

class SPRenderVis: public CommonRenderVis
{
    typedef CommonRenderVis MyParent;
    public:
                  SPRenderVis(RectListRgn& r_lst, xmlpp::Element* spu_node)
                    : MyParent(r_lst), isSelect(false), spuNode(spu_node) {}

            void  SetSelect(bool is_select) { isSelect = is_select; }

         //virtual  void  VisitImpl(MenuRegion& menu_rgn);
         virtual  void  RenderBackground();
         virtual  void  Visit(TextObj& t_obj);
         virtual  void  Visit(FrameThemeObj& fto);

    protected:
            bool  isSelect;
  xmlpp::Element* spuNode;

     virtual CanvasBuf& FindCanvasBuf(MenuRegion& menu_rgn) { return GetAuthorPCB(menu_rgn); }
};

// используем "родной" прозрачный цвет в png
//const uint BLACK2_CLR = 0x010101ff; // заменяется на прозрачный spumux'ом
const uint HIGH_CLR   = 0xfff00080;   // прозрачность = 50%
const uint SELECT_CLR = 0xff006c80;

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
        fOpt.set_antialias(alias_type);
        pCont->set_cairo_font_options(fOpt);
    }
};

static void ScriptButton(xmlpp::Element* spu_node, bool is_select, const Rect& plc)
{
    if( !is_select )
    {
        xmlpp::Element* btn_node = spu_node->add_child("button");
        // :KLUDGE: должны быть четными
        btn_node->set_attribute("x0", boost::lexical_cast<std::string>(plc.lft));
        btn_node->set_attribute("y0", boost::lexical_cast<std::string>(plc.top));
        btn_node->set_attribute("x1", boost::lexical_cast<std::string>(plc.rgt));
        btn_node->set_attribute("y1", boost::lexical_cast<std::string>(plc.btm));
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

void SPRenderVis::Visit(TextObj& t_obj)
{
    std::string targ_str;    
    if( HasButtonLink(t_obj, targ_str) )
    {
        RGBA::Pixel clr = isSelect ? SELECT_CLR : HIGH_CLR;
        // меняем на лету цвет и др. параметры
        SubtitleWrapper sw(t_obj, clr, FindTextRenderer(t_obj, *cnvBuf));
        Make(t_obj).Render();
        // все равно приходится явно дискретизировать, потому что подчеркивание, например,
        // добавляет еще несколько цветов (особенно если есть пересечение его с буквой)
        Rect plc = CalcRelPlacement(t_obj.Placement());
        DiscreteByAlpha(MakeSubPixbuf(drw->Canvas(), plc), clr);

        ScriptButton(spuNode, isSelect, plc);
    }
}

static void CopyArea(RefPtr<Gdk::Pixbuf> canv_pix, RefPtr<Gdk::Pixbuf> obj_pix, 
                     const Rect& plc, const Rect& drw_rgn)
{
    obj_pix->copy_area(drw_rgn.lft-plc.lft, drw_rgn.top-plc.top, drw_rgn.Width(), drw_rgn.Height(), 
                       canv_pix, drw_rgn.lft, drw_rgn.top);
}

void SPRenderVis::Visit(FrameThemeObj& fto)
{
    std::string targ_str;    
    if( HasButtonLink(fto, targ_str) )
    {
        const Editor::ThemeData& td = Editor::ThemeCache::GetTheme(fto.Theme());
        // как и в случае текста, для субтитров DVD главное - не перебрать ограничение
        // кол-ва используемых цветов (<=16); поэтому подгоняем рамку по месту, а
        // не наоборот (чтоб обойти без финального скалирования) 
        Rect plc = CalcRelPlacement(fto.Placement());
        Point sz(plc.Size());
        RefPtr<Gdk::Pixbuf> obj_pix = CreatePixbuf(sz); //PixbufSize(td.vFrameImg));
        uint clr = isSelect ? SELECT_CLR : HIGH_CLR;
        obj_pix->fill(clr);

        RefPtr<Gdk::Pixbuf> vf_pix = CreatePixbuf(sz);
        RGBA::Scale(vf_pix, td.vFrameImg);

        RGBA::CopyAlphaComposite(obj_pix, vf_pix, true);
        DiscreteByAlpha(obj_pix, clr);

        //drw->CompositePixbuf(obj_pix, plc);
        using namespace boost;
        RGBA::RgnPixelDrawer::DrwFunctor drw_fnr = lambda::bind(&CopyArea, drw->Canvas(), obj_pix, plc, lambda::_1);
        drw->DrawWithFunctor(plc, drw_fnr);

        ScriptButton(spuNode, isSelect, plc);
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

void InitAuthorData(Menu mn)
{
    SimpleInitTextVis titv(GetAuthorPCB(mn));
    GetMenuRegion(mn).Accept(titv);
}

static int WorkCnt = 0;
static int DoneCnt = 0;
static void PulseRenderProgress()
{
    if( Project::CheckAuthorMode::Flag )
        return;

    DoneCnt++;
    ASSERT( WorkCnt && (DoneCnt <= WorkCnt) );
    Author::SetStageProgress( DoneCnt/(double)WorkCnt * 100.);

    IteratePendingEvents();
    if( Author::GetES().userAbort )
        throw std::runtime_error("User Abortion"); // строка реально не нужна - сработает userAbort
}

bool RenderSubPictures(const std::string& out_dir, Menu mn, int i, 
                       std::iostream& menu_list)
{
    InitAuthorData(mn);

    MenuRegion& m_rgn   = GetMenuRegion(mn);
    CanvasBuf&  cnv_buf = GetAuthorPCB(mn);
    RefPtr<Gdk::Pixbuf> cnv_pix = cnv_buf.FramePixbuf();
    // :WARN: правильно ли обращаемся (на питоне) к файловой системе не в utf8?
    std::string mn_dir = MenuAuthorDir(mn, i, false);
    menu_list << "'''" << mn_dir << "''',\n"; 
    mn_dir = AppendPath(out_dir, ConvertPathFromUtf8(mn_dir));
    fs::create_directory(mn_dir);
    WorkCnt++;

    RectListRgn rct_lst;
    rct_lst.push_back(cnv_buf.FramePlacement());
    xmlpp::Document doc;
    xmlpp::Element* root_node = doc.create_root_node("subpictures");
    xmlpp::Element* spu_node = root_node->add_child("stream")->add_child("spu");
    spu_node->set_attribute("start", "00:00:00.0");
    spu_node->set_attribute("end",   "00:00:00.0");
    spu_node->set_attribute("highlight", "MenuHighlight.png");
    spu_node->set_attribute("select",    "MenuSelect.png");
    // используем "родной" прозрачный цвет в png
    //std::string trans_color = ColorToString(BLACK2_CLR);
    //spu_node->set_attribute("transparent", trans_color);
    spu_node->set_attribute("force",       "yes");
    spu_node->set_attribute("autoorder",   "rows");

    // * активация
    SPRenderVis r_vis(rct_lst, spu_node);
    m_rgn.Accept(r_vis);
    SaveMenuPicture(mn_dir, "MenuHighlight.png", cnv_pix);
    doc.write_to_file_formatted(AppendPath(mn_dir, "Menu.xml"));

    // * выбор
    r_vis.SetSelect(true);
    m_rgn.Accept(r_vis);
    SaveMenuPicture(mn_dir, "MenuSelect.png", cnv_pix);

    // * остальное
    fs::copy_file(SConsAuxDir()/"menu_SConscript", fs::path(mn_dir)/"SConscript");
    // для последующего рендеринга
    SetCBDirty(cnv_buf);

    return true;
}

class MenuRenderVis: public CommonRenderVis
{
    typedef CommonRenderVis MyParent;
    public:
                  MenuRenderVis(RectListRgn& r_lst): MyParent(r_lst) {}

         //virtual  void  VisitImpl(MenuRegion& menu_rgn);
         virtual RefPtr<Gdk::Pixbuf> CalcBgShot();
         virtual  void  Visit(FrameThemeObj& fto);

    protected:
     virtual CanvasBuf& FindCanvasBuf(MenuRegion& menu_rgn) { return GetAuthorPCB(menu_rgn); }
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
        using namespace boost;
        ShotFunctor s_fnr = lambda::bind(&GetAuthoredMenu, mn);
        pix = MakeCopyWithSz(GetAuthoredLRS(s_fnr), sz);
    }
    else
        pix = GetPrimaryAuthoredShot(mi, sz);
    return pix;
}

void MenuRenderVis::Visit(FrameThemeObj& fto)
{
    // используем кэш
    FTOAuthorData& pix_data = fto.GetData<FTOAuthorData>(AUTHOR_TAG);
    drw->CompositePixbuf(pix_data.GetPix(), CalcRelPlacement(fto.Placement()));
}

bool RenderMainPicture(const std::string& out_dir, Menu mn, int i)
{
    Author::Info((str::stream() << "Rendering menu \"" << mn->mdName << "\" ...").str());

    CanvasBuf& cnv_buf = UpdateAuthoredMenu(mn);
    SaveMenuPicture(MakeMenuPath(out_dir, mn, i), "Menu.png", cnv_buf.FramePixbuf());

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
    bool IsMenuToBe4_3();
    menu_list << "Is4_3 = " << (IsMenuToBe4_3() ? 1 : 0) << "\n";
    // список меню
    menu_list << "List = [\n";
    using namespace boost;
    // за один проход можно сделать и подготовку, и рендеринг вспомог. данных
    // * подготовка к рендерингу
    // * создание слоев/субтитров выбора и скрипта
    //ForeachMenu(lambda::bind(&SetAuthorControl, lambda::_1, lambda::_2));
    ForeachMenu(lambda::bind(&RenderSubPictures, boost::ref(out_dir), lambda::_1, lambda::_2, 
                             boost::ref(menu_list)));
    menu_list << "]\n" << io::endl;
    PulseRenderProgress();

    // * отрисовка основного изображения
    ForeachMenu(lambda::bind(&RenderMainPicture, boost::ref(out_dir), lambda::_1, lambda::_2));
}

} // namespace Project


