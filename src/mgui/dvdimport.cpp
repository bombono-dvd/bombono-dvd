//
// mgui/dvdimport.cpp
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

#include "dvdimport.h"

#include <mgui/render/common.h> // FillEmpty()
#include <mgui/project/mb-actions.h> // TryAddMedia()

#include "sdk/window.h"
#include "sdk/packing.h"
#include "sdk/widget.h"
#include "sdk/browser.h"   // VideoPE
#include "dialog.h"

#include <mbase/project/table.h>
#include <mlib/filesystem.h>
#include <mlib/sdk/logger.h>

#include <iomanip>

namespace DVD {

static VobFields& VF()
{
    return VobFields::Instance();
}

ImportData::ImportData(): srcChooser(Gtk::FILE_CHOOSER_ACTION_OPEN), 
    curPage(ipNONE_PAGE), isBreak(false), addToProject(false),
    // value, lower, upper, step_increment = 1, page_increment = 10, page_size = 0
    previewAdj(0., 0., 0.)
{
    SetOutputFormat(thumbPlyr,   fofRGB);
    SetOutputFormat(previewPlyr, fofRGB);
}

static std::string PageTitle(int pg_num)
{
    std::string title;
    switch( pg_num )
    {
    case ipCHOOSE_SOURCE: 
        title = "Choose Source DVD-Video";
        break;
    case ipSELECT_VOBS: 
        title = "Select Videos to Import";
        break;
    case ipCHOOSE_DEST: 
        title = "Select Folder to Save Videos";
        break;
    case ipIMPORT_PROC: 
        title = "Importing...";
        break;
    case ipEND: 
        title = "Import is completed.";
        break;
    default: 
        ASSERT(0);
    }
    return title;
}

static bool IsForwardMove(ImportData& id, ImportPage cur_ip)
{
    return id.curPage + 1 == cur_ip;
}

static void SetCurPageComplete(Gtk::Assistant& ast, bool is_complete)
{
    ast.set_page_complete(*ast.get_nth_page(ast.get_current_page()), is_complete);
}

static void CompleteSelection(ImportData& id, bool is_on)
{
    bool real_complete = is_on && id.vobList->children().size();
    SetCurPageComplete(id.ast, real_complete);
}

static void ForeachVob(ImportData& id, boost::function<bool(Gtk::TreeIter&)> fnr)
{
    Gtk::TreeModel::Children children = id.vobList->children();
    for( Gtk::TreeIter itr = children.begin(), end = children.end(); itr != end; ++itr )
        if( !fnr(itr) )
            break;
}

static bool CheckVobSelect(Gtk::TreeIter& itr, bool& has_check)
{
    has_check = itr->get_value(VF().selState);
    return !has_check;
}

static void OnSelVob(ImportData& id)
{
    bool res = false;
    ForeachVob(id, bl::bind(&CheckVobSelect, bl::_1, boost::ref(res)));
    CompleteSelection(id, res);    
}

static bool OpenVob(VobPtr vob, Mpeg::FwdPlayer& plyr, dvd_reader_t* dvd)
{
    ptr::shared<VobStreambuf> strm_buf = new VobStreambuf(vob, dvd);
    return plyr.OpenFBuf(strm_buf);
}

static bool OnSelectIdle(ImportData& id)
{
    int num = id.numToThumb++;
    VobArr& arr = id.dvdVobs;

    bool res = num < (int)arr.size();
    if ( res )
    {
        Mpeg::FwdPlayer& plyr = id.thumbPlyr;
        if( OpenVob(arr[num], plyr, id.reader->dvd) )
        {
            Gtk::TreeRow row = *id.vobList->get_iter(Gtk::TreePath((Gtk::TreePath::size_type)1, num));
            Point sz = PixbufSize(row[VF().thumbnail]);

            double tm = Mpeg::GetMediaSize(plyr);
            double preview_time = 3.0; // показываем кадр третьей (или первой?) секунды, иначе - середину
            if( preview_time >= tm )
                preview_time = tm / 2;
            row[VF().thumbnail] = Project::VideoPE(plyr, preview_time).Make(sz).RWPixbuf();
        }
        else
            LOG_INF << "OpenFBuf() failed: " << plyr.MInfo().ErrorReason() << io::endl;

        plyr.CloseFBuf();
    }
    return res;
}

static void OnPreparePage(ImportData& id)
{
    ImportPage ip = (ImportPage)id.ast.get_current_page();
    // пересчитываем при движении вперед
    if( ipSELECT_VOBS == ip )
    {
        if( IsForwardMove(id, ip) )
        {
            ReaderPtr& reader = id.reader;
            ASSERT(reader);
            VobArr& vobs = id.dvdVobs;
            vobs.clear();
            FillVobArr(vobs, reader->dvd);
    
            // временные изображения - по спекам DVD достаточно двух
            Point sz4_3 = Project::Calc4_3Size(SMALL_THUMB_WDH);
            const int thumb_square = sz4_3.x * sz4_3.y;
            RefPtr<Gdk::Pixbuf> pix4_3  = CreatePixbuf(sz4_3);
            FillEmpty(pix4_3);
            RefPtr<Gdk::Pixbuf> pix16_9 = CreatePixbuf(Project::CalcProportionSize(Point(16, 9), thumb_square));
            FillEmpty(pix16_9);
    
            RefPtr<Gtk::ListStore>& vob_list = id.vobList;
            vob_list->clear();
            for( VobArr::iterator itr = vobs.begin(), end = vobs.end(); itr != end; ++itr )
            {
                Gtk::TreeRow row = *vob_list->append();
                Vob& vob = (**itr);
                row[VF().selState]  = false;
                row[VF().name]      = VobFName(vob.pos);
                row[VF().thumbnail] = vob.aspect == af4_3 ? pix4_3 : pix16_9;
                std::string desc = (str::stream(Mpeg::SecToHMS(vob.tmLen, true)) <<  ", "
                                    << vob.sz.x << "x" << vob.sz.y << ", "
                                    << (vob.aspect == af4_3 ? "4:3" : "16:9") << ", " 
                                    << std::fixed << std::setprecision(2) << vob.Count()/512. << " Mb").str();
                row[VF().desc]      = desc;
            }
            CompleteSelection(id, false);

            // заново устанавливаем
            id.numToThumb = 0;
        }
        
        // бывают случаи вызова prepare на одной странице дважды 
        // - set_current_page() + "map"
        if( id.curPage != ipSELECT_VOBS ) // дважды
            id.thumbIdler.ConnectIdle(bl::bind(&OnSelectIdle, boost::ref(id)));
    }
    else
        id.CloseIdlers();

    id.curPage = ip;
}

static void OnPrepareSelect(Gtk::Widget& sel_group, Gtk::Assistant& ast)
{
    (ipSELECT_VOBS == ast.get_current_page()) ? sel_group.show() : sel_group.hide() ;
}

ReaderPtr OpenDVD(const std::string& dvd_path, bool& is_pal)
{
    ReaderPtr reader;
    if( !dvd_path.empty() ) // зачем зря диск "топтать-то"
    {
        dvd_reader_t* dvd = DVDOpen(dvd_path.c_str());
        if( dvd )
        {
            reader.reset(new Reader(dvd));

            ifo_handle_t* vmg_ifo = ifoOpen(dvd, 0); // VMG
            if( vmg_ifo )
            {
                is_pal = IsPAL(vmg_ifo);    
                ifoClose(vmg_ifo); // ок
            }
            else
                reader.reset();    // плохо
        }
    }
    return reader;
}

static void OnSelectSource(ImportData& id)
{
    Gtk::FileChooserWidget& fcw = id.srcChooser;
    std::string dvd_path = fcw.get_filename().raw();

    bool is_pal;
    ReaderPtr rd = OpenDVD(dvd_path, is_pal);
    if( rd && id.addToProject && (is_pal != Project::AData().PalTvSystem()) )
    {
        rd.reset();
        id.errLbl.show();
    }
    else
        id.errLbl.hide();

    id.reader = rd;
    SetCurPageComplete(id.ast, id.reader);
}

bool SetVobSel(Gtk::TreeIter& itr, bool select_all)
{
    itr->set_value(VF().selState, select_all);
    return true;
}

static void OnSelectionButton(ImportData& id, bool select_all)
{
    ForeachVob(id, bl::bind(SetVobSel, bl::_1, select_all));
    CompleteSelection(id, select_all);
}

static RefPtr<Gdk::Pixbuf> CreatePreviewPix(AspectFormat af)
{
    int wdh = BIG_THUMB_WDH;
    double ratio = (af == af4_3) ? 0.75 : 0.5625; // 4:3 или 16:9

    RefPtr<Gdk::Pixbuf> pix = CreatePixbuf(Point(wdh, int(wdh*ratio)));
    FillEmpty(pix);
    return pix;
}

void InitPreview(ImportData& id, AspectFormat af)
{
    id.previewImg.set(CreatePreviewPix(af));
    id.previewAdj.set_upper(0);
    id.previewAdj.set_value(0);

    id.previewPlyr.CloseFBuf();
    id.previewIdler.Disconnect();
}

static bool UpdatePreview(ImportData& id)
{
    Mpeg::FwdPlayer& plyr = id.previewPlyr;
    if( plyr.IsOpened() )
    {
        double tm = GetFrameTime(plyr, (int)id.previewAdj.get_value());
        RefPtr<Gdk::Pixbuf> pix = id.previewImg.get_pixbuf();

        Project::VideoPE(plyr, tm).Fill(pix);
        id.previewImg.queue_draw();
    }
    return false;
}

static void OnPreviewValueChanged(ImportData& id)
{
    id.previewIdler.Disconnect();
    id.previewIdler.ConnectIdle(bl::bind(&UpdatePreview, boost::ref(id)));
}

static void OnVobActivate(const Gtk::TreePath& pth, ImportData& id)
{
    VobPtr vob = id.dvdVobs[pth[0]];

    InitPreview(id, vob->aspect);
    Mpeg::FwdPlayer& plyr = id.previewPlyr;
    if( OpenVob(vob, id.previewPlyr, id.reader->dvd) )
    {
        int upper = plyr.MInfo().FramesCount();
        id.previewAdj.set_upper(upper);
    
        OnPreviewValueChanged(id);
    }
}

static void OnSelectDest(Gtk::FileChooserWidget& fcw, ImportData& id)
{
    id.destPath = fcw.get_filename().raw();
    SetCurPageComplete(id.ast, fs::is_directory(id.destPath));
}

static void FillLabelForImport(Gtk::Label& lbl, const std::string& desc_str)
{
    lbl.set_markup("<span weight=\"bold\">" + desc_str + "</span>");
}

static Gtk::Label& PackLabelForImport(Gtk::VBox& vbox, const std::string& desc_str)
{
    Gtk::Label& lbl = PackStart(vbox, NewManaged<Gtk::Label>());
    SetAlign(lbl);
    FillLabelForImport(lbl, desc_str);

    return lbl;
}

static void PackFCWPage(Gtk::VBox& vbox, Gtk::FileChooserWidget& fcw,
                        const std::string& desc_str)
{
    PackLabelForImport(vbox, desc_str);

    //PackHSeparator(vbox);
    //Gtk::Alignment& alg = NewManaged<Gtk::Alignment>();
    //alg.set_padding(5, 5, 5, 5);
    //PackStart(vbox, PackWidgetInFrame(alg, Gtk::SHADOW_ETCHED_IN), Gtk::PACK_EXPAND_WIDGET);
    //Add(alg, id.srcChooser);

    PackStart(vbox, fcw, Gtk::PACK_EXPAND_WIDGET);
}

static void PackSelectionButton(Gtk::HBox& hbox, RefPtr<Gtk::SizeGroup> sg, bool select_all,
                                ImportData& id)
{
    const char* text = select_all ? "Select All" : "Unselect All" ;
    Gtk::Button& btn = PackStart(hbox, NewManaged<Gtk::Button>(text));
    sg->add_widget(btn);

    btn.signal_clicked().connect(bl::bind(&OnSelectionButton, boost::ref(id), select_all));
}

static double ToPercent(uint32_t written_cnt, uint32_t sector_cnt)
{
    return written_cnt/(double)sector_cnt*100;
}

static void UpdateImportBar(Gtk::ProgressBar& bar, uint32_t written_cnt, uint32_t sector_cnt)
{
    SetPercent(bar, ToPercent(written_cnt, sector_cnt));
    IteratePendingEvents();
}

static void OnImportStop(ImportData& id)
{
    // мы не можем пустить исключение здесь, потому что оно пойдет
    // и через C-шный код (Gtk) - отложим
    if( MessageBox("Are you sure to stop importing?", 
                   Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO) == Gtk::RESPONSE_YES )
        id.isBreak = true;
}

const char* USERT_BREAK_STR = "Break import!";

static char* CopyFilePartWithProgress(io::stream& dst, char* buf, int len, 
                                      uint32_t& written_cnt, uint32_t sector_cnt, ImportData& id)
{
    dst.write(buf, len);

    written_cnt += len >> 11; // в секторах меряем
    UpdateImportBar(id.prgBar, written_cnt, sector_cnt);
    if( id.isBreak )
        throw std::runtime_error(USERT_BREAK_STR);

    return buf;
}

static void OnApply(ImportData& id)
{
    std::string& dir_path = id.destPath;
    VobArr& arr = id.dvdVobs;
    ASSERT( fs::is_directory(dir_path) );

    bool res = Project::HaveFullAccess(dir_path);
    if( !res )
        MessageBox("Cant write to folder " + dir_path + " (check permissions).",
                   Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
    else
    {
        Gtk::TreeModel::Children children = id.vobList->children();

        // * просчитаем кол-во работы (в записываемых секторах)
        typedef std::vector<uint32_t> SecArr;
        SecArr sec_arr;
        uint32_t sector_cnt = 0;
        int i = 0;
        for( Gtk::TreeIter itr = children.begin(), end = children.end(); itr != end; ++itr, ++i )
        {
            sec_arr.push_back(sector_cnt);
            if( itr->get_value(VF().selState) )
                sector_cnt += arr[i]->Count();
        }
        
        // * запись
        try
        {
            i = 0;
            for( Gtk::TreeIter itr = children.begin(), end = children.end(); itr != end; ++itr, ++i )
                if( itr->get_value(VF().selState) )
                {
                    std::string name  = itr->get_value(VF().name);
                    std::string fname = AppendPath(dir_path, name);
                    if( fs::exists(fname) )
                        if( MessageBox("The file already " + name + " already exists. Overwrite it?",
                                       Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO) != Gtk::RESPONSE_YES )
                        {
                            res = false;    
                            break;
                        }
                    // импорт vob - выделение в блок для закрытия файла перед добавлением
                    // в проект
                    {
                        io::stream out_strm(fname.c_str(), iof::out);
                        uint32_t written_cnt = sec_arr[i];
                        ReadFunctor fnr = bl::bind(&CopyFilePartWithProgress, boost::ref(out_strm), 
                                                   bl::_1, bl::_2, 
                                                   boost::ref(written_cnt), sector_cnt, 
                                                   boost::ref(id));
                        //ReadFunctor fnr = MakeWriter(out_strm);
            
                        id.prgLabel.set_text(name);
                        UpdateImportBar(id.prgBar, written_cnt, sector_cnt);
            
                        VobPtr vob = arr[i];
                        ExtractVob(fnr, vob, id.reader->dvd);
                    }
                    
                    if( id.addToProject )
                    {
                        std::string err_str;
                        Gtk::TreePath pth;
                        if( !Project::TryAddMedia(fname.c_str(), pth, err_str) )
                            LOG_ERR << "Import error: " << err_str << io::endl;
                    }
                }
        }
        catch(const std::exception& err)
        {
            res = false;
	    const char* what = err.what();
	    if( what && (strcmp(what, USERT_BREAK_STR) != 0) )
	      MessageBox("Import error!", Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, what);
        }
    }

    const char* final = res ? "Videos successfully imported." 
        : "Import has been interrupted." ;
    FillLabelForImport(id.finalMsg, final);
    id.ast.set_current_page(ipEND);
}

static Gtk::VBox& AppendVBoxAsPage(Gtk::Assistant& ast, Gtk::Container*& page)
{
    Gtk::VBox& vbox = NewManaged<Gtk::VBox>(false, 5);
    ast.append_page(vbox);

    page = &vbox;
    return vbox;
}

// поменяем условия упаковки
static void SetExpandFill(Gtk::Widget& child)
{
    GtkWidget* child_wdg = (GtkWidget*)child.gobj();
    GtkWidget* box_ = gtk_widget_get_parent(child_wdg);
    if( box_ && GTK_IS_BOX(box_) )
    {
        GtkContainer* cont = (GtkContainer*)box_;
        gtk_container_child_set(cont, child_wdg, "expand", TRUE,  NULL);
        gtk_container_child_set(cont, child_wdg, "fill", TRUE,  NULL);
    }
}

void ConstructImporter(ImportData& id)
{
    Gtk::Assistant& ast = id.ast;
    ast.set_title("DVD-Video Import");
    ast.set_default_size(600, 500);

    ast.signal_cancel().connect(&Gtk::Main::quit);
    ast.signal_close().connect(&Gtk::Main::quit);

    boost::reference_wrapper<ImportData> ref_id(id);
    ast.signal_prepare().connect(bl::bind(&OnPreparePage, ref_id));
    ast.signal_apply().connect(bl::bind(&OnApply, ref_id));

    for( int i=0; i<ipPAGE_NUM; i++ )
    {
        Gtk::Container* page = 0;
        Gtk::AssistantPageType typ = Gtk::ASSISTANT_PAGE_CONTENT;
        bool is_complete = true;
        switch( i )
        {
        case ipCHOOSE_SOURCE:
            {
                Gtk::VBox& vbox = AppendVBoxAsPage(ast, page);
                typ = Gtk::ASSISTANT_PAGE_INTRO;

                Gtk::FileChooserWidget& fcw = id.srcChooser;
                PackFCWPage(vbox, fcw, "Choose DVD disc, DVD folder or iso image file.");
                fcw.signal_selection_changed().connect(bl::bind(&OnSelectSource, ref_id));

                // 
                // От установки фильтра отказался, потому что бывают диски DVD, не открывающиеся на
                // чтение для простого пользователя и при этом не зашифрованные (мой DVD recorder такие 
                // создает). В этом случае единственный выход - выбрать прямо устройство /dev/dvd и т.д.
                //

                //Gtk::FileFilter ff;
                //ff.add_pattern("*.BUP"); // DVD-Video
                //ff.add_pattern("*.IFO");
                //ff.add_pattern("*.VOB");
                //ff.add_pattern("*.iso"); // iso image
                //fcw.set_filter(ff);

                Gtk::Label& err_lbl = id.errLbl;
                err_lbl.set_markup("<span foreground=\"red\">NTSC/PAL mismatch. Try another disc"
                                   " or import to project of corresponding type.</span>");
                ast.add_action_widget(err_lbl); // по умолчанию не видна
                SetExpandFill(err_lbl);
                SetAlign(err_lbl);
            }
            break;
        case ipSELECT_VOBS:
            {
                Gtk::HBox& hbox = NewManaged<Gtk::HBox>(false, 5);
                ast.append_page(hbox);
                page = &hbox;

                RefPtr<Gtk::ListStore>& vob_list = id.vobList;
                vob_list = Gtk::ListStore::create(VF());

                Gtk::TreeView& view = NewManaged<Gtk::TreeView>(vob_list);
                view.signal_row_activated().connect(bl::bind(&OnVobActivate, bl::_1, ref_id));
                // нужно/нет
                Gtk::CellRendererToggle& sel_rndr = *dynamic_cast<Gtk::CellRendererToggle*>(
                    view.get_column(view.append_column_editable("", VF().selState) - 1)->get_first_cell_renderer());
                sel_rndr.signal_toggled().connect(bl::bind(&OnSelVob, ref_id));

                // имя
                Gtk::TreeView::Column& name_cln = NewManaged<Gtk::TreeView::Column>("Name");
                name_cln.set_resizable(true);
                name_cln.set_expand(true);

                name_cln.pack_start(VF().thumbnail, false);
                Gtk::CellRendererText& rndr = NewManaged<Gtk::CellRendererText>();
                rndr.property_xpad() = 5;
                rndr.property_weight() = PANGO_WEIGHT_BOLD;

                //name_cln.pack_start(VF().name);
                name_cln.pack_start(rndr);
                name_cln.set_renderer(rndr, VF().name);
                view.append_column(name_cln);
                view.append_column("Details", VF().desc);

                Gtk::ScrolledWindow& scr_win = PackStart(hbox, NewManaged<Gtk::ScrolledWindow>(),
                                                         Gtk::PACK_EXPAND_WIDGET);
                scr_win.set_shadow_type(Gtk::SHADOW_IN);
                scr_win.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
                scr_win.add(view);

                // окно предпросмотра
                {
                    Gtk::Alignment& alg = PackStart(hbox, NewManaged<Gtk::Alignment>(0.5, 0.0, 0., 0.));
                    Gtk::VBox& vbox     = Add(alg, NewManaged<Gtk::VBox>());

                    InitDefPreview(id);
                    Add(vbox, id.previewImg);

                    Gtk::HScale& scl = PackStart(vbox, NewManaged<Gtk::HScale>(id.previewAdj));
                    SetScaleSecondary(scl);
                    id.previewAdj.signal_value_changed().connect(
                        bl::bind(&OnPreviewValueChanged, ref_id));
                }

                // кнопки выбора
                {
                    Gtk::Alignment& alg = NewManaged<Gtk::Alignment>(0., 0.5, 0., 1.);
                    ast.add_action_widget(alg);
                    SetExpandFill(alg);

                    Gtk::HBox& hbox = Add(alg, NewManaged<Gtk::HBox>(false, 2));
                    RefPtr<Gtk::SizeGroup> sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

                    PackSelectionButton(hbox, sg, true,  id);
                    PackSelectionButton(hbox, sg, false, id);

                    ast.signal_prepare().connect(
                        bl::bind(&OnPrepareSelect, boost::ref(alg), boost::ref(ast)));
                    alg.show_all();
                }
            }
            break;
        case ipCHOOSE_DEST:
            {
                Gtk::VBox& vbox = AppendVBoxAsPage(ast, page);
                typ = Gtk::ASSISTANT_PAGE_CONFIRM;

                Gtk::FileChooserWidget& fcw = NewManaged<Gtk::FileChooserWidget>(Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
                PackFCWPage(vbox, fcw, "It is desirable the destination folder to be empty.");

                fcw.signal_selection_changed().connect(
                    bl::bind(&OnSelectDest, boost::ref(fcw), ref_id));
            }
            break;
        case ipIMPORT_PROC: 
            {
                Gtk::Alignment& alg = NewManaged<Gtk::Alignment>(0.5, 0.8, 1.0, 0.0);
                ast.append_page(alg);
                page = &alg;
                typ = Gtk::ASSISTANT_PAGE_PROGRESS;
                is_complete = false;

                Gtk::VBox& vbox = Add(alg, NewManaged<Gtk::VBox>(false, 5));
                Gtk::Label& lbl = PackStart(vbox, id.prgLabel);
                SetAlign(lbl);
                Gtk::ProgressBar& bar = PackStart(vbox, id.prgBar);
                SetPercent(bar, 0);

                // кнопка отмены
                Gtk::Alignment& btn_alg = PackStart(vbox, NewManaged<Gtk::Alignment>(1.0, 0.5, 0.1, 1.0));
                Gtk::Button& btn = Add(btn_alg, *Gtk::manage(new Gtk::Button(Gtk::Stock::STOP)));
                btn.signal_clicked().connect(bl::bind(&OnImportStop, ref_id));
            }
            break;
        case ipEND:
            SetAlign(PackStart(AppendVBoxAsPage(ast, page), id.finalMsg));
            typ = Gtk::ASSISTANT_PAGE_SUMMARY;
            break;
        default:
            ASSERT(0);
        }

        ASSERT(page);
        page->set_border_width(15);
        ast.set_page_complete(*page, is_complete);
        ast.set_page_type(*page, typ);
        ast.set_page_title(*page, PageTitle(i));
    }
}

void RunImport(Gtk::Window& par_win, const std::string& dvd_path)
{
    ImportData id;
    Gtk::Assistant& ast = id.ast;

    ast.set_transient_for(par_win);
    id.addToProject = true;

    ConstructImporter(id);
    
    if( !dvd_path.empty() )
    {
        // реализовать переход сразу на 2ю страницу оказалось
        // непростым делом:
        // - после явного открытия reader'а самое главное - 
        //   не дать srcChooser переустановить его вызовом
        //   OnSelectSource() =>
        // - перепробовав все, что можно (и скрытие, и удаление
        //   первой страницы, ...), стало ясно, что нельзя вызывать
        //   show_all() окна, если на первой странице =>
        // - простой переход на 2ю страницу дает сегфолт, если
        //   ранее не проходил show_all() (см. интересную функцию
        //   compute_last_button_state()) - пат
        // - и только смена типа с GTK_ASSISTANT_PAGE_CONTENT на любой
        //   другой дала нужный результат!

        bool is_pal;
        ReaderPtr rd = OpenDVD(dvd_path, is_pal);
        if( rd && (is_pal == Project::AData().PalTvSystem()) )
        {
            // чтоб не смутить OnPreparePage()
            id.curPage = ipCHOOSE_SOURCE;
            id.reader = rd;

            ast.set_page_type(*ast.get_nth_page(ipSELECT_VOBS), Gtk::ASSISTANT_PAGE_INTRO);
            ast.set_current_page(ipSELECT_VOBS);
        }
        else
            // не получилось - тогда хотя бы перейдем в директорию
            id.srcChooser.select_filename(dvd_path);
    }
    RunWindow(ast);
}

} // namespace DVD

