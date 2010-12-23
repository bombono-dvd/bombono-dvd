//
// mgui/project/media-browser.cpp
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
#include "dnd.h"
#include "video.h"

#include <mgui/timeline/mviewer.h>
#include <mgui/sdk/packing.h>
#include <mgui/sdk/menu.h>
#include <mgui/sdk/widget.h>
#include <mgui/sdk/window.h>
#include <mgui/dialog.h> // MessageBox
#include <mgui/gettext.h>
#include <mgui/key.h>
#include <mgui/win_utils.h>
#include <mgui/execution.h> // PipeOutput()

#include <mgui/editor/toolbar.h>

#include <mbase/project/table.h> 

#include <mlib/sigc.h> 
#include <mlib/sdk/logger.h>
#include <mlib/filesystem.h>

#include <glib/gstdio.h> // g_stat

io::pos FFmpegSizeForDVD(double sec, int vrate, int anum)
{
    int snum = 0; // DeVeDe учитывает и субтитры (по 8kbit/s), но ИМХО ерунда
    // 1000/8 (бит в байте)
    io::pos res = (io::pos)sec * (vrate + TRANS_AUDIO_BITRATE*anum + snum*8) * 125;
    // изначально хотелось показывать "точный" размер по битрейту, но там
    // своя двусмысленность (почему сумма вдруг стала большe?); к тому же при самом
    // транскодировании требуется вести расчет по всему проекту + необходим подсчет
    // каждого файла снова (со страховкой) - заводить 2 варианта расчета значит усложнять.
    return res * Project::TRANS_OVER_ASSURANCE;
}

namespace Project
{

MediaStore::TrackFields& MediaStore::Fields()
{
    return GetColumnFields<MediaStore::TrackFields>();
}

MediaStore::MediaStore() { set_column_types(GetColumnRecord<TrackFields>()); }

MediaItem MediaStore::Get(const Gtk::TreeRow& row)
{
    return row.get_value(Fields().media);
}

MediaItem MediaStore::GetMedia(const Gtk::TreeIter& itr) const
{
    return Get(*itr);
}

bool MediaStore::row_drop_possible_vfunc(const TreeModel::Path& dest, const Gtk::SelectionData& data) const
{
    Gtk::TreePath tmp_path = GetSourcePath(data);

    RefPtr<MediaStore> this_ = MakeRefPtr(const_cast<MediaStore*>(this));
    // 1 главы вообще никак нельзя передвигать в браузере
    if( IsChapter(Project::GetMedia(this_, tmp_path)) )
        return false;

    // 2
    bool can_drop = false;
    if( dest.get_depth() == 1 )
        can_drop = true;
    else
    {
        tmp_path = dest;
        tmp_path.up();
        if( Project::GetMedia(this_, tmp_path)->IsFolder() )
            can_drop = true;
    }
    return can_drop;
}

bool ValidateMediaInsertionPos(Gtk::TreePath& brw_pth, bool want_ia)
{
    bool insert_after = false;
    if( !brw_pth.empty() )
    {
        while( brw_pth.get_depth() > 1 )
        {
            insert_after = true;
            brw_pth.up();
        }

        if( !insert_after )
            insert_after = want_ia;
    }
    return insert_after;
}

static void OnURIsDrop(MediaBrowser& brw, const StringList& paths, const Point& loc)
{
    Gtk::TreePath brw_pth;
    Gtk::TreeViewDropPosition pos;
    brw.get_dest_row_at_pos(loc.x, loc.y, brw_pth, pos);

    ValidatePath(brw_pth);
    bool insert_after = ValidateMediaInsertionPos(brw_pth, pos != Gtk::TREE_VIEW_DROP_BEFORE);

    TryAddMedias(paths, brw, brw_pth, insert_after);
}

static void SetConstEALink(PostAction& pa, PostActionType typ, const ActionFunctor& on_updater)
{
    ASSERT_RTL( typ != patEXP_LINK );
    pa.paTyp  = typ;
    pa.paLink = 0;

    on_updater();
}

static void SetEALink(PostAction& pa, MediaItem mi, const ActionFunctor& on_updater)
{
    ASSERT_RTL(mi); // пустая явная ссылка? - нет пути!
    pa.paTyp  = patEXP_LINK;
    pa.paLink = mi;

    on_updater();
}

EndActionMenuBld::EndActionMenuBld(PostAction& pa, const ActionFunctor& on_updater,
                                   const Functor& cc_adder)
: MyParent(pa.paLink, false), pAct(pa), onUpdater(on_updater), ccAdder(cc_adder) 
{
    ASSERT( onUpdater );
    ASSERT( ccAdder );
}

ActionFunctor EndActionMenuBld::CreateAction(Project::MediaItem mi)
{
    return bb::bind(&SetEALink, boost::ref(pAct), mi, onUpdater);
}

void EndActionMenuBld::AddConstantItem(const std::string& label, PostActionType typ)
{
    AddPredefinedItem(label, typ == pAct.paTyp, 
                      bb::bind(&SetConstEALink, boost::ref(pAct), typ, onUpdater));
}

void EndActionMenuBld::AddConstantChoice()
{
    ccAdder(*this);
}

static void VideoAddConstantChoice(EndActionMenuBld& bld)
{
    void AddPA(EndActionMenuBld& bld, PostActionType pat, bool is_video);
    AddPA(bld, patAUTO,       true);
    AddPA(bld, patNEXT_TITLE, true);
    AddPA(bld, patPLAY_ALL,   true);
}

static bool OnOBButtonPress(ObjectBrowser& brw, const RightButtonFunctor& fnr, GdkEventButton* event)
{
    // :TRICKY: переопределением on_button_press_event() было бы все проще;
    // но пусть будет - как пример Gtk::Widget::event()

    // сделано по аналогии с list_button_press_event_cb, GtkFileChooserDefault
    // Суть в том, что:
    // - GtkTreeView не пускает сигнал нажатия после себя -> регистрир. до
    // - хочется выполнить обработку GtkTreeView по новому выделению -> вложенный
    //   gtk_widget_event(), c защитой от рекурсии
    // - в конце точно надо не пускать сигнал дальше
    static bool in_press = false;
    if( in_press )
        return false;

    //if (event->button != 3)
    //  return FALSE;

    in_press = true;
    brw.event((GdkEvent*)event);
    in_press = false;

    if( IsRightButton(event) )
        if( MediaItem mi = GetCurMedia(brw) )
            fnr(brw, mi, event);

    return true;
}

RTCache& GetRTC(VideoItem vi)
{
    RTCache& rtc = vi->GetData<RTCache>();
    if( !rtc.isCalced )
    {
        const std::string& fname = GetFilename(*vi);
        bool is_mpeg2;
        std::string err_string;
        rtc.reqTrans = !IsVideoDVDCompliant(fname.c_str(), err_string, is_mpeg2);

        FFInfo ffi(GetFilename(*vi).c_str());
        rtc.duration = Duration(ffi);
        rtc.vidSz    = ffi.vidSz;
        rtc.dar      = DAspectRatio(ffi);
        // расчет числа аудио
        AVFormatContext* ic = ffi.iCtx;
        // :TODO: отрефакторить цикл
        rtc.audioNum = 0;
        for( int i=0; i < (int)ic->nb_streams; i++ )
        {
            AVCodecContext* avctx = ic->streams[i]->codec;
            if( avctx->codec_type == CODEC_TYPE_AUDIO )
                rtc.audioNum++;
        }

        rtc.isCalced = true;
    }

    return rtc;
}

bool RequireTranscoding(VideoItem vi)
{
    return GetRTC(vi).reqTrans;
}

static void AppendNamedValue(Gtk::VBox& vbox, RefPtr<Gtk::SizeGroup> sg, const char* name, 
                             const std::string& value)
{
    Gtk::Label& lbl = NewManaged<Gtk::Label>(value);
    SetAlign(lbl);
    AppendWithLabel(vbox, sg, lbl, name);
}

// синхронизировать со списком DVDDims
int DVDWidths[] = { 0, 352, 352, 704, 720 };

static int MinDVDHeight(bool is_pal)
{
    return is_pal ? 288 : 240 ;
}

Point DVDDimension(DVDDims dd, bool is_pal)
{
    ASSERT( dd != dvdAUTO );
    int wdh = DVDWidths[dd];
    int hgt;
    if( dd == dvd352s )
        hgt = MinDVDHeight(is_pal);
    else
        hgt = is_pal ? 576 : 480 ;
    return Point(wdh, hgt);
}

Point DVDDimension(DVDDims dd)
{
    return DVDDimension(dd, IsPALProject());
}

// расчет по оригиналу
static DVDDims CalcDimsAuto(RTCache& rtc)
{
    Point orig_sz(rtc.vidSz);

    DVDDims dd = dvd352;
    if( orig_sz.x > DVDDimension(dd).x )
        dd = dvd720;
    else if( orig_sz.y <= MinDVDHeight(IsPALProject()) )
        dd = dvd352s;
    return dd;
}

DVDDims CalcDimsAuto(VideoItem vi)
{
    return CalcDimsAuto(GetRTC(vi));
}

int CalcVRateAuto(DVDDims dd)
{
    return (dd == dvd352s) ? 2000 : (dd == dvd352) ? 3000 : 5000 ;
}

// в kbit/s, как у DeVeDe
const int MIN_TRANS_VRATE = 400;
const int MAX_TRANS_VRATE = 8500;

DVDTransData DVDDims2TDAuto(DVDDims dd)
{
    DVDTransData res;
    res.dd    = dd;
    res.vRate = CalcVRateAuto(dd);
    return res;
}

DVDTransData GetRealTransData(VideoItem vi)
{
    DVDTransData res = vi->transDat;
    if( res.dd == dvdAUTO || (res.vRate < MIN_TRANS_VRATE) || (res.vRate > MAX_TRANS_VRATE) )
        res = DVDDims2TDAuto(CalcDimsAuto(vi));
    return res;
}

// показываем этот список в настройках транскодирования
DVDDims TransList[]= {dvd352s, dvd352, dvd720};

DVDDims Index2Dimension(int idx)
{
    ASSERT( (idx >= 0) && (idx < (int)ARR_SIZE(TransList)) );
    return TransList[idx];
}

struct BitrateControls
{
    Gtk::SpinButton  vRate;
  Gtk::ComboBoxText  cmbDims;
         Gtk::Label  szLbl;

            RTCache& rtc;

    BitrateControls(RTCache& rtc_): rtc(rtc_) {}
};

static void SetAutoBitrate(BitrateControls& bc, DVDDims dd)
{
    bc.vRate.set_value(CalcVRateAuto(dd));
}

static DVDDims Dimensions(BitrateControls& bc)
{
    return Index2Dimension(bc.cmbDims.get_active_row_number());
}

int OutAudioNum(int i_anum)
{
    ASSERT( i_anum >= 0 );
    return std::min(i_anum, 8);
}

io::pos CalcTransSize(RTCache& rtc, int vrate)
{
    return FFmpegSizeForDVD(rtc.duration, vrate, OutAudioNum(rtc.audioNum));
}

static void OnBitrateControlsChanged(BitrateControls& bc, bool dims_changed)
{
    if( dims_changed )
        SetAutoBitrate(bc, Dimensions(bc));

    // расчет размера результата
    gint64 fsize = CalcTransSize(bc.rtc, bc.vRate.get_value());
    bc.szLbl.set_text(ShortSizeString(fsize));
}

static void SetDims(BitrateControls& bc, DVDDims dd)
{
    Gtk::ComboBoxText& dim_cmb = bc.cmbDims;
    for( int i=0; i<(int)ARR_SIZE(TransList); i++ )
        if( Index2Dimension(i) == dd )
        {
            dim_cmb.set_active(i);
            break;
        }
}

static void OnDefBitrate(BitrateControls& bc)
{
    DVDDims dd = CalcDimsAuto(bc.rtc); 
    SetDims(bc, dd);
    SetAutoBitrate(bc, dd);
}

static void SetTransData(VideoItem vi, DVDDims dd, int vrate)
{
    DVDTransData& td = vi->transDat;
    td.dd    = dd;
    td.vRate = vrate;
}

static void RunBitrateCalc(VideoItem vi, Gtk::Dialog& dlg)
{
    dlg.set_title(boost::format("%1% (%2%)") % _("Bitrate Calculator") % vi->mdName % bf::stop);

    DialogVBox& vbox = AddHIGedVBox(dlg);
    RefPtr<Gtk::SizeGroup> sg = vbox.labelSg;

    RTCache& rtc = GetRTC(vi);
    BitrateControls bc(rtc);
    DVDTransData td = GetRealTransData(vi);
    {
        Gtk::VBox& bc_box = PackParaBox(vbox);
        Gtk::SpinButton& vrate_btn = bc.vRate;
        ConfigureSpin(vrate_btn, td.vRate, MAX_TRANS_VRATE, MIN_TRANS_VRATE, 100);
        Gtk::Label& kbps_lbl = Pack2NamedWidget(bc_box, SMCLN_("Video bitrate"), vrate_btn,
                                                sg);
        const char* kbps_txt = _("kbps");
        kbps_lbl.set_label(kbps_txt);
    
        Gtk::ComboBoxText& dim_cmb = bc.cmbDims;
        for( int i=0; i<(int)ARR_SIZE(TransList); i++ )
        {
            DVDDims dd2 = Index2Dimension(i);
            dim_cmb.append_text(PointToStr(DVDDimension(dd2)));
        }
        AppendWithLabel(bc_box, sg, dim_cmb, SMCLN_("Dimensions"));
        SetDims(bc, td.dd);

        Gtk::Label& sz_lbl = bc.szLbl;
        SetAlign(sz_lbl);
        AppendWithLabel(bc_box, sg, sz_lbl, SMCLN_("Expected file size"));
    
        OnBitrateControlsChanged(bc, false); // расчет размера
        vrate_btn.signal_value_changed().connect(bb::bind(&OnBitrateControlsChanged, b::ref(bc), false));
        dim_cmb.signal_changed().connect(bb::bind(&OnBitrateControlsChanged, b::ref(bc), true));

        Gtk::Button& def_btn = PackStart(bc_box, NewManaged<Gtk::Button>(_("_Restore default bitrate"), true));
        def_btn.signal_clicked().connect(bb::bind(&OnDefBitrate, b::ref(bc)));
        //PackStart(vbox, NewManaged<Gtk::Button>(_("_Adjust disc usage"), true));
    }

    // О файле
    {
        std::string fname = GetFilename(*vi);
        FFInfo ffv(fname.c_str());
        Gtk::VBox& info_box = PackParaBox(vbox, _("Original file info"));

        AppendNamedValue(info_box, sg, SMCLN_("Dimensions"), PointToStr(ffv.vidSz));
        // :TODO: убрать подчеркивание
        AppendNamedValue(info_box, sg, RMU_(SMCLN_("_Duration (in seconds)")), Double2Str(Duration(ffv)));
        AppendNamedValue(info_box, sg, SMCLN_("Frame rate"), 
                         boost::format("%1% %2%") % Double2Str(FrameFPS(ffv)) % _("fps") % bf::stop);
        // :KLUGDE: не показываем оригинальный битрейт видеопотока:
        // - для mpeg2 это maxrate (пиковая нагрузка), к реальному -b vrate отношение
        //   не имеещий (для dvd ffmpeg ставит его как 9000kbps)
        // - для других кодеков (mpeg4, avc) ffmpeg вообще не определяет его
        // Потому, если и показывать, то только полный битрейт, ffv.iCtx->bit_rate,
        // он хоть адекватен
        //AppendNamedValue(vbox, SMCLN_("Video bitrate"),
        //                 boost::format("%1% %2%") % (GetVideoCtx(ffv)->bit_rate/1000) % kbps_txt % bf::stop);
        AppendNamedValue(info_box, sg, SMCLN_("Display aspect ratio"), 
                         boost::format("%1% : %2%") % rtc.dar.x % rtc.dar.y % bf::stop);
        AppendNamedValue(info_box, sg, SMCLN_("Audio stream number"), Int2Str(rtc.audioNum));
        AppendNamedValue(info_box, sg, SMCLN_("File size"), ShortSizeString(PhisSize(fname.c_str())));
    }

    if( CompleteAndRunOk(dlg) )
    {
        SetTransData(vi, Dimensions(bc), bc.vRate.get_value());
        UpdateDVDSize();
    }
}

static void ShowDVDCompliantStatus(VideoItem vi)
{
    std::string err_string(_("Reason For Transcoding"));
    std::string desc_string;
    bool is_mpeg2 = false;
    if( IsVideoDVDCompliant(GetFilename(*vi).c_str(), desc_string, is_mpeg2) )
        err_string = _("The video is DVD compliant.");
    else if( !is_mpeg2 )
        desc_string = _("The video is not MPEG2.");

    MessageBox(err_string, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, desc_string);
}

double Duration(VideoItem vi)
{
    return GetRTC(vi).duration;
}

static SizeStat ProjectStatEx(bool fixed_part)
{
    SizeStat ss;
    io::pos& sz = ss.prjSum;
    io::pos& tr_sz = ss.transSum;

    boost_foreach( VideoItem vi, AllVideos() )
    {
        if( RequireTranscoding(vi) )
        {
            RTCache& rtc = GetRTC(vi);
            int vrate    = fixed_part ? 0 : GetRealTransData(vi).vRate ;
            io::pos v_sz = CalcTransSize(rtc, vrate);

            tr_sz += v_sz;
            sz    += v_sz;
        }
        else
            sz += PhisSize(GetFilename(*vi).c_str());
    }
    sz += Author::MenusSize();

    return ss;
}

SizeStat ProjectStat()
{
    return ProjectStatEx(false);
}

io::pos ProjectSizeSum(bool fixed_part)
{
    return ProjectStatEx(fixed_part).prjSum;
}

DVDDims GetRealTD(VideoItem vi)
{
    return GetRealTransData(vi).dd;
}

double RelTransWeight(VideoItem vi)
{
    Point osz = DVDDimension(GetRealTD(vi));
    double square = osz.x*osz.y/double(352*240); // относительный вес видео

    return square * Duration(vi);
}

static void AdjustDiscUsage()
{
    if( MessageBox(_("Do you want to adjust disc usage?"), 
                   Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, "", true) != Gtk::RESPONSE_YES )
        return;

    io::pos dvd_sz  = DVDPayloadSize();
    io::pos work_sz = (dvd_sz - ProjectSizeSum(true)) / TRANS_OVER_ASSURANCE;

    double total_weight = 0.;
    boost_foreach( VideoItem vi, AllTransVideos() )
        total_weight += RelTransWeight(vi);
    ASSERT( total_weight > 0. );

    bool is_overflow = false;
    boost_foreach( VideoItem vi, AllTransVideos() )
    {
        // в kbit/s
        int vrate = work_sz * RelTransWeight(vi)/(total_weight * 125 * Duration(vi));
        if( vrate < MIN_TRANS_VRATE )
            is_overflow = true;

        vrate = std::max(vrate, MIN_TRANS_VRATE);
        vrate = std::min(vrate, MAX_TRANS_VRATE);
        SetTransData(vi, GetRealTD(vi), vrate);
    }
    UpdateDVDSize();

    if( is_overflow )
        MessageBox(_("Too many videos for this disc size. Please select a bigger disc size or remove some videos."), 
                   Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
    else
        MessageBox(BF_("Disc usage is %1%%% now.") % Round(100*(ProjectSizeSum()/(double)dvd_sz)) % bf::stop, 
                   Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
}

// вывод кодировки в формате iconv
// если локаль не национальная, то явно ставить opts как "-L lang"
bool GetEncoding(const std::string& fpath, std::string& enc_str, 
                 const std::string& opts = std::string())
{
    bool res = PipeOutput(boost::format("enca %1%-i %2%") % opts % FilenameForCmd(fpath) % bf::stop, 
                          enc_str).IsGood();
    if( res )
    {
        int len = enc_str.size();
        res = (len >= 1) && (enc_str[len-1] = '\n');
        if( res )
            enc_str = std::string(enc_str.c_str(), enc_str.size()-1);
    }
    return res;
}

static void OnSelectSubtitles(Gtk::FileChooserButton& s_btn, Gtk::ComboBoxText& enc_cmb)
{
    std::string path = s_btn.get_filename();
    if( !path.empty() )
    {
        std::string enc_str;
        if( GetEncoding(path, enc_str) )
        {
            std::string old_enc = enc_cmb.get_active_text();
            // по-другому установить по значению(!) нельзя 
            enc_cmb.set_active_text(enc_str);
            // восстанавливаем старую, если новой нет в списке
            if( !old_enc.empty() && enc_cmb.get_active_text().empty() )
                enc_cmb.set_active_text(old_enc);
        }
    }
}

static void SetSubtitles(VideoItem vi, Gtk::Dialog& dlg)
{
    SubtitleData& dat = vi->subDat;

    Gtk::FileChooserButton& s_btn = NewManaged<Gtk::FileChooserButton>(
        _("Select subtitles"), Gtk::FILE_CHOOSER_ACTION_OPEN);
    Gtk::CheckButton& ds_btn   = NewManaged<Gtk::CheckButton>(_("_Turn on subtitles by default"), true);
    Gtk::ComboBoxText& enc_cmb = NewManaged<Gtk::ComboBoxText>();

    DialogVBox& vbox = AddHIGedVBox(dlg);
    SetFilename(s_btn, dat.pth);
    AppendWithLabel(vbox, s_btn, SMCLN_("Select subtitles"));
    ds_btn.set_active(dat.defShow);
    PackStart(vbox, ds_btn);
    // взято у DeVeDe с минимальными изменениями
    // :TRICKY: держим список строк в формате iconv (что и понимает spumux), и
    // при помощи enca надеемся на совпадение алиасов кодировок
    io::stream strm(DataDirPath("copy-n-paste/codepages.lst").c_str());
    std::string enc_str;
    for( int i=0; std::getline(strm, enc_str), strm; i++ )
    {
        enc_cmb.append_text(enc_str);
        if( dat.encoding == enc_str )
            enc_cmb.set_active(i);
    }
    s_btn.signal_file_set().connect(bb::bind(&OnSelectSubtitles, b::ref(s_btn), b::ref(enc_cmb)));

    AppendWithLabel(vbox, enc_cmb, SMCLN_("_Encoding"));

    if( CompleteAndRunOk(dlg) )
    {
        dat.pth      = s_btn.get_filename();
        dat.defShow  = ds_btn.get_active();
        dat.encoding = enc_cmb.get_active_text();
    }
}

DialogParams SubtitlesDialog(VideoItem vi, Gtk::Widget* par_wdg)
{
    return DialogParams(_("Add Subtitles"), bb::bind(&SetSubtitles, vi, _1), 400, par_wdg);
}

static void OnMBButtonPress(ObjectBrowser& brw, MediaItem mi, GdkEventButton* event)
{
    Gtk::Menu& mn  = NewPopupMenu(); 
    Gtk::MenuItem& ea_itm = AppendMI(mn, NewManaged<Gtk::MenuItem>(_("End Action")));
    // только видео
    VideoItem vi = IsVideo(mi);
    if( SetEnabled(ea_itm, vi) )
        ea_itm.set_submenu(EndActionMenuBld(vi->PAction(), boost::function_identity,
                                            VideoAddConstantChoice).Create());

    AppendSeparator(mn);
    bool tr_enabled = IsTransVideo(vi);
    AddEnabledItem(mn, _("Adjust Bitrate to Fit to Disc"), &AdjustDiscUsage, tr_enabled);
    // калькулятор
    AddDialogItem(mn, DialogParams(_("Bitrate Calculator"), bb::bind(&RunBitrateCalc, vi, _1), 
                                   350, &brw), tr_enabled);
    AddEnabledItem(mn, _("Reason For Transcoding"), bb::bind(&ShowDVDCompliantStatus, vi), vi);
    AppendSeparator(mn);

    AddDialogItem(mn, SubtitlesDialog(vi, &brw), vi);

    Popup(mn, event, true);
}

void SetOnRightButton(ObjectBrowser& brw, const RightButtonFunctor& fnr)
{
    sig::connect(brw.signal_button_press_event(), bb::bind(&OnOBButtonPress, boost::ref(brw), fnr, _1), false);
}

MediaBrowser::MediaBrowser(RefPtr<MediaStore> a_lst)
{
    set_model(a_lst);
    BuildStructure();

    SetupURIDrop(*this, bb::bind(&OnURIsDrop, boost::ref(*this), _1, _2));
    SetOnRightButton(*this, bb::bind(&OnMBButtonPress, _1, _2, _3));
}

// Названия типов для i18n
F_("Video")
F_("Chapter")
F_("Still Picture")

void RenderMediaType(Gtk::CellRendererText* rndr, MediaItem mi)
{
    rndr->property_text() = gettext(mi->TypeString().c_str());
}

void MediaBrowser::BuildStructure()
{
    RefPtr<MediaStore> ms = GetMediaStore();
    const MediaStore::TrackFields& trk_fields = MediaStore::Fields();

    SetupBrowser(*this, trk_fields.media.index(), true);

    // 1 миниатюра + имя
    {
        Gtk::TreeView::Column& name_cln = *Gtk::manage( new Gtk::TreeView::Column(_("Name")) );
        // ширину колонки можно менять
        name_cln.set_resizable(true);
        
        name_cln.pack_start(trk_fields.thumbnail, false);
        
        // 2 имя
        Gtk::CellRendererText& rndr = *Gtk::manage( new Gtk::CellRendererText() );
        //name_cln.set_renderer(rndr, trk_fields.title);
        SetupNameRenderer(name_cln, rndr, ms);

        append_column(name_cln);
    }

    // 3 тип
    {
        Gtk::TreeView::Column& cln  = *Gtk::manage( new Gtk::TreeView::Column(_("Type")) );
        Gtk::CellRendererText& rndr = *Gtk::manage( new Gtk::CellRendererText() );

        cln.pack_start(rndr, false);
        // не используем данных,- вычисляем на лету
        //cln.set_renderer(rndr, trk_fields.title);
        SetRendererFnr(cln, rndr, ms, &RenderMediaType);

        append_column(cln);
    }
}

void ExecuteForMedia(MediaBrowser& mb, MediaActionFnr fnr)
{
    if( Gtk::TreeIter itr = GetSelectPos(mb) )
    {
        RefPtr<ObjectStore> os = mb.GetObjectStore();
        fnr(os->GetMedia(itr), itr);
    }
}

void DeleteBrowserMedia(MediaItem md, Gtk::TreeIter& itr,
                        RefPtr<MediaStore> ms)
{
    GetBrowserDeletionSign(md) = true;
    DeleteMedia(ms, itr);
}

void ConfirmDeleteBrowserMedia(MediaItem md, Gtk::TreeIter& itr,
                               RefPtr<MediaStore> ms)
{
    if( ConfirmDeleteMedia(md) )
        DeleteBrowserMedia(md, itr, ms);
}

void DeleteMediaFromBrowser(MediaBrowser& mb)
{
    ExecuteForMedia(mb, bb::bind(&ConfirmDeleteBrowserMedia, _1, _2, mb.GetMediaStore()));
}

void MediaBrowser::DeleteMedia()
{
    DeleteMediaFromBrowser(*this);
}

void PackMediaBrowser(Gtk::Container& contr, MediaBrowser& brw)
{
    Gtk::VBox& vbox   = *Gtk::manage(new Gtk::VBox);
    contr.add(PackWidgetInFrame(vbox, Gtk::SHADOW_OUT));

//     // не меньше чем размер шрифта элемента в списке
//     Gtk::Label& label = *Gtk::manage(new Gtk::Label("<span font_desc=\"Sans Bold 12\">Media List</span>"));
//     label.set_use_markup(true);
//     vbox.pack_start(label, Gtk::PACK_SHRINK);
//     Gtk::Requisition req = label.size_request();
//     label.set_size_request(0, req.height+10);
    vbox.pack_start(MakeTitleLabel(_("Media List")), Gtk::PACK_SHRINK);
    PackHSeparator(vbox);

//     Gtk::ScrolledWindow* scr_win = Gtk::manage(new Gtk::ScrolledWindow);
//     scr_win->set_shadow_type(Gtk::SHADOW_NONE); //IN);
//     scr_win->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
//     vbox.pack_start(*scr_win);
//     scr_win->add(brw);
    vbox.pack_start(PackInScrolledWindow(brw));
}

void OnMBChangeCursor(MediaBrowser& brw, Gtk::Button* edit_btn)
{
    bool is_on = false;
    if( MediaItem mi = GetCurMedia(brw) )
        is_on = GetViewerFunctor(mi);

    edit_btn->set_sensitive(is_on);
}

static void SetDefaultButtonOnEveryMap(Gtk::Button& btn)
{
    // при смене вкладки, например, теряется фокус по умолчанию
    btn.signal_map().connect(bb::bind(&SetDefaultButton, boost::ref(btn)));
}

void PackMediaBrowserAll(Gtk::Container& contr, MediaBrowser& brw, ActionFunctor add_media_fnr, 
                         ActionFunctor remove_media_fnr, ActionFunctor edit_media_fnr)
{
    Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 2));
    contr.add(*vbox);
    {
        PackMediaBrowser(*vbox, brw);

        // группа кнопок
        //Gtk::HBox& hbox = *Gtk::manage(new Gtk::HBox(true, 4));
        Gtk::HButtonBox& hbox = CreateMListButtonBox();
        vbox->pack_start(hbox, Gtk::PACK_SHRINK);
        {
            Gtk::Button* add_btn = CreateButtonWithIcon("", Gtk::Stock::ADD, 
                                                        _("Add Media from File Browser"));
            hbox.pack_start(*add_btn);
            //bbox.pack_start(*add_btn);
            add_btn->signal_clicked().connect(add_media_fnr);
            // при смене вкладки теряется фокус по умолчанию
            //SetDefaultButton(*add_btn);
            SetDefaultButtonOnEveryMap(*add_btn);

            Gtk::Button* rm_btn = CreateButtonWithIcon("", Gtk::Stock::REMOVE,
                                                       _("Remove Media"));
            hbox.pack_start(*rm_btn);
            //bbox.pack_start(*rm_btn);
            rm_btn->signal_clicked().connect(remove_media_fnr);
            // Translators: it is normal to translate "Edit" as " " (empty) and
            // to keep the button small; let the tooltip tell the purpose. The same thing 
            // with the button "Edit" in Menu List
            // Замечание: так как переводчики не обращают внимание (фин, например), то ставим пусто,
            // чтобы качество GUI не зависело от локали
            //const char* edit_text = C_("MediaBrowser", "Edit");
            const char* edit_text = "";
            Gtk::Button* edit_btn = CreateButtonWithIcon(edit_text, Gtk::Stock::YES, _("Make Chapters for Video"));
            hbox.pack_start(*edit_btn);
            //bbox.pack_start(*edit_btn);
            edit_btn->signal_clicked().connect(edit_media_fnr);
            // управление состоянием кнопки
            edit_btn->set_sensitive(false);
            brw.signal_cursor_changed().connect( 
                bb::bind(&OnMBChangeCursor, boost::ref(brw), edit_btn) );
        }
    }
}

Gtk::TreePath& GetBrowserPath(StorageItem si)
{
    return LocalPath(si.get());
}

RefPtr<MediaStore> CreateEmptyMediaStore()
{
    RefPtr<MediaStore> ms(new MediaStore);
    void RegisterMSHandlers(RefPtr<MediaStore> ms);
    RegisterMSHandlers(ms);

    return ms;
}

RefPtr<MediaStore> CreateMediaStore()
{
    RefPtr<MediaStore> ms = CreateEmptyMediaStore();

    PublishMediaStore(ms);
    return ms;
}

io::pos PhisSize(const char* fname)
{
    io::pos res = 0;
    struct stat buf;
    if( g_stat(fname, &buf) == 0 )
        res = buf.st_size;
    return res;
}

} // namespace Project

